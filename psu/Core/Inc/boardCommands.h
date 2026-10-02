#ifndef BOARDCOMMANDS_H
#define BOARDCOMMANDS_H

#define TX_BUFFER_SIZE 64
#define RX_BUFFER_SIZE 64
#define MAX_PARAMS 8

typedef void (*CommandFunc)(char *splitCmd[MAX_PARAMS], int countOfParams);

typedef struct {
    CommandFunc cmd;
    const char *name;
} Command;

void USART_IRQ_Hnadler(void);

int splitCommand(char *str, char *splitCmd[MAX_PARAMS]);

void processCommand(void);

void help(char *splitCmd[MAX_PARAMS], int countOfParams);
void readAdc(char *splitCmd[MAX_PARAMS], int countOfParams);
void setDac(char *splitCmd[MAX_PARAMS], int countOfParams);
void set(char *splitCmd[MAX_PARAMS], int countOfParams);
void enable(char *splitCmd[MAX_PARAMS], int countOfParams);

#endif