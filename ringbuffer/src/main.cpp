#include <stdbool.h>
#include <ucos_ii.h>
#include <mbed.h>
#include <display.h>
#include "buffer.h"

/*
*********************************************************************************************************
*                                            APPLICATION TASK PRIORITIES
*********************************************************************************************************
*/

typedef enum {
	APP_TASK_BUTTONS_PRIO = 4,
	APP_TASK_POT_PRIO,
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
#define  APP_TASK_POT_STK_SIZE               256
#define  APP_TASK_LED1_STK_SIZE              256
#define  APP_TASK_LED2_STK_SIZE              256
#define  APP_TASK_LCD_STK_SIZE               256

static OS_STK appTaskButtonsStk[APP_TASK_BUTTONS_STK_SIZE];
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
	RB_POT
} deviceNames_t;

typedef enum {
	JLEFT = 0,
	JRIGHT,
	JUP,
	JDOWN
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
static DigitalIn buttons[] = {P5_0, P5_4, P5_2, P5_1}; // LEFT, RIGHT, UP, DOWN
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

  /* Initialise the OS */
  OSInit();                                                   

  /* Create the tasks */
  OSTaskCreate(appTaskButtons,                               
               (void *)0,
               (OS_STK *)&appTaskButtonsStk[APP_TASK_BUTTONS_STK_SIZE - 1],
               APP_TASK_BUTTONS_PRIO);
  
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
               (OS_STK *)&appTaskLCDStk[APP_TASK_LED2_STK_SIZE - 1],
               APP_TASK_LCD_PRIO);
							 
  safeBufferInit();
							 
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
  
  /* Task main loop */
  while (true) {
    if (buttonPressedAndReleased(JLEFT)) {
			flashing[0] = !flashing[0];
		}
		else if (buttonPressedAndReleased(JRIGHT)) {
			flashing[1] = !flashing[1];
		}
		else if (flashing[0] && buttonPressedAndReleased(JUP)) {
			decDelay();
		}
		else if (flashing[0] && buttonPressedAndReleased(JDOWN)) {
			incDelay();
		}
    OSTimeDlyHMSM(0,0,0,100);
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
    OSTimeDlyHMSM(0,0,0,200);
  }
}

static void appTaskLED1(void *pdata) {
	message_t msg;
	
  while (true) {
		if (flashing[0]) {
      led1 = !led1;
		}
		msg.id = RB_LED1;
		msg.data[0] = flashing[0];
		msg.data[1] = flashingDelay[0];
		safeBufferPut(&msg);
    OSTimeDly(flashingDelay[0]);
  }
}


static void appTaskLED2(void *pdata) {
	message_t msg;
	
  while (true) {
		if (flashing[1]) {
      led2 = !led2;
		}
		msg.id = RB_LED2;
		msg.data[0] = flashing[1];
		msg.data[1] = flashingDelay[1];
		safeBufferPut(&msg);
    OSTimeDly(flashingDelay[1]);
  } 
}

static void appTaskLCD(void *pdata) {
	message_t msg;
	
	/* Initialise the display */	
	d->fillScreen(WHITE);
	d->setTextColor(BLACK, WHITE);
  d->setCursor(2, 2);
	d->printf("EN0572 Lab 08");
	d->drawRect(109, 12, 102, 10, BLACK);
	
	while (true) {
		safeBufferGet(&msg);
		switch (msg.id) {
			case RB_LED1 : {
 		    d->setCursor(2,42);
		    d->printf("(LED1) F: %s, D: %04d", msg.data[0] ? " ON" : "OFF", msg.data[1]);
				break;
			}
			case RB_LED2 : {
 		    d->setCursor(2,52);
		    d->printf("(LED2) F: %s, D: %04d", msg.data[0] ? " ON" : "OFF", msg.data[1]);
				break;
			}
			case RB_POT : {
        d->setCursor(2, 12);
		    d->printf("Pot value : %1.2f\n", msg.fdata[0]);	
        barChart(msg.fdata[0]);		
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
	static uint32_t savedState[4] = {1,1,1,1};
	
	state = buttons[b].read();
  if ((savedState[b] == 0) && (state == 1)) {
		result = true;
	}
	savedState[b] = state;
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
	uint16_t const left = 110;
	uint16_t const top = 13;
	uint16_t const width = int(value * max);
	uint16_t const height = 8; 
	
	d->fillRect(left, top, width, height, RED);
	d->fillRect(left + width, top, max - width, height, WHITE);
}

