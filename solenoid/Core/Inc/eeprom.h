#ifndef EEPROM_H_
#define EEPROM_H_

#include "stm32g0xx_hal.h"

struct eeprom {
  I2C_HandleTypeDef * hi2c;
  uint8_t address;
  uint8_t addressSize;
  uint16_t pageSize;
};

HAL_StatusTypeDef EEPROM_Read (struct eeprom eep, uint16_t address, uint16_t size, uint8_t *data, uint32_t Timeout);
HAL_StatusTypeDef EEPROM_Write (struct eeprom eep, uint16_t address, uint16_t size, uint8_t *data, uint32_t Timeout);

#endif