/*****************************************************************************
* bsp.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 27,2019
*****************************************************************************/

/**/
#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "xintc.h"
#include "xspi.h"
#include "xspi_l.h"
#include "xil_exception.h"

#include <stdio.h>
#include <stdlib.h>
#include <mb_interface.h>

#include "xparameters.h"
#include <xil_types.h>
#include <xil_assert.h>

#include <xio.h>
#include "xtmrctr.h"
#include "fft.h"
#include "note.h"
#include "trig.h"
#include "stream_grabber.h"
#include <stdbool.h>

/*****************************/

/* Define all variables and Gpio objects here  */

#define GPIO_CHANNEL1 1
#define SAMPLES 128 // AXI4 Streaming Data FIFO has size 512
#define M 7 //2^m=samples
#define CLOCK 100000000.0 //clock speed

extern bool inHomeScreen;
float frequency;
float sample_f;
int m = 5;
int incr = 32;
int sample_size = 1;
int octave = 2;
int error = 0;
int a4 = 440;

int octaveOffsets[10] = {5, 5, 5, 5, 4, 4, 2, 2, 1, 0};

static float q[SAMPLES];
static float w[SAMPLES];
static float zero[SAMPLES];

void debounceInterrupt(); // Write This function
void read_fsl_values(float *q, int n);

// Create ONE interrupt controllers XIntc
XIntc sys_intc;
// Create two static XGpio variables
static XGpio btn;
static XGpio encoder;
static XSpi spi;
// Suggest Creating two int's to use for determining the direction of twist
int state = 0;

/*..........................................................................*/
void BSP_init(void) {
/* Setup LED's, etc */
/* Setup interrupts and reference to interrupt handler function(s)  */

	/*
	 * Initialize the interrupt controller driver so that it's ready to use.
	 * specify the device ID that was generated in xparameters.h
	 *
	 * Initialize GPIO and connect the interrupt controller to the GPIO.
	 *
	 */

	XStatus Status;
	XStatus btnStatus;
	XStatus encoderStatus;
	XStatus spiStatus;

	Status = XST_SUCCESS;
	Status = XIntc_Initialize(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);

	if (Status == XST_SUCCESS)
		xil_printf("Interrupt controller initialized!\n\r");
	else
		xil_printf("Interrupt controller not initialized!\n\r");

	// Press Knob
	btnStatus = XIntc_Connect(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR,
			(XInterruptHandler)BtnHandler, &btn);
	if (btnStatus == XST_SUCCESS)
		xil_printf("Button handler connected\n\r");
	else
		xil_printf("Button handler not connected\n\r");

	btnStatus = XGpio_Initialize(&btn, XPAR_AXI_GPIO_BTN_DEVICE_ID);
	if (btnStatus == XST_SUCCESS)
			xil_printf("Button initialized\n\r");
		else
			xil_printf("Button not initialized\n\r");

	// Twist Knob
	encoderStatus = XIntc_Connect(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_ENCODER_IP2INTC_IRPT_INTR,
			(XInterruptHandler)TwistHandler, &encoder);
	if (encoderStatus == XST_SUCCESS)
		xil_printf("Encoder handler connected\n\r");
	else
		xil_printf("Encoder handler not connected\n\r");
		
	encoderStatus = XGpio_Initialize(&encoder, XPAR_ENCODER_DEVICE_ID);
	if (encoderStatus == XST_SUCCESS)
		xil_printf("Encoder initialized\n\r");
	else
		xil_printf("Encoder not initialized\n\r");

	// Initialize SPI driver
	XSpi_Config *spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
	if (spiConfig == NULL) {
		xil_printf("Can't find spi device!\n");
		return XST_DEVICE_NOT_FOUND;
	}

	spiStatus = XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
	if (spiStatus == XST_SUCCESS) {
		xil_printf("Initialized spi!\n");
	}

	// Reset SPI to a known value
	XSpi_Reset(&spi);

	// Set up controller to enable master mode
	u32 controlReg = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi,
			(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
			(~XSP_CR_TRANS_INHIBIT_MASK));

	// Select 1st slave device
	XSpi_SetSlaveSelectReg(&spi, ~0x01);
}
/*..........................................................................*/
void QF_onStartup(void) {                 /* entered with interrupts locked */

/* Enable interrupts */
	xil_printf("\n\rQF_onStartup\n"); // Comment out once you are in your complete program

	XStatus Status;

	// Press Knob
	// Enable interrupt controller
	XIntc_Enable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR);
	XIntc_Enable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_ENCODER_IP2INTC_IRPT_INTR);

	// Start interrupt controller
	Status = XIntc_Start(&sys_intc, XIN_REAL_MODE);

	if (Status == XST_SUCCESS)
		xil_printf("Interrupt controller started\n\r");
	else
		xil_printf("Interrupt controller failed to start\n\r");

	// register handler with Microblaze
	microblaze_register_handler((XInterruptHandler)XIntc_DeviceInterruptHandler,
					(void*)XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);

	// Global enable of interrupt
	microblaze_enable_interrupts();

	// Enable interrupt on the GPIO
	XGpio_SetDataDirection(&btn, 1, 0xFFFFFFFF);
	XGpio_InterruptEnable(&btn, 1);
	XGpio_InterruptGlobalEnable(&btn);

	XGpio_SetDataDirection(&encoder, 1, 0xFFFFFFFF);
	XGpio_InterruptEnable(&encoder, 1);
	XGpio_InterruptGlobalEnable(&encoder);

	xil_printf("Finished Initialization\r\n");

	read_fsl_values(q, 4096); //start pipeline
	stream_grabber_start();
	makeTrigLUT(SAMPLES);

	// Twist Knob

	// General
	// Initialize Exceptions
	// Press Knob
	// Register Exception
	// Twist Knob
	// Register Exception
	// General
	// Enable Exception

	// Variables for reading Microblaze registers to debug your interrupts.
//	{
//		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
//		u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
//		u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
//		u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
//		u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
//		u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
//		u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
//		u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
//		u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
//		u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
//		u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
//		u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
//		u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003; // & 0xMASK
//		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000; // & 0xMASK
//	}
}


void QF_onIdle(void) {        /* entered with interrupts locked */

//
    QF_INT_UNLOCK();                       /* unlock interrupts */

    {
    	// Write code to increment your interrupt counter here.
    	// QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN); is used to post an event to your FSM
		if (inHomeScreen) {

			sample_f = (int)100000000 >> (11 + octaveOffsets[octave]);

			//zero w array
			memcpy(w, zero, sizeof(float)*SAMPLES);

			frequency = fft(q, w, SAMPLES, M, sample_f);
			if (octave <= 5)
				frequency += 14;
			findNote(frequency);

			//pipeline
			stream_grabber_wait_enough_samples(4096/sample_size);
			fill_samples(q, 4096, m, incr, sample_size);
			stream_grabber_start();

			QActive_postISR((QActive *)&AO_Lab2A, IDLE_SIG);
		}




// 			Useful for Debugging, and understanding your Microblaze registers.
//    		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
//    	    u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
//    	    u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
//
//    	    u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
//    	    u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
//    	    u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
//    	    u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
//    	    u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
//    	    u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
//    	    u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
//    	    u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
//
//    	    // Expect to see 0x00000001
//    	    u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
//    	    // Expect to see 0x00000001
//    	    u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003;
//
//    	    // Expect to see 0x80000000 in GIER
//    		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000;


    }
}

/* Do not touch Q_onAssert */
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    (void)file;                                   /* avoid compiler warning */
    (void)line;                                   /* avoid compiler warning */
    QF_INT_LOCK();
    for (;;) {
    }
}

/* Interrupt handler functions here.  Do not forget to include them in lab2a.h!
To post an event from an ISR, use this template:
QActive_postISR((QActive *)&AO_Lab2A, SIGNALHERE);
Where the Signals are defined in lab2a.h  */

/******************************************************************************
*
* This is the interrupt handler routine for the GPIO for this example.
*
******************************************************************************/
void BtnHandler(void *CallbackRef) {
	XGpio_InterruptClear(&btn, 1);

	u32 data = XGpio_DiscreteRead(&btn, 1);

	if (data == 1) { //octave button
		QActive_postISR((QActive *)&AO_Lab2A, OCTAVE_CLICK);
	}

	else if (data == 2) { //A4 button
		QActive_postISR((QActive *)&AO_Lab2A, A4_CLICK);
	}

	else if (data == 4) { //set button
		QActive_postISR((QActive *)&AO_Lab2A, SET_CLICK);
	}
}

void GpioHandler(void *CallbackRef) {
	// Increment A counter
}

void TwistHandler(void *CallbackRef) {
	//XGpio_DiscreteRead( &twist_Gpio, 1);
	XGpio_InterruptClear(&encoder, 1);

	u32 data = XGpio_DiscreteRead( &encoder, 1);

	switch (state) {
	case (0):
			switch (data) {
			case (1):
					state = 1;
					break;
			case (2):
					state = 2;
					break;
			case (3):
					state = 0;
					break;
			}
			break;
	case (1):
			switch (data) {
			case (0):
					state = 3;
					break;
			case (1):
					state = 1;
					break;
			case (3):
					state = 0;
					break;
			}
			break;
	case (2):
			switch (data) {
			case (0):
					state = 4;
					break;
			case (2):
					state = 2;
					break;
			case (3):
					state = 0;
					break;
			}
			break;
	case (3):
			switch (data) {
			case (0):
					state = 3;
					break;
			case (1):
					state = 1;
					break;
			case (2):
					state = 5;
					break;
			}
			break;
	case (4):
			switch (data) {
			case (0):
					state = 4;
					break;
			case (1):
					state = 6;
					break;
			case (2):
					state = 2;
					break;
			}
			break;
	case (5):
			switch (data) {
			case (0):
					state = 3;
					break;
			case (2):
					state = 5;
					break;
			case (3):
					state = 0;
					QActive_postISR((QActive *)&AO_Lab2A, ENCODER_UP);
					break;
			}
			break;
	case (6):
			switch (data) {
			case (0):
					state = 4;
					break;
			case (1):
					state = 6;
					break;
			case (3):
					state = 0;
					QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN);
					break;
			}
	}
}

void read_fsl_values(float* q, int n) {
	stream_grabber_start();
	stream_grabber_wait_enough_samples(n);

	fill_samples(q, n, m, incr, sample_size);
}

void fill_samples(float *q, int n, int m, int incr, int sample_size) {
	int i;
	for (i = 0; i < n/sample_size; i+=incr) {
		int avg = 0;
		int j = 0;

		for (j = 0; j < incr; j++) {
			avg += stream_grabber_read_sample(i + j);
		}

		avg = avg >> m;

		q[i >> m] = 3.3 * avg/67108864.0;
	}
}

void debounceTwistInterrupt(){
	// Read both lines here? What is twist[0] and twist[1]?
	// How can you use reading from the two GPIO twist input pins to figure out which way the twist is going?
}

void debounceInterrupt() {
	//QActive_postISR((QActive *)&AO_Lab2A, ENCODER_CLICK);
	// XGpio_InterruptClear(&sw_Gpio, GPIO_CHANNEL1); // (Example, need to fill in your own parameters
}
