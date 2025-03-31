/*
 * EEPROM storage
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2022-2025
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#include "stm32f1xx_hal.h"

#define SETTINGS_MAGIC 0xAA55000A

struct eeprom
{
	I2C_HandleTypeDef *i2c;
	uint8_t dev_addr;
	uint8_t mem_addr;
};

typedef struct
{
	uint32_t magic;
	uint32_t period;
	uint32_t oa0;
	uint32_t oa1;
	uint32_t oa2;
} settings_t;


int eeprom_init(struct eeprom *eeprom, I2C_HandleTypeDef *i2c, uint8_t dev_addr,
		uint8_t mem_addr);
int eeprom_get(struct eeprom *eeprom, settings_t *settings);
int eeprom_set(struct eeprom *eeprom, settings_t *settings);

#endif /* EEPROM_H_ */
