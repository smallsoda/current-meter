//
//

#ifndef STATUS_LED_H_
#define STATUS_LED_H_

#include <stdint.h>

#define LED0_PORT GPIOB
#define LED0_PIN  GPIO_PIN_13
#define LED1_PORT GPIOB
#define LED1_PIN  GPIO_PIN_14


void status_led(uint8_t led, uint8_t num);
void status_led_callback(void);

#endif /* STATUS_LED_H_ */
