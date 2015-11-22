#ifndef __GPIOPIN_H__
#define __GPIOPIN_H__

#include <LPC407x_8x_177x_8x.h>

typedef struct {
	__IO uint32_t    *pcon;
	LPC_GPIO_TypeDef *port;
	uint32_t          mask;
} gpioPin_t;

typedef enum {
	INPUT_PIN,
	OUTPUT_PIN
} gpioPinDir_t;

/* Initialise the gpioPin_t data structure, given port number portN, 
 * pin number pinN, and whether input or output, dir.
 */
void gpioPinInit(gpioPin_t *pin, uint32_t portN, uint32_t pinN, gpioPinDir_t dir);
void gpioPinSet(gpioPin_t *pin);     /* make the value of the pin HIGH */
void gpioPinClr(gpioPin_t *pin);     /* make the value of the pin LOW */
void gpioPinToggle(gpioPin_t *pin);  /* if pin LOW -> HIGH, if pin HIGH -> LOW */
uint32_t gpioPinVal(gpioPin_t *pin); /* return the pin value: 0 if LOW, 1 if HIGH */

#endif

