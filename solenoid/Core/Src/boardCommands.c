#include "boardCommands.h"
#include "main.h"
#include "stm32g051xx.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>
#include <sys/_types.h>
#include "supportingFunctions.h"


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// Internal temperature sensor calibration parameters
#define TS_CAL1_TEMP    30.0f      // °C
#define TS_CAL2_TEMP    130.0f     // °C
#define TS_CAL1_ADDR    ((uint16_t *)0x1FFF75A8)
#define TS_CAL2_ADDR    ((uint16_t *)0x1FFF75CA)

char tx_buff[TX_BUFFER_SIZE];
int txidx, txcnt;
uint8_t TxCmdBusy;

char rx_buff[RX_BUFFER_SIZE];
int rxidx, rxcnt;
uint8_t commandReceived;


void fcn1(char *splitCmd[MAX_PARAMS], int countOfParams)
{
    strncpy(tx_buff, "fcn1\n", TX_BUFFER_SIZE);
    txcnt = rxcnt;
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
}

void fcn2(char *splitCmd[MAX_PARAMS], int countOfParams)
{

    if(countOfParams != 1){
        //TODO
        return;
    }
    int num = atoi(splitCmd[1]);

    txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "fcn2, %i\n", num);
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
}

static const Command commands[] = {
    { fcn1, "fcn1" },
    { fcn2, "fcn2" },
    { set, "set"},
    { open, "open"},
    { close, "close"},
    { help, "help"},
};

static const int command_count = sizeof(commands) / sizeof(commands[0]);

void USART_IRQ_Hnadler(void){
    uint32_t isrflags   = USART1->ISR;

    uint32_t errorflags;

    uint8_t data;

    /* If no error occurs */
    errorflags = (isrflags & (uint32_t)(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE | USART_ISR_RTOF));
    if (errorflags){
        // manage errors
    }
    
    // Rx Flag & Rx interrupts enabled
    if ((USART1->ISR & USART_ISR_RXNE_RXFNE) && (USART1->CR1 |= USART_CR1_RXNEIE_RXFNEIE)){
        data = USART1->RDR;
        // USART1->TDR = data; // Loopback
        rx_buff[rxidx] = data;
        if (data == '\n'){
            rx_buff[rxidx] = 0;
            commandReceived = 1;
        }
        rxidx++;
        rxcnt++;
        if (rxidx >= RX_BUFFER_SIZE){
            // TODO Handle buffer overflow
            commandReceived = 1;
        }
    }

    // Tx flag & Tx interrupts enabled
    if ((USART1->ISR & USART_ISR_TXE_TXFNF) && (USART1->CR1 & USART_CR1_TXEIE_TXFNFIE)){
        // Copy data from Tx buffer into Tx register until all data is transferred, then disable Tx interrupts
        USART1->TDR = tx_buff[txidx];
        txidx++;
        // Stop when there is no more data to transmit or when the enxt character is \0
        if ((txidx >= txcnt) || (tx_buff[txidx] == 0)){
            USART1->CR1 &= ~USART_CR1_TXEIE_TXFNFIE;    // Disable Tx interrupts
        }

    }

    USART1->ICR = -1; // Clear all interrupt flags, unsafe, but gets the job done
}
int splitCommand(char *str, char *splitCmd[MAX_PARAMS]) {
    char *separators = " \t,;\n\r";

    // Sets the counters
    int i = 0;              // Counts chars in str
    int paramsCount = 0;    // Counts the number of parameters found

    // Skips all separators found at the start of the input string
    while(isSeparator(str[i], separators) && i<RX_BUFFER_SIZE) i++;

    // Check if there is any command, if not returns -1 to signal an error
    if (!str[i] || i>=RX_BUFFER_SIZE) return -1;

    // Sets first element of splitCmd to a pointer to the real start of the input string
    splitCmd[0] = str + i;

    while(str[i] && i<RX_BUFFER_SIZE && paramsCount<MAX_PARAMS) { // Loops until it goes out of bounds or finds a null terminator or finds the maximum number of parameters
        if(isSeparator(str[i], separators)) {
            str[i] = '\0'; // Replaces the first separator encountered with null terminator

            // Skips all following separators until it finds the next parameter or goes out of bounds
            do {
                i++;
            } while (isSeparator(str[i], separators) && i<RX_BUFFER_SIZE);

            // Breaks if it finds a null terminator or goes out of bounds
            if (!str[i] || i>=RX_BUFFER_SIZE) break;

            paramsCount++;
            splitCmd[paramsCount] = str + i; // Adds the next parameter pointer to params
        }
        i++;
    }
    return paramsCount;
}


void processCommand(void){

    char *splitCmd[MAX_PARAMS] = {0};

    if(commandReceived){
        int numOfParams = splitCommand(rx_buff, splitCmd);
        if(numOfParams == -1){
            // TODO: handle no command error
            rxcnt = 0;
            rxidx = 0;
            commandReceived = 0;
            return;
        }
        for(int i = 0; i < command_count; i++){
            if(strcmp(splitCmd[0], commands[i].name) == 0){
                commands[i].cmd(splitCmd, numOfParams);
                return;
            }
        }

        
        help(NULL, 0);
        rxcnt = 0;
        rxidx = 0;
        commandReceived = 0;
    }

}

void help(char *splitCmd[MAX_PARAMS], int countOfParams){
    const char *help_msg = "Valid commands:\nhelp, set, open, close\n";
    txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, help_msg);
    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
}


void set(char *splitCmd[MAX_PARAMS], int countOfParams){
    if(countOfParams != 2){
        txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "E: Incorrect number of parameters (required 2, got %d)\n", countOfParams);
        USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
        txidx = 0;
        rxcnt = 0;
        rxidx = 0;
        commandReceived = 0;
        return;
    }
    
    uint16_t selector = atoi(splitCmd[1]);
    uint16_t value = atoi(splitCmd[2]);

    if ((value<0) || (value>=0xffff)){
        txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "Parameter 2 out of range, must be [0,65535] (is %d)\n", value);
        USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
        txidx = 0;
        rxcnt = 0;
        rxidx = 0;
        commandReceived = 0;
        return;
    }

    if(selector == 1){
        TIM1->CCR1 = value;
    } else if(selector == 2){
        TIM1->CCR2 = value;
    } else if(selector == 3){
        TIM1->CCR3 = value;
    } else if(selector == 4){
        TIM1->CCR4 = value;
    } else if(selector == 5){
        TIM2->CCR1 = value;
    }else if(selector == 6){
        TIM2->CCR2 = value;
    }else if(selector == 7){
        TIM2->CCR3 = value;
    }else if(selector == 8){
        TIM2->CCR4 = value;
    } else {
        txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "E: Invalid parameter 1 (must [1,8], got %d)\n", selector);
        USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
        txidx = 0;
        rxcnt = 0;
        rxidx = 0;
        commandReceived = 0;
        return;
    }
    txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "Set channle %d to %d\n", selector, value);

    
    
    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
}

void open(char *splitCmd[MAX_PARAMS], int countOfParams){
    if(countOfParams != 1){
        txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "E: Incorrect number of parameters (required 1, got %d)\n", countOfParams);
        USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
        txidx = 0;
        rxcnt = 0;
        rxidx = 0;
        commandReceived = 0;
        return;
    }
    
    uint16_t selector = atoi(splitCmd[1]);

    uint32_t t = 500;

    // TODO: must be tested
    if(selector == 1){
        TIM1->CCR1 = 0xffff;
        Ton1 = HAL_GetTick() + t;
    } else if(selector == 2){
        TIM1->CCR2 = 0xffff;
        Ton2 = HAL_GetTick() + t;
    } else if(selector == 3){
        TIM1->CCR3 = 0xffff;
        Ton3 = HAL_GetTick() + t;
    } else if(selector == 4){
        TIM1->CCR4 = 0xffff;
        Ton4 = HAL_GetTick() + t;
    } else if(selector == 5){
        TIM2->CCR1 = 0xffff;
        Ton5 = HAL_GetTick() + t;
    }else if(selector == 6){
        TIM2->CCR2 = 0xffff;
        Ton6 = HAL_GetTick() + t;
    }else if(selector == 7){
        TIM2->CCR3 = 0xffff;
        Ton7 = HAL_GetTick() + t;
    }else if(selector == 8){
        TIM2->CCR4 = 0xffff;
        Ton8 = HAL_GetTick() + t;
    } else {
        txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "E: Invalid parameter 1 (must [1,8], got %d)\n", selector);
        USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
        txidx = 0;
        rxcnt = 0;
        rxidx = 0;
        commandReceived = 0;
        return;
    }
    txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "Opened valve %d\n", selector);

    
    
    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
}
void close(char *splitCmd[MAX_PARAMS], int countOfParams){
    if(countOfParams != 1){
        txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "E: Incorrect number of parameters (required 1, got %d)\n", countOfParams);
        USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
        txidx = 0;
        rxcnt = 0;
        rxidx = 0;
        commandReceived = 0;
        return;
    }
    
    uint16_t selector = atoi(splitCmd[1]);

    // TODO: must be tested
    if(selector == 1){
        TIM1->CCR1 = 0;
        Ton1 = 0;
    } else if(selector == 2){
        TIM1->CCR2 = 0;
        Ton2 = 0;
    } else if(selector == 3){
        TIM1->CCR3 = 0;
        Ton3 = 0;
    } else if(selector == 4){
        TIM1->CCR4 = 0;
        Ton4 = 0;
    } else if(selector == 5){
        TIM2->CCR1 = 0;
        Ton5 = 0;
    }else if(selector == 6){
        TIM2->CCR2 = 0;
        Ton6 = 0;
    }else if(selector == 7){
        TIM2->CCR3 = 0;
        Ton7 = 0;
    }else if(selector == 8){
        TIM2->CCR4 = 0;
        Ton8 = 0;
    } else {
        txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "E: Invalid parameter 1 (must [1,8], got %d)\n", selector);
        USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
        txidx = 0;
        rxcnt = 0;
        rxidx = 0;
        commandReceived = 0;
        return;
    }
    txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "Opened valve %d\n", selector);

    
    
    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
}