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
	APP_TASK_TIMER_PRIO,	
  APP_TASK_LED1_PRIO,
  APP_TASK_LED2_PRIO,	
	APP_TASK_LCD_PRIO
} taskPriorities_t;

/*
*********************************************************************************************************
*                                            APPLICATION TASK STACKS
*********************************************************************************************************
*/

#define  APP_TASK_BUTTONS_STK_SIZE           256
#define  APP_TASK_ACC_STK_SIZE               256
#define  APP_TASK_POT_STK_SIZE               256
#define  APP_TASK_TIMER_STK_SIZE             256
#define  APP_TASK_LED1_STK_SIZE              256
#define  APP_TASK_LED2_STK_SIZE              256
#define  APP_TASK_LCD_STK_SIZE               256

static OS_STK appTaskButtonsStk[APP_TASK_BUTTONS_STK_SIZE];
static OS_STK appTaskAccStk[APP_TASK_ACC_STK_SIZE];
static OS_STK appTaskPotStk[APP_TASK_POT_STK_SIZE];
static OS_STK appTaskTimerStk[APP_TASK_TIMER_STK_SIZE];
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
static void appTaskTimer(void *pdata);
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
	RB_TIMER,
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

//SysTick Timer
typedef enum {
	TIMER_TASK,POT_TIMER
} taskNames_t;


enum {
	FLASH_MIN_DELAY     = 1,
	FLASH_INITIAL_DELAY = 500,
	FLASH_MAX_DELAY     = 1000,
	FLASH_DELAY_STEP    = 50
};

struct caseStatus {
	int locked;
	int armed;
	int codePointer;
	int codeNum[4];
};

static bool buttonPressedAndReleased(buttonId_t button);
static void barChart(float value);

void sysTickHandler(void);
void updateCase(struct bcStatus *bcStatus);


/*
*********************************************************************************************************
*                                            PIN DEFINITIONS
*********************************************************************************************************
*/

static DigitalOut led1(P1_18);
static DigitalOut led2(P0_13);
static DigitalIn buttons[] = {P5_4, P5_0, P5_2, P5_1, P5_3}; // LEFT, RIGHT, UP, DOWN, CENTER
MMA7455 acc(P0_27, P0_28);
int32_t accVal[3];
static AnalogIn potentiometer(P0_23);
static Display *d = Display::theDisplay();
bool accInit(MMA7455& acc); //prototype of init routine

static bool flashing[2] = {false, false};
static int32_t flashingDelay[2] = {FLASH_INITIAL_DELAY, FLASH_INITIAL_DELAY};
//bcStatus_t bc;

/*
*********************************************************************************************************
*                                            GLOBAL FUNCTION DEFINITIONS
*********************************************************************************************************
*/

	int locked = 0;
	int armed = 0;
	int moved = 0;
	float potVal = 0.0;
	float timerVal = 0.0;
	int codePointer = 0;
	int codeNum[4] = {0,0,0,0};
	int codeNumX[4] = {258,276,294,312};
	int correctCode[4] = {0,1,2,3};
	char printMsg[64];

int main() {
	
	/* Initialise the Accelerometer */
	if (accInit(acc)) {
			d->setCursor(4,76);
			d->printf("Accelerometer initialised");
			//OSTimeDlyHMSM(0,0,10,0);
		} else {
			d->setCursor(4,76);
			d->printf("Could not initialise accelerometer");
			//OSTimeDlyHMSM(0,0,10,0);
		}	

	/* Initialise the display */	
	d->fillScreen(WHITE);
	d->setTextColor(BLACK, WHITE);
  d->setCursor(2, 2);
	d->printf("EN0572 Assignment                         Cameron Bennett & George Coldham");

  /* Initialise the OS */
  OSInit();
	
//Buffer Init
	void safeBufferInit(void);

	
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
							 
	OSTaskCreate(appTaskTimer,                               
               (void *)0,
               (OS_STK *)&appTaskTimerStk[APP_TASK_TIMER_STK_SIZE - 1],
               APP_TASK_TIMER_PRIO);

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

	//********************************
	
	//int i = 2;
  /* Task main loop */
  while (true) {
    if (buttonPressedAndReleased(JLEFT)) {			
			if (!armed) {
				if (locked) {
					msg.id = RB_LEFT;
					armed = 1;
					safeBufferPut(&msg);
				}
			}
			else if (armed) {
				msg.id = RB_LEFT;
				if (codePointer > 0) {
					codePointer = codePointer - 1;
				}
				else if (codePointer == 0) {
					codePointer = 3;
				}
				safeBufferPut(&msg);
			}
		}
		else if (buttonPressedAndReleased(JRIGHT)) {
			if (armed) {
				msg.id = RB_RIGHT;
				if (codePointer < 3) {
					codePointer = codePointer + 1;
				}
				else if (codePointer == 3) {
					codePointer = 0;
				}
				safeBufferPut(&msg);
			}
		}
		else if (buttonPressedAndReleased(JUP)) {
			
			if (!armed) { 
					if (!locked) {
						msg.id = RB_UP;
						locked = 1;						
						msg.data[0] = 0;
						safeBufferPut(&msg);
					}
				}
			else if (armed) {
				msg.id = RB_UP;				
				msg.data[0] = 1;		
				if (codeNum[codePointer] == 9) {
					codeNum[codePointer] = 0;
				}
				else {
					codeNum[codePointer] = (codeNum[codePointer] + 1);
				}	
				
				msg.data[1] = codePointer;
				msg.data[2] = codeNum[codePointer];
				
				safeBufferPut(&msg);
			}
		}
		else if (buttonPressedAndReleased(JDOWN)) {			
			if (!armed) {
				if(locked) {
					msg.id = RB_DOWN;	
					locked = !locked;	
					msg.data[0] = 0;
					safeBufferPut(&msg);
				}
			}
			else if (armed) {
				msg.id = RB_DOWN;				
				msg.data[0] = 1;		
				if (codeNum[codePointer] == 0) {
					codeNum[codePointer] = 9;
				}
				else {
					codeNum[codePointer] = (codeNum[codePointer] - 1);
				}	
				
				msg.data[1] = codePointer;
				msg.data[2] = codeNum[codePointer];
				
				safeBufferPut(&msg);
			}
		}
		else if (buttonPressedAndReleased(JCENTER)) {
			if (armed) {
				if ((correctCode[0] == codeNum[0]) && (correctCode[1] == codeNum[1]) && (correctCode[2] == codeNum[2]) && (correctCode[3] == codeNum[3])) {
					msg.id = RB_CENTER;
					armed = 0;
					locked = 0;
					moved = 0;
					codeNum[0] = 0;
					codeNum[1] = 0;
					codeNum[2] = 0;
					codeNum[3] = 0;
					codePointer = 0;
					safeBufferPut(&msg);
				}
			}
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
	message_t msg;
  while (true) {
		if(!armed) {
			potVal = 1.00F - potentiometer.read();
			msg.id = RB_POT;
			msg.fdata[0] = potVal;
			potVal = potVal * 1.2F;
			potVal = potVal	* 100;
			if (potVal < 10) {
				potVal = 10;
			}
			msg.fdata[1] = potVal;
			safeBufferPut(&msg);
		}
		OSTimeDlyHMSM(0,0,0,10);		
  }
}

static void appTaskTimer(void *pdata) {
	message_t msg;
	while (true) {
			if (armed) {
				if (moved) {
					if (potVal == 0) {
						msg.id = RB_TIMER;
						msg.data[0] = 0;
						safeBufferPut (&msg);
					}
					else if (potVal > 0) {
						msg.id = RB_TIMER;
						msg.data[0] = 1;
						potVal = potVal - 1;		
						safeBufferPut(&msg);
					}
				}
		}
		OSTimeDlyHMSM(0,0,1,0);
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
	
	/* Initialise the display */	
	d->fillScreen(WHITE);
	d->setTextColor(BLACK, WHITE);
  d->setCursor(4, 4);
	d->printf("EN0572 Assignment                             Cameron Bennett & George Coldham");
	d->drawRect(281, 36, 102, 10, BLACK);
	d->drawRect(2, 14, 476, 256, BLACK);
	d->setCursor(180, 24);
	d->printf("ALARM     :  PENDING");
	d->setCursor(180, 49);
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
					d->setCursor(180, 38);				
					d->printf("INTERVAL  : %3.0f\n", msg.fdata[1]);	
					barChart(msg.fdata[0]);				
				break;
			}
			case RB_TIMER : {				
				if (msg.data[0] == 0) {
					d->setCursor(150, 130);
					d->printf("ENTER CODE TO UNLOCK AND DISARM DEVICE!");
					flashing[0] = 1;
					flashing[1] = 1;
				}
				else if (msg.data[0] == 1) {
					d->setCursor(240, 49);
					d->printf(": %3.0f\n", potVal);
				}				
				break;
			}
			case RB_ACC : {
				float accVal0;
				float accVal1;
				float accVal2;
				
				if (armed) {
					if((msg.fdata[0] > (accVal0 - 7)) && (msg.fdata[0] < (accVal0 + 7))) {
						d->setCursor(2,2);
						d->printf("No Change");
					}
					else if((msg.fdata[1] > (accVal1 - 7)) && (msg.fdata[1] < (accVal1 + 7))) {
						d->setCursor(2,2);
						d->printf("No Change");
					}
					else if((msg.fdata[2] > (accVal2 - 7)) && (msg.fdata[2] < (accVal2 + 7))) {
						d->setCursor(2,2);
						d->printf("No Change");
					}
					else {
						accVal0 = msg.fdata[0];
						accVal1 = msg.fdata[1];
						accVal2 = msg.fdata[2];
						
						d->setCursor(2,2);
						d->setCursor(240, 72);
						d->printf(":  MOVED     ");
						
						moved = 1;
					}
				}
				break;
			}
			case RB_UP : {
				if (msg.data[0] == 0) {
					d->setCursor(240,60);
					d->printf(":  LOCKED   ");
				}
				else if (msg.data[0] == 1) {
					switch (msg.data[1]) {
						case 0 : {
							d->setCursor(258, 96);
							d->printf("%i", msg.data[2]);
							break;
						}
						case 1 : {
							d->setCursor(276, 96);
							d->printf("%i", msg.data[2]);
							break;
						}
						case 2 : {
							d->setCursor(294, 96);
							d->printf("%i", msg.data[2]);
							break;
						}
						case 3 : {
							d->setCursor(312, 96);
							d->printf("%i", msg.data[2]);
							break;
						}
					}
				}
				break;
			}
			case RB_DOWN : {
				if (msg.data[0] == 0) {
					d->setCursor(240, 60);
					d->printf(":  UNLOCKED   ");
				}
				else if (msg.data[0] == 1) {
					switch (msg.data[1]) {
						case 0 : {
							d->setCursor(258, 96);
							d->printf("%i", msg.data[2]);
							break;
						}
						case 1 : {
							d->setCursor(276, 96);
							d->printf("%i", msg.data[2]);
							break;
						}
						case 2 : {
							d->setCursor(294, 96);
							d->printf("%i", msg.data[2]);
							break;
						}
						case 3 : {
							d->setCursor(312, 96);
							d->printf("%i", msg.data[2]);
							break;
						}
					}
				}
				break;
			}
			case RB_LEFT : {
				if (msg.data[0] == 0) {
						d->setCursor(240, 24);
						d->printf(":  ARMED    ");
				}
				else if (msg.data[0] == 1) {
					
				}
				break;
			}
			case RB_RIGHT : {
				
				break;
			}
			case RB_CENTER : {
				d->setCursor(240, 24);
				d->printf(":  UNARMED    ");
				d->setCursor(240, 60);
				d->printf(":  UNLOCKED   ");			
				d->setCursor(240, 72);
				d->printf(":  STATIONARY");
				d->setCursor(150, 130);
				d->printf("                                                 ");
				
				flashing[0] = 0;
				flashing[1] = 0;
				
				d->setCursor(258, 96);
				d->printf("0");
				d->setCursor(276, 96);
				d->printf("0");
				d->setCursor(294, 96);
				d->printf("0");
				d->setCursor(312, 96);
				d->printf("0");			
				
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


/*void incDelay(void) {
	if (flashingDelay[0] + FLASH_DELAY_STEP > FLASH_MAX_DELAY) {
		flashingDelay[0] = FLASH_MAX_DELAY;
	}
	else {
		flashingDelay[0] += FLASH_DELAY_STEP;
	}
}*/

/*void decDelay(void) {
	if (flashingDelay[0] - FLASH_DELAY_STEP < FLASH_MIN_DELAY) {
		flashingDelay[0] = FLASH_MIN_DELAY;
	}
	else {
		flashingDelay[0] -= FLASH_DELAY_STEP;
	}
}*/

static void barChart(float value) {
	uint16_t const max = 100;
	uint16_t const left = 282;
	uint16_t const top = 37;
	uint16_t const width = int(value * max);
	uint16_t const height = 8; 
	
	d->fillRect(left, top, width, height, RED);
	d->fillRect(left + width, top, max - width, height, WHITE);
}
