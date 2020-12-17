/*****************************************************************************
* lab2a.h for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#ifndef lab2a_h
#define lab2a_h

enum Lab2ASignals {
	ENCODER_UP = Q_USER_SIG,
	ENCODER_DOWN,
	IDLE_SIG,
	OCTAVE_CLICK,
	A4_CLICK,
	SET_CLICK
};


extern struct Lab2ATag AO_Lab2A;


void Lab2A_ctor(void);
void GpioHandler(void *CallbackRef);
void TwistHandler(void *CallbackRef);
void BtnHandler(void *CallbackRef);

void drawBackGround();
void drawNote(char* note);

void drawFrequency(float frequency);
void drawFrequencyLabel();

void drawError(int error);
void drawErrorBar(int error);

void setBackGroundColor();
void setDrawingBgColor();

void updateOctave(int octave);
void updateA4 (int frequency);

void drawHomeScreen(char* note, float frequency, int error);
void clearHomeScreen();

void drawOctaveSelection(int octave);
void clearOctaveScreen();

void tuneA4(int frequency);
void clearA4Screen();

#endif  
