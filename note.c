#include "note.h"
#include <string.h>
#include <math.h>
//#include "lcd.h"

extern int error;
extern int a4;
extern char note[3];

//array to store note names for findNote
static char notes[12][3]={"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};

//finds and prints note of frequency and deviation from note
void findNote(float f) {
	float c=(a4/440.0) * 261.63;
	float r;
	int oct=4;
	int n=0;
	//determine which octave frequency is in
	if(f >= c) {
		while(f > c*2) {
			c=c*2;
			oct++;
		}
	}
	else { //f < C4
		while(f < c) {
			c=c/2;
			oct--;
		}
	
	}

	//find note below frequency
	//c=middle C
	r=c*root2;
	while(f > r) {
		c=c*root2;
		r=r*root2;
		n++;
	}

	if ((f-c) > (r-f))
		n++;

	float expFreq = a4 * pow(2, ((n - 9)/12.0) + oct - 4);

	error = (int)(1200 * log10(f/expFreq)/log10(2)) % 100;
	if (error < 0)
		error *= -1;
	if (error > 99)
		error = 99;

	strcpy(note, notes[n]);

   /*
	//determine which note frequency is closest to
	if((f-c) <= (r-f)) { //closer to left note
		WriteString("N:");
		WriteString(notes[note]);
		WriteInt(oct);
		WriteString(" D:+");
		WriteInt((int)(f-c+.5));
		WriteString("Hz");
	}
	else { //f closer to right note
		note++;
		if(note >=12) note=0;
		WriteString("N:");
		WriteString(notes[note]);
		WriteInt(oct);
		WriteString(" D:-");
		WriteInt((int)(r-f+.5));
		WriteString("Hz");
	}
   */
}
