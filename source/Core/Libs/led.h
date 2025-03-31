/*
 * LED
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2022-2025
 */

#ifndef LED_H_
#define LED_H_

#include "stm32f1xx_hal.h"

struct led
{
	GPIO_TypeDef *port;
	uint16_t pin;
	int cnt;
	int num;
};


int led_init(struct led *led, GPIO_TypeDef *port, uint16_t pin);
void led_irq(struct led *led);
void led(struct led *led, int num);

#endif /* LED_H_ */
