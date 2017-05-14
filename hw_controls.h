#include <stdio.h>


char** trackNames;
uint32_t* trackLengths;
int tracks, currentTrack, status;
uint32_t currentTime = 0;
DIR dir;
FATFS Fatfs[_VOLUMES];
FIL currentFile;
uint8_t buffer[8192];
unsigned int l_buf, r_buf;
alt_up_audio_dev * audio_dev;
long curTrackRemaining;

int forwButton = 0;
int backButton = 0;
int ppButton = 0;
int stopButton = 0;
FILE *lcd;


static void handleButtonPress(void* context, alt_u32 id) {
    usleep(50000);
    uint8_t data = IORD(BUTTON_PIO_BASE, 0) ^ 0xF;

    //Fast forward button pressed
    if ((data | 0xE) == 0xF && forwButton == 0) forwButton++;
    else if ((data | 0xE) == 0xE && forwButton == 1) forwButton++;

    //Play/Pause button pressed
    if ((data | 0xD) == 0xF && ppButton == 0) ppButton++;
    else if ((data | 0xD) == 0xD && ppButton == 1) ppButton++;

    //Stop button pressed
    if ((data | 0xB) == 0xF && stopButton == 0) stopButton++;
    else if ((data | 0xB) == 0xB && stopButton == 1) stopButton++;

    //Fast back button pressed
    if ((data | 0x7) == 0xF && backButton == 0) backButton++;
    else if ((data | 0x7) == 0x7 && backButton == 1) backButton++;

    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(BUTTON_PIO_BASE, 0x0);
    return;
}


void put_rc(FRESULT rc)
{
    const char *str =
        "OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
        "INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
        "INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
        "LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0";
    FRESULT i;

    for (i = 0; i != rc && *str; i++) {
        while (*str++);
    }
    xprintf("rc=%u FR_%s\n", (uint32_t) rc, str);
}