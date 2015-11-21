/*
 * @brief This program is a starting point for the briefcase assignment for EN0572.
 *        The project must be inside the EN0572 workspace in order to build correctly.
 *        A basic framework for a uCOS-II program is provided. There are some task declarations
 *        and some use of devices. These are not meant to be definitive. You should modify the
 *				program structure to suit your requirements. Some tasks may not be required. Other tasks
 *				may need to be added. You will certainly need to add some concurrency control.
 */


#include <stdbool.h>
#include <ucos_ii.h>
#include <mbed.h>
#include <MMA7455.h>
#include <display.h>
#include "buffer.h"

/*
*********************************************************************************************************
*                                            APPLICATION TASK PRIORITIES
*********************************************************************************************************
*/

typedef enum {
	APP_TASK_BUTTONS_PRIO = 4,
	APP_TASK_ACC_PRIO,
	APP_TASK_POT_PRIO,	
  APP_TASK_LED1_PRIO,
  APP_TASK_LED2_PRIO,	
	APP_TASK_LCD_PRIO,	
	
} taskPriorities_t;

/*
*********************************************************************************************************
*                                            APPLICATION TASK STACKS
*********************************************************************************************************
*/

#define  APP_TASK_BUTTONS_STK_SIZE           256
#define  APP_TASK_ACC_STK_SIZE               256
#define  APP_TASK_POT_STK_SIZE               256
#define  APP_TASK_LED1_STK_SIZE              256
#define  APP_TASK_LED2_STK_SIZE              256
#define  APP_TASK_LCD_STK_SIZE               256

static OS_STK appTaskButtonsStk[APP_TASK_BUTTONS_STK_SIZE];
static OS_STK appTaskAccStk[APP_TASK_ACC_STK_SIZE];
static OS_STK appTaskPotStk[APP_TASK_POT_STK_SIZE];
static OS_STK appTaskLED1Stk[APP_TASK_LED1_STK_SIZE];
static OS_STK appTaskLED2Stk[APP_TASK_LED2_STK_SIZE];
static OS_STK appTaskLCDStk[APP_TASK_LCD_STK_SIZE];

/*
*********************************************************************************************************
*                                            APPLICATION FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static void appTaskButtons(void *pdata);
static void appTaskAcc(void *pdata);
static void appTaskPot(void *pdata);
static void appTaskLED1(void *pdata);
static void appTaskLED2(void *pdata);
static void appTaskLCD(void *pdata);

/*
*********************************************************************************************************
*                                            GLOBAL TYPES AND VARIABLES 
*********************************************************************************************************
*/


typedef enum {
	RB_LED1 = 0,
	RB_LED2,
	RB_ACC,
	RB_POT,	
	RB_LEFT,
	RB_RIGHT,
	RB_UP,
	RB_DOWN,
	RB_CENTER
} deviceNames_t;

typedef enum {
	JLEFT = 0,
	JRIGHT,
	JUP,
	JDOWN,
	JCENTER
} buttonId_t;

enum {
	FLASH_MIN_DELAY     = 1,
	FLASH_INITIAL_DELAY = 500,
	FLASH_MAX_DELAY     = 1000,
	FLASH_DELAY_STEP    = 50
};

static bool buttonPressedAndReleased(buttonId_t button);
static void incDelay(void);
static void decDelay(void);
static void barChart(float value);

static DigitalOut led1(P1_18);
static DigitalOut led2(P0_13);
static DigitalIn buttons[] = {P5_4, P5_0, P5_2, P5_1, P5_3}; // LEFT, RIGHT, UP, DOWN, CENTER
MMA7455 acc(P0_27, P0_28);
bool accInit(MMA7455& acc); //prototype of init routine
int32_t accVal[3];
static AnalogIn potentiometer(P0_23);
static Display *d = Display::theDisplay();

static bool flashing[2] = {false, false};
static int32_t flashingDelay[2] = {FLASH_INITIAL_DELAY, FLASH_INITIAL_DELAY};

/*
*********************************************************************************************************
*                                            GLOBAL FUNCTION DEFINITIONS
*********************************************************************************************************
*/

int main() {

	/* Initialise the display */	
	d->fillScreen(WHITE);
	d->setTextColor(BLACK, WHITE);
  d->setCursor(2, 2);
	d->printf("EN0572 Assignment                         Cameron Bennett & George Coldham");

  /* Initialise the OS */
  OSInit();      
	
	/* Initialise the Accelerometer */
	if (accInit(acc)) {
			d->setCursor(4,76);
			d->printf("Accelerometer initialised");
		} else {
			d->setCursor(4,76);
			d->printf("Could not initialise accelerometer");
		}

  /* Create the tasks */
  OSTaskCreate(appTaskButtons,                               
               (void *)0,
               (OS_STK *)&appTaskButtonsStk[APP_TASK_BUTTONS_STK_SIZE - 1],
               APP_TASK_BUTTONS_PRIO);
							 
	OSTaskCreate(appTaskAcc,                               
               (void *)0,
               (OS_STK *)&appTaskAccStk[APP_TASK_ACC_STK_SIZE - 1],
               APP_TASK_ACC_PRIO);
  
  OSTaskCreate(appTaskPot,                               
               (void *)0,
               (OS_STK *)&appTaskPotStk[APP_TASK_POT_STK_SIZE - 1],
               APP_TASK_POT_PRIO);

	OSTaskCreate(appTaskLED1,                               
               (void *)0,
               (OS_STK *)&appTaskLED1Stk[APP_TASK_LED1_STK_SIZE - 1],
               APP_TASK_LED1_PRIO);
  
  OSTaskCreate(appTaskLED2,                               
               (void *)0,
               (OS_STK *)&appTaskLED2Stk[APP_TASK_LED2_STK_SIZE - 1],
               APP_TASK_LED2_PRIO);

	OSTaskCreate(appTaskLCD,                               
               (void *)0,
               (OS_STK *)&appTaskLCDStk[APP_TASK_LCD_STK_SIZE - 1],
               APP_TASK_LCD_PRIO);
  
  /* Start the OS */
  OSStart();                                                  
  
  /* Should never arrive here */ 
  return 0;      
}

/*
*********************************************************************************************************
*                                            APPLICATION TASK DEFINITIONS
*********************************************************************************************************
*/

static void appTaskButtons(void *pdata) {
  /* Start the OS ticker -- must be done in the highest priority task */
  SysTick_Config(SystemCoreClock / OS_TICKS_PER_SEC);
	
  message_t msg;
	
  /* Task main loop */
  while (true) {
    if (buttonPressedAndReleased(JLEFT)) {
			flashing[0] = !flashing[0];
			msg.id = RB_LEFT;
			safeBufferPut(&msg);
		}
		else if (buttonPressedAndReleased(JRIGHT)) {
			flashing[1] = !flashing[1];
			msg.id = RB_RIGHT;
			safeBufferPut(&msg);
		}
		else if (buttonPressedAndReleased(JUP)) {
			msg.id = RB_UP;
			safeBufferPut(&msg);
		}
		else if (flashing[0] && buttonPressedAndReleased(JDOWN)) {
			incDelay();
			msg.id = RB_DOWN;
			safeBufferPut(&msg);
		}
		else if (flashing[0] && buttonPressedAndReleased(JCENTER)) {
			incDelay();
			msg.id = RB_CENTER;
			safeBufferPut(&msg);
		}
    OSTimeDlyHMSM(0,0,0,100);
  }
}

static void appTaskAcc(void *pdata) {
	
			message_t msg;
	
		while (true) {
			acc.read(accVal[0], accVal[1], accVal[2]);
		
			msg.id = RB_ACC;
			msg.fdata[0] = accVal[0];
			msg.fdata[1] = accVal[1];
			msg.fdata[2] = accVal[2];
			safeBufferPut(&msg);
			OSTimeDlyHMSM(0,0,0,10);
		}
}

static void appTaskPot(void *pdata) {
	float potVal;
	message_t msg;
	
  while (true) {
		potVal = 1.0F - potentiometer.read();
		flashingDelay[1] = int(potVal * 1000);
		msg.id = RB_POT;
		msg.fdata[0] = potVal;
		safeBufferPut(&msg);
    OSTimeDlyHMSM(0,0,0,10);
  }
}

static void appTaskLED1(void *pdata) {
  while (true) {
		if (flashing[0]) {
      led1 = !led1;
		}
    OSTimeDly(flashingDelay[0]);
  }
}


static void appTaskLED2(void *pdata) {
  while (true) {
		if (flashing[1]) {
      led2 = !led2;
		}
    OSTimeDly(flashingDelay[1]);
  } 
}

static void appTaskLCD(void *pdata) {
	
	message_t msg;
	int arr[7] = {0,0,3,0,0,0,0}; 
	/*
	0 = (0)locked/(1)unlocked
	1 = (0)unarmed/(1)armed
	2 = which bit of password pointed at(bits: 3,4,5,6)
	3,4,5,6 = password numbers.
	*/
	/* Initialise the display */	
	d->fillScreen(WHITE);
	d->setTextColor(BLACK, WHITE);
  d->setCursor(4, 4);
	d->printf("EN0572 Assignment                             Cameron Bennett & George Coldham");
	d->drawRect(281, 36, 102, 10, BLACK);
	d->drawRect(2, 14, 476, 256, BLACK);
	d->setCursor(180, 24);
	d->printf("ALARM     :  PENDING");
	d->setCursor(180, 48);
	d->printf("TIME      :  0");
	d->setCursor(180, 60);
	d->printf("CASE      :  UNLOCKED");
	d->setCursor(240, 72);
	d->printf(":  STATIONARY");
	d->setCursor(180, 96);
	d->printf("CODE      :  0  0  0  0");
	d->setCursor(258, 108);
	d->printf("-  -  -  -");
	
	while (true) {
		safeBufferGet(&msg);
		switch (msg.id) {
			case RB_LED1 : {
 		    d->setCursor(4,44);
		    d->printf("(LED1) F: %s, D: %04d", msg.data[0] ? " ON" : "OFF", msg.data[1]);
				break;
			}
			case RB_LED2 : {
 		    d->setCursor(4,54);
		    d->printf("(LED2) F: %s, D: %04d", msg.data[0] ? " ON" : "OFF", msg.data[1]);
				break;
			}
			case RB_POT : {
				float tempPot = msg.fdata[0];
				tempPot = 100*(tempPot * 1.2F);
				
        d->setCursor(180, 38);				
		    d->printf("INTERVAL  : %3.0f\n", tempPot);	
        barChart(msg.fdata[0]);		
				break;
			}
			case RB_ACC : {
				float accVal0;
				float accVal1;
				float accVal2;
				if((msg.fdata[0] > (accVal0 - 5)) && (msg.fdata[0] < (accVal0 + 5))) {
					d->setCursor(2,2);
					d->printf("No Change");
				}
				else if((msg.fdata[1] > (accVal1 - 5)) && (msg.fdata[1] < (accVal1 + 5))) {
					d->setCursor(2,2);
					d->printf("No Change");
				}
				else if((msg.fdata[2] > (accVal2 - 5)) && (msg.fdata[2] < (accVal2 + 5))) {
					d->setCursor(2,2);
					d->printf("No Change");
				}
				else {
					accVal0 = msg.fdata[0];
					accVal1 = msg.fdata[1];
					accVal2 = msg.fdata[2];
					
					d->setCursor(2,2);
					d->printf("BIG Change!");
				}
				
				
        d->setCursor(4, 150);
		    d->printf("Acc = (%05d, %05d, %05d)", accVal[0], accVal[1], accVal[2]);
				break;
			}
			case RB_UP : {
        if (arr[1] == 0) { 
					if (arr[0] == 0) {
						arr[0] = 1;
						d->setCursor(240, 60);
						d->printf(":  LOCKED   ");
					}
				}
				else if (arr[1] == 1) {
					arr[arr[2]] = (arr[arr[2]] + 1);
					d->setCursor(258, 96);
					d->printf("%i",arr[arr[2]]);
				}
				break;
			}
			case RB_DOWN : {
        if ((arr[0] == 1) && (arr[1] == 0)) {
					arr[0] = 0;
					d->setCursor(240, 60);
					d->printf(":  UNLOCKED   ");
				}
				break;
			}
			case RB_LEFT : {
				if (arr[0] == 1) {
					arr[1] = 1;
					d->setCursor(240, 24);
					d->printf(":  ARMED    ");
				}
				break;
			}
			case RB_RIGHT : {
        d->setCursor(4, 94);
		    d->printf("RIGHT   ");
				break;
			}
			case RB_CENTER : {
        d->setCursor(4, 94);
		    d->printf("CENTER   ");
				break;
			}
			default : {
				break;
			}
		}
	}
}

/*
 * @brief buttonPressedAndReleased(button) tests to see if the button has
 *        been pressed then released.
 *        
 * @param button - the name of the button
 * @result - true if button pressed then released, otherwise false
 *
 * If the value of the button's pin is 0 then the button is being pressed,
 * just remember this in savedState.
 * If the value of the button's pin is 1 then the button is released, so
 * if the savedState of the button is 0, then the result is true, otherwise
 * the result is false.
 */
bool buttonPressedAndReleased(buttonId_t b) {
	bool result = false;
	uint32_t state;
	static uint32_t savedState[5] = {1,1,1,1,1};
	
	state = buttons[b].read();
  if ((savedState[b] == 0) && (state == 1)) {
		result = true;
	}
	savedState[b] = state;
	return result;
}

bool accInit(MMA7455& acc) {
  bool result = true;
  if (!acc.setMode(MMA7455::ModeMeasurement)) {
    // screen->printf("Unable to set mode for MMA7455!\n");
    result = false;
  }
  if (!acc.calibrate()) {
    // screen->printf("Failed to calibrate MMA7455!\n");
    result = false;
  }
  // screen->printf("MMA7455 initialised\n");
  return result;
}

void incDelay(void) {
	if (flashingDelay[0] + FLASH_DELAY_STEP > FLASH_MAX_DELAY) {
		flashingDelay[0] = FLASH_MAX_DELAY;
	}
	else {
		flashingDelay[0] += FLASH_DELAY_STEP;
	}
}

void decDelay(void) {
	if (flashingDelay[0] - FLASH_DELAY_STEP < FLASH_MIN_DELAY) {
		flashingDelay[0] = FLASH_MIN_DELAY;
	}
	else {
		flashingDelay[0] -= FLASH_DELAY_STEP;
	}
}

static void barChart(float value) {
	uint16_t const max = 100;
	uint16_t const left = 282;
	uint16_t const top = 37;
	uint16_t const width = int(value * max);
	uint16_t const height = 8; 
	
	d->fillRect(left, top, width, height, RED);
	d->fillRect(left + width, top, max - width, height, WHITE);
}

