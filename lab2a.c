/*****************************************************************************
* lab2a.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#define AO_LAB2A

#include <stdlib.h>
#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "lcd.h"
#include <string.h>
#include <stdbool.h>

bool inHomeScreen = false;
char fr[9] = "Note: ";
char note[3] = "";

extern float frequency;
extern int octave;
extern int m;
extern int incr;
extern int sample_size;
extern int a4;
extern int error;

void adjust_fft_func_values(int octave);

typedef struct Lab2ATag  {               //Lab2A State machine
	QActive super;
}  Lab2A;

/* Setup state machines */
/**********************************************************************/
static QState Lab2A_initial (Lab2A *me);
static QState Lab2A_on      (Lab2A *me);
static QState homeMenu  (Lab2A *me);
static QState octaveMenu  (Lab2A *me);
static QState a4Menu	(Lab2A *me);

/**********************************************************************/


Lab2A AO_Lab2A;


void Lab2A_ctor(void)  {
	Lab2A *me = &AO_Lab2A;
	QActive_ctor(&me->super, (QStateHandler)&Lab2A_initial);
}


QState Lab2A_initial(Lab2A *me) {
	xil_printf("\n\rInitialization");
    return Q_TRAN(&Lab2A_on);
}

QState Lab2A_on(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("\n\rOn");
			}
			
		case Q_INIT_SIG: {
			initLCD();
			clrScr();
			drawBackGround();
			return Q_TRAN(&homeMenu);
			}
	}
	
	return Q_SUPER(&QHsm_top);
}


/* Create Lab2A_on state and do any initialization code if needed */
/******************************************************************/

QState homeMenu(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			inHomeScreen = true;
			//strcpy(note, "A#");
			strcat(fr, note);
			drawHomeScreen(fr, frequency, error);
			strcpy(fr, "Note: ");
			xil_printf("Startup homeMenu\n");
			return Q_HANDLED();
		}
		
		case ENCODER_UP: {
			xil_printf("ENCODER UP\n");
			return Q_HANDLED();
		}

		case ENCODER_DOWN: {
			xil_printf("ENCODER DOWN\n");
			return Q_HANDLED();
		}

		case IDLE_SIG: {
			//strcpy(note, "A#");
			strcat(fr, note);
			drawNote(fr);
			drawFrequency(frequency);
			drawError(error);
			drawErrorBar(error);
			strcpy(fr, "Note: ");
			return Q_HANDLED();
		}

		case OCTAVE_CLICK:  {
			xil_printf("Changing to Octave Menu\n");
			clearHomeScreen();
			inHomeScreen = false;
			return Q_TRAN(&octaveMenu);
		}

		case A4_CLICK: {
			xil_printf("Changing to A4 Menu\n");
			clearHomeScreen();
			inHomeScreen = false;
			return Q_TRAN(&a4Menu);
		}

		case SET_CLICK: {
			return Q_HANDLED();
		}

	}

	return Q_SUPER(&Lab2A_on);

}

QState octaveMenu(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("Entering Octave Menu\n");
			drawOctaveSelection(octave);
			return Q_HANDLED();
		}
		
		case ENCODER_UP: {
			xil_printf("Octave up\n");
			if (octave < 9) {
				octave++;
				updateOctave(octave);
				adjust_fft_func_values(octave);
			}
			return Q_HANDLED();
		}

		case ENCODER_DOWN: {
			xil_printf("Octave down\n");
			if (octave > 0) {
				octave--;
				updateOctave(octave);
				adjust_fft_func_values(octave);
			}
			return Q_HANDLED();
		}

		case SET_CLICK:  {
			xil_printf("Changing to home screen\n");
			clearOctaveScreen();
			return Q_TRAN(&homeMenu);
		}

		case A4_CLICK: {
			xil_printf("Changing to a4Menu");
			clearOctaveScreen();
			return Q_TRAN(&a4Menu);
		}

		case OCTAVE_CLICK:
			return Q_HANDLED();

	}

	return Q_SUPER(&Lab2A_on);

}

QState a4Menu(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("Entering A4 Final");
			tuneA4(a4);
			return Q_HANDLED();
		}

		case ENCODER_UP: {
			xil_printf("A4 up\n");
			if (a4 < 460) {
				a4++;
				updateA4(a4);
			}
			return Q_HANDLED();
		}

		case ENCODER_DOWN: {
			xil_printf("A4 down\n");
			if (a4 > 420) {
				a4--;
				updateA4(a4);
			}
			return Q_HANDLED();
		}

		case SET_CLICK:  {
			xil_printf("Changing to home screen\n");
			clearA4Screen();
			return Q_TRAN(&homeMenu);
		}

		case OCTAVE_CLICK: {
			xil_printf("Changing to Octave Menu");
			clearA4Screen();
			return Q_TRAN(&octaveMenu);
		}
		case A4_CLICK:
			return Q_HANDLED();
	}

	return Q_SUPER(&Lab2A_on);
}

void adjust_fft_func_values(int octave) {
	switch(octave) {
		case 0:
		case 1:
		case 2:
		case 4: {
			m = 4;
			incr = 16;
			sample_size = 1;
			break;
		}
		case 5:{
			m = 3;
			incr = 8;
			sample_size = 2;
			break;
		}
		case 6:
		case 7: {
			m = 2;
			incr = 4;
			sample_size = 4;
			break;
		}
		case 8: {
			m = 1;
			incr = 2;
			sample_size = 8;
			break;
		}
		case 9: {
			m = 0;
			incr = 1;
			sample_size = 16;
			break;
		}
	}
}

/* set up functions */
void setBackGroundColor() {
	setColor(52, 235, 146);
}

void setDrawingBgColor() {
	setColorBg(52, 235, 146);
}

void drawBackGround() {
	setBackGroundColor();
	fillRect(20, 20, 220, 300);
}
/* set up functions */

/* homeMenu draw functions */
void drawHomeScreen(char* note, float frequency, int error) {
	drawFrequencyLabel();
	drawFrequency(frequency);
	drawNote(note);
	drawError(error);
}

void drawNote(char* note) {
	setFont(BigFont);
	setDrawingBgColor();
	setColor(255, 255, 255);
	lcdPrint(note, 50, 130);
	drawHLine(50, 150, 150);
	setBackGroundColor();
}

void drawFrequency(float frequency) {
	setFont(SevenSegNumFont);
	setColor(255, 255, 255);
	setColorBg(0, 0, 0);
	char frequencyStr[6] = "00000";
	char temp = '0';
	int i;
	for (i = 4; i > -1; i--) {
		temp = (int)frequency%10 + 48;
		frequencyStr[i] = temp;
		frequency /= 10;
	}
	lcdPrint(frequencyStr, 50, 70);
	setBackGroundColor();
}

void drawFrequencyLabel() {
	setFont(BigFont);
	setColor(255, 255, 255);
	setDrawingBgColor();
	lcdPrint("Frequency:", 50, 50);
}

void drawError(int error) {
	setFont(BigFont);
	setColor(255, 0, 0);
	setDrawingBgColor();
	lcdPrint("Error: ", 50, 160);
	char errStr[9];
	itoa(error, errStr, 10);
	strcat(errStr, " cents");
	if (errStr[6] == 's')
		strcat(errStr, " ");
	lcdPrint(errStr, 50, 190);
	setBackGroundColor();
}

void drawErrorBar(int error) {
	setColor(242, 245, 66);
	fillRect(50, 220, 50 + (error * 1.5), 250);
	setBackGroundColor();
	fillRect(50 + (error * 1.5), 220, 200, 250);
}

void clearHomeScreen() {
	setBackGroundColor();
	setDrawingBgColor();
	fillRect(50, 50, 220, 250);
}
/* homeMenu draw functions */

/* octaveMenu draw functions */
void drawOctaveSelection(int octave) {
	setFont(BigFont);
	setColor(255, 255, 255);
	setDrawingBgColor();
	lcdPrint("Octave:", 70, 100);
	setFont(SevenSegNumFont);
	char octaveStr[2];
	itoa(octave, octaveStr, 10);
	lcdPrint(octaveStr, 100, 130);
	setBackGroundColor();
}

void updateOctave(int octave) {
	setDrawingBgColor();
	setFont(SevenSegNumFont);
	setColor(255, 255, 255);
	char octaveStr[2];
	itoa(octave, octaveStr, 10);
	lcdPrint(octaveStr, 100, 130);
	setBackGroundColor();
}

void clearOctaveScreen() {
	setBackGroundColor();
	setDrawingBgColor();
	fillRect(70, 100, 220, 200);
}
/* octaveMenu draw functions */

/* tuneA4Menu draw functions */
void tuneA4(int frequency) {
	setFont(BigFont);
	setColor(255, 255, 255);
	setDrawingBgColor();
	lcdPrint("Tune A4:", 70, 100);
	setFont(SevenSegNumFont);
	char A4Str[4];
	itoa(frequency, A4Str, 10);
	lcdPrint(A4Str, 70, 130);
	setBackGroundColor();
}

void updateA4(int frequency) {
	setFont(SevenSegNumFont);
	setColor(255, 255, 255);
	setDrawingBgColor();
	char A4Str[4];
	itoa(frequency, A4Str, 10);
	lcdPrint(A4Str, 70, 130);
	setBackGroundColor();
}

void clearA4Screen() {
	setBackGroundColor();
	setDrawingBgColor();
	fillRect(70, 100, 220, 200);
}
/* tunea4Menu draw functions */
