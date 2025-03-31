/*
 * EEPROM storage
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2022-2025
 */

#include "eeprom.h"

#include <string.h>

#define I2C_READY_TRIALS 32
#define I2C_TIMEOUT 100

#define WRITE_MAX_NUM 16


/******************************************************************************/
int eeprom_init(struct eeprom *eeprom, I2C_HandleTypeDef *i2c, uint8_t dev_addr,
		uint8_t mem_addr)
{
	settings_t settings;
	int ret;

	if (!eeprom)
		return -1;

	memset(eeprom, 0, sizeof(*eeprom));
	eeprom->i2c = i2c;
	eeprom->dev_addr = dev_addr;
	eeprom->mem_addr = mem_addr;

	ret = eeprom_get(eeprom, &settings);
	if (ret)
		return -1;

	// Default settings
	if (settings.magic != SETTINGS_MAGIC)
	{
		settings.magic = SETTINGS_MAGIC;
		settings.period = 10;
		settings.oa0 = 1000;
		settings.oa1 = 1000;
		settings.oa2 = 1000;

		ret = eeprom_set(eeprom, &settings);
		if (ret)
			return -1;
	}

	return 0;
}

static int get_data(struct eeprom *eeprom, uint8_t address, size_t len,
		uint8_t *buffer)
{
	HAL_StatusTypeDef status;

	status = HAL_I2C_Mem_Read(eeprom->i2c, eeprom->dev_addr, address,
			I2C_MEMADD_SIZE_8BIT, buffer, len, I2C_TIMEOUT);
	if (status != HAL_OK)
		return -1;

	status = HAL_I2C_IsDeviceReady(eeprom->i2c, eeprom->dev_addr,
			I2C_READY_TRIALS, I2C_TIMEOUT);
	if (status != HAL_OK)
		return -1;

	return 0;
}

static int set_data(struct eeprom *eeprom, uint8_t address, size_t len,
		uint8_t *buffer)
{
	HAL_StatusTypeDef status;
	size_t block;

	while (len)
	{
		block = len > WRITE_MAX_NUM ? WRITE_MAX_NUM : len;
		status = HAL_I2C_Mem_Write(eeprom->i2c, eeprom->dev_addr, address,
				I2C_MEMADD_SIZE_8BIT, buffer, block, I2C_TIMEOUT);
		if (status != HAL_OK)
			return -1;

		status = HAL_I2C_IsDeviceReady(eeprom->i2c, eeprom->dev_addr,
				I2C_READY_TRIALS, I2C_TIMEOUT);
		if (status != HAL_OK)
			return -1;

		buffer += block;
		address += block;
		len -= block;
	}

	return 0;
}

/******************************************************************************/
int eeprom_get(struct eeprom *eeprom, settings_t *settings)
{
	return get_data(eeprom, eeprom->mem_addr, sizeof(settings_t),
			(uint8_t *) settings);
}

/******************************************************************************/
int eeprom_set(struct eeprom *eeprom, settings_t *settings)
{
	return set_data(eeprom, eeprom->mem_addr, sizeof(settings_t),
			(uint8_t *) settings);
}
