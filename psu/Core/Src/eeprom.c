#include "eeprom.h"
#include "stm32g0xx_hal_i2c.h"
// https://www.st.com/resource/en/datasheet/m24c64-f.pdf

HAL_StatusTypeDef EEPROM_Read (struct eeprom eep, uint16_t address, uint16_t size, uint8_t *data, uint32_t Timeout){
    
    HAL_StatusTypeDef status;

    // // To read data, address is transmitted first, followed by an arbitryry number of reads. If the address isn't transmitted, last address is considered as start address.
    // status = HAL_I2C_Master_Transmit(eep.hi2c, eep.address, &address, eep.addressSize,  Timeout);
    // if (status) return status;

    // status = HAL_I2C_Master_Receive(eep.hi2c, eep.address, data, size, Timeout);
    status = HAL_I2C_Mem_Read(eep.hi2c, eep.address, address, eep.addressSize, data, size, Timeout);
    return status;
}
HAL_StatusTypeDef EEPROM_Write (struct eeprom eep, uint16_t address, uint16_t size, uint8_t *data, uint32_t Timeout){ 
    
    HAL_StatusTypeDef status;
    uint16_t bytesToWrite;

    // If the address points to the middle of a page, only data up to the end of this page may be written to.
    // Payload may be smaller than the payload, so handle this as well.
    bytesToWrite = eep.pageSize - (address % eep.pageSize);
    if (bytesToWrite > size) bytesToWrite = size;
    status = HAL_I2C_Mem_Write(eep.hi2c, eep.address, address, eep.addressSize, data, bytesToWrite, Timeout);
    data += bytesToWrite;
    size -= bytesToWrite;
    if (status) return status;

    // for the rest of the payload, write one page of bytes, or whatever remains to be written if that is less.
    while (size){
        bytesToWrite = size < eep.pageSize ? size : eep.pageSize;
        status = HAL_I2C_Mem_Write(eep.hi2c, eep.address, address, eep.addressSize, data, bytesToWrite, Timeout);
        data += bytesToWrite;
        size -= bytesToWrite;
        if (status) return status;
    }
    return status;
}