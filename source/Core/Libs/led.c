/*
 * LED
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2022-2025
 */

#include "led.h"

#include <string.h>

#define CNT 8
#define CNT_MASK (CNT - 1)
#define MAX_BLINKS 3

static const uint8_t blinks[4][8] = {
		{0, 0, 0, 0, 0, 0, 0, 0},
		{1, 0, 0, 0, 0, 0, 0, 0},
		{1, 0, 1, 0, 0, 0, 0, 0},
		{1, 0, 1, 0, 1, 0, 0, 0}};


/******************************************************************************/
int led_init(struct led *led, GPIO_TypeDef *port, uint16_t pin)
{
	if (!led || !port)
		return -1;

	memset(led, 0, sizeof(*led));
	led->port = port;
	led->pin = pin;
	led->cnt = 0;
	led->num = 0;

	return 0;
}

/******************************************************************************/
void led_irq(struct led *led)
{
	led->cnt = (led->cnt + 1) & CNT_MASK;
	HAL_GPIO_WritePin(led->port, led->pin, blinks[led->num][led->cnt]);
}

/******************************************************************************/
void led(struct led *led, int num)
{
	if (num <= MAX_BLINKS)
		led->num = num;
}
