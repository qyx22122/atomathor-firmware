#ifndef BOARDCOMMANDS_H
#define BOARDCOMMANDS_H

#include <stdint.h>

#define TX_BUFFER_SIZE 64
#define RX_BUFFER_SIZE 64

typedef void (*CommandFunc)(const char *params);

typedef struct {
    CommandFunc cmd;
    const char *name;
} Command;

void USART_IRQ_Hnadler(void);

int findNextCommandParameter(const char *str);

void processCommand(void);

void readAdc(const char *params);
void checkPower(const char *params);
void readSensors(const char *params);
void readSensorsRaw(const char *params);

int readPirani(uint16_t readings[4]);

#endif