#ifndef BOARDCOMMANDS_H
#define BOARDCOMMANDS_H

#include <stdint.h>

#define TX_BUFFER_SIZE 64
#define RX_BUFFER_SIZE 64
#define MAX_PARAMS 8



void USART_IRQ_Hnadler(void);


void processCommand(void);

void readAdc(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]);
void checkPower(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]);
void readSensors(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]);
//void readSensorsRaw(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]); not needed

int readPirani(uint16_t readings[4]);

#endif