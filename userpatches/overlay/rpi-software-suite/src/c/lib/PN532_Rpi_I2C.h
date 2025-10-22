#ifndef _PN532_RPI_I2C_H
#define _PN532_RPI_I2C_H

#include "pn532.h"

int PN532_Reset(void);
void PN532_Log(const char* log);


int PN532_I2C_Init(PN532* dev);
int PN532_I2C_ReadData(uint8_t* data, uint16_t count);
int PN532_I2C_WriteData(uint8_t *data, uint16_t count);
bool PN532_I2C_WaitReady(uint32_t timeout);
int PN532_I2C_Wakeup(void);
void PN532_I2C_Close(void);


#endif 
