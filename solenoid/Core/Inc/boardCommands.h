#ifndef BOARDCOMMANDS_H
#define BOARDCOMMANDS_H

#define TX_BUFFER_SIZE 64
#define RX_BUFFER_SIZE 64
#define MAX_PARAMS 8


void USART_IRQ_Hnadler(void);

void processCommand(void);

void help(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]);
void set(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]);
void open(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]);
void close(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]);

#endif