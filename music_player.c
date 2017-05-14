/*
 * Music player for embedded system with Nios II processor
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <io.h>
#include "altera_up_avalon_audio.h"
#include "altera_avalon_pio_regs.h"
#include "altera_avalon_timer.h"
#include "uart.h"
#include "fatfs.h"
#include "diskio.h"
#include "alt_types.h"
#include "hw_controls.h"
#include "track_controls.h"

#define ESC 27
#define CLEAR_LCD_STRING "[2J"
#define STOPPED 0
#define PLAYING 1
#define PAUSED 2
#define FORWARDS 3



int main()
{
	int i = 0;
	tracks = 0;
	disk_initialize((uint8_t) 0);
	put_rc(f_mount((uint8_t) 0, &Fatfs[0]));

	//Set up display
	lcd = fopen("/dev/lcd_display", "w");

	// Set up Audio
	audio_dev = alt_up_audio_open_dev ("/dev/Audio");
	if ( audio_dev == NULL) alt_printf ("Error: could not open audio device \n");
	else alt_printf ("Opened audio device \n");

	//Initialize tracks
	getTracks();
	currentTrack = 0;
	openTrack();
	curTrackRemaining = trackLengths[currentTrack];
	displayTrack();

	// Register button presses
	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(BUTTON_PIO_BASE, 0xf);
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(BUTTON_PIO_BASE, 0x0);
	alt_irq_register(BUTTON_PIO_IRQ, (void *)0, handleButtonPress);

	// State Machine
	int readSize, backSize;
	while(1) {
		// Set Statuses based on interrupts
		if (forwButton == 2 && status == FORWARDS) {
			status = PLAYING;
			forwButton = 0;
			if (stopButton == 2) stopButton = 0;
			if (ppButton == 2) ppButton = 0;
			if (backButton == 2) backButton = 0;
		}

		if (forwButton == 1 && status == PLAYING || status == FORWARDS) {
			status = FORWARDS;
		} else if (backButton == 1 && status == PLAYING) {
			while (backButton == 1) {
				if (currentFile.fptr > 294000) {
					f_lseek(&currentFile, currentFile.fptr - 294000);
					curTrackRemaining += 294000;
				} else {
					f_lseek(&currentFile, 0);
					curTrackRemaining = trackLengths[currentTrack];
					continue;
				}

				backSize = 49000;
				while(backSize > 0) {
					if (backButton == 2) {
						break;
					}

					curTrackRemaining -= 512;
					backSize -= 512;
					play(512);
				}
			}
			if (stopButton == 2) stopButton = 0;
			if (ppButton == 2) ppButton = 0;
			if (forwButton == 2) forwButton = 0;
			backButton = 0;
		} else if (forwButton == 2 && (status == PAUSED || status == STOPPED)) {
			changeTrack(1);
			forwButton = 0;
		} else if (backButton == 2 && status == STOPPED) {
			changeTrack(-1);
			backButton = 0;
		} else if (backButton == 2 && status == PAUSED) {
			status = STOPPED;
			f_lseek(&currentFile, 0);
			curTrackRemaining = trackLengths[currentTrack];
			backButton = 0;
			currentTime = 0;
			displayTrack();
		} else if (stopButton == 2) {
			// reset file to beginning
			f_lseek(&currentFile, 0);
			curTrackRemaining = trackLengths[currentTrack];
			currentTime = 0;
			status = STOPPED;
			stopButton = 0;
			displayTrack();
		} else if (ppButton == 2) {
			if (status == PLAYING ) {
				status = PAUSED;
			} else if (status == PAUSED || status == STOPPED) {
				status = PLAYING;
			}

			ppButton = 0;
		}

		if (status == PLAYING || status == FORWARDS) {
			if (curTrackRemaining == 0) {
				if (status == FORWARDS) {
					forwButton = 2;
				}
				changeTrack(1);
			}

			if (curTrackRemaining >= 512) {
				readSize = 512;
				curTrackRemaining -= 512;
			} else {
				readSize = curTrackRemaining;
				curTrackRemaining = 0;
			}
			play(readSize);
		}
	}

    return 0;
}
