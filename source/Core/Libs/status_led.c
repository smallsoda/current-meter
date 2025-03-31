//
//

#include "status_led.h"

#include "stm32f1xx_hal.h"


uint8_t blinks_cnt = 0;
uint8_t blinks_num[2] = {0, 0};
uint8_t blinks[4][8] = {
		{0, 0, 0, 0, 0, 0, 0, 0},
		{1, 0, 0, 0, 0, 0, 0, 0},
		{1, 0, 1, 0, 0, 0, 0, 0},
		{1, 0, 1, 0, 1, 0, 0, 0}};


// @brief Set number of status LED blinks
//
void status_led(uint8_t led, uint8_t num)
{
	if((led < 2) && (num < 4))
		blinks_num[led] = num;
}


// @brief Status LED callback
//
void status_led_callback(void)
{
	blinks_cnt = (blinks_cnt + 1) & 0x07;

	if(blinks[blinks_num[0]][blinks_cnt])
		HAL_GPIO_WritePin(LED0_PORT, LED0_PIN, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(LED0_PORT, LED0_PIN, GPIO_PIN_RESET);

	if(blinks[blinks_num[1]][blinks_cnt])
		HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);
}
