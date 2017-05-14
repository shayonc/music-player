#include <stdio.h>


int isWav(char* fileName) {
    int length = strlen(fileName);
    return fileName[length - 4] == '.' && fileName[length - 3] == 'W' && fileName[length - 2] == 'A' && fileName[length - 1] == 'V';
}

void getTracks() {
    int track = 0;
    uint8_t result;
    FILINFO Finfo;

    result = f_opendir(&dir, NULL);
    if (result) {
        put_rc(result);
        return;
    }

    while (1) {
        result = f_readdir(&dir, &Finfo);
        if ((result != FR_OK) || !Finfo.fname[0])
            break;
        if (!(Finfo.fattrib & AM_DIR) && isWav(Finfo.fname)) {
            tracks++;
        }
    }
    trackNames = (char**) malloc(sizeof(char*)*tracks);
    trackLengths = (uint32_t*) malloc(sizeof(uint32_t)*tracks);

    result = f_opendir(&dir, NULL);
    if (result) {
        put_rc(result);
        return;
    }
    while (1) {
        result = f_readdir(&dir, &Finfo);
        if ((result != FR_OK) || !Finfo.fname[0])
            break;
        if (!(Finfo.fattrib & AM_DIR) && isWav(Finfo.fname)) {
            trackNames[track] = (char*) malloc(sizeof(char)*13);
            strcpy(trackNames[track], Finfo.fname);
            trackLengths[track] = Finfo.fsize;
            track++;
        }
    }

}

void displayTrack() {
    if (lcd != NULL )
    {
        fprintf(lcd, "%c%s%02d   %02d:%02d/%02d:%02d\n%*s\n", ESC, CLEAR_LCD_STRING,
                currentTrack + 1, currentTime/60, currentTime%60, trackLengths[currentTrack]/11760000,
                (trackLengths[currentTrack]/196000)%60, 16, trackNames[currentTrack]);
    }
}

void openTrack() {
    f_open(&currentFile, trackNames[currentTrack], 1);
}

void play(long readSize){
    long read, total;
    int fifospace;
    uint8_t result;

    if (currentTime != currentFile.fptr/196000) {
        currentTime = currentFile.fptr/196000;
        displayTrack();
    }

    //p1 is number of bytes to read from file
    result = f_read(&currentFile, buffer, readSize, &read);
    if (result != FR_OK) {
        put_rc(result);
        return;
    }
    if (readSize != read) {
        xprintf("Did not fully read music");
        return;
    }

    uint32_t i;
    int increment = 4;
    if (status == FORWARDS) increment = 8;
    for (i = 0; i < read; i += increment) {
        l_buf = buffer[i] | ((unsigned int) buffer[i + 1]) << 8;
        r_buf = buffer[i + 2] | ((unsigned int) buffer[i + 3]) << 8;

        while(1) {
            fifospace = alt_up_audio_write_fifo_space(audio_dev, ALT_UP_AUDIO_RIGHT);
            if (fifospace > 0) // check if data is available
            {
                // write audio buffer
                alt_up_audio_write_fifo (audio_dev, &(r_buf), 1, ALT_UP_AUDIO_RIGHT);
                alt_up_audio_write_fifo (audio_dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);
                break;
            }
        }
    }

    return;
}

void changeTrack(int diff) {
    currentTrack = (currentTrack + diff) % tracks;
    if (currentTrack < 0) currentTrack = currentTrack + tracks;
    openTrack();
    curTrackRemaining = trackLengths[currentTrack];
    currentTime = 0;
    displayTrack();
}
