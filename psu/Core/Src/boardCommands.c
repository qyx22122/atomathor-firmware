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
#include "stm32g0xx_hal_dac.h"
#include "supportingFunctions.h"
#include "ratatoskr.h"

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

static const Command commands[] = {
    { ping, "ping"},
    { setDac, "setDac"},
    { readAdc, "readAdc"},
    { set, "set"},
    { help, "help"},
    { enable, "enable"},
    /* ... */
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

void processCommand(void){

    char *splitCmd[MAX_PARAMS] = {0};
    char msg[RX_BUFFER_SIZE] = {0};
    char ret[TX_BUFFER_SIZE] = {0};
    uint16_t msgID;
    char addr[] = "P1";

    char test[RX_BUFFER_SIZE] = {0};
    

    if(commandReceived){
        if(!parseMSG(rx_buff, rxcnt, addr, &msgID, msg)){
            rxcnt = 0;
            rxidx = 0;
            commandReceived = 0;
            return;
        }
        int numOfParams = splitCommand(msg, MAX_PARAMS, splitCmd, RX_BUFFER_SIZE);
        if(numOfParams == -1){
            // TODO: handle no command error
            rxcnt = 0;
            rxidx = 0;
            commandReceived = 0;
            return;
        }
        for(int i = 0; i < command_count; i++){
            if(strcmp(splitCmd[0], commands[i].name) == 0){
                commands[i].cmd(splitCmd, numOfParams, ret);
                txcnt = composeMSG(tx_buff, TX_BUFFER_SIZE, msgID, ret);
                USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
                txidx = 0;
                rxcnt = 0;
                rxidx = 0;
                commandReceived = 0;
                return;
            }
        }

        
        rxcnt = 0;
        rxidx = 0;
        commandReceived = 0;
    }

}

void help(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]){
    const char *help_msg = "Valid commands: help, set, enable, readAdc, setDac";
    strcpy(ret, help_msg);
}

void readAdc(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]){

    if(countOfParams != 1){
        snprintf(ret, TX_BUFFER_SIZE, "E: Incorrect number of parameters (required 1, got %d)", countOfParams);
        return;
    }
    
    if(strlen(splitCmd[1]) != 1){
        snprintf(ret, TX_BUFFER_SIZE, "E: Parameter 1 must only be one character (required 1, got %d)", strlen(splitCmd[1]));
        return;
    }

    char channel = splitCmd[1][0];


    uint8_t channelIdx = 0;

    // Make sure that channel is valid
    if ((channel == 'u') || (channel == 'U')){
        channel = 'u';
        channelIdx = 2;
    }
    else if ((channel == 'i') || (channel == 'I')){
        channel = 'i';
        channelIdx = 3;
    }
    else if ((channel == 't') || (channel == 'T')){
        channel = 't';
        channelIdx = 12;
    }
    else if ((channel == 'p') || (channel == 'P')){
        channel = 'p';
        channelIdx = 10;
    }
    else{
        snprintf(ret, TX_BUFFER_SIZE, "Chennel should be U, I, P, or T (got %c)", channel);
    }
    
    if (channelIdx) {
        HAL_StatusTypeDef status;
        uint16_t ADCmsmnt;
        float Vsns;

        ADC1->CHSELR = channelIdx << ADC_CHSELR_SQ1_Pos;
        status = HAL_ADC_Start(&hadc1);
        status |= HAL_ADC_PollForConversion(&hadc1, 10);
        ADCmsmnt = HAL_ADC_GetValue(&hadc1);
        // This error handling if iffy at best
        if (status){
            snprintf(ret, TX_BUFFER_SIZE, "Reading ADC failed with error 0x%x", status);
        }
        
        else{
            Vsns = ADCmsmnt*(2.5/(1<<16));
            // %f formatting can be enabled by adding "set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -u _printf_float")" to "cmake\gcc-arm-none-eabi.cmake", but it consumes 12 kB of FLASH
            if (channel == 'u'){
                // TODO add calibration and calculations
                snprintf(ret, TX_BUFFER_SIZE, "Vsns = %i.%0*i V", (int)Vsns, 2, (int)getDecimals(Vsns, 2));
            }
            else if (channel == 'i'){
                // TODO add calibration and calculations
                snprintf(ret, TX_BUFFER_SIZE, "Isns = %i.%0*i V", (int)Vsns, 2, (int)getDecimals(Vsns, 2));
            }
            if (channel == 'p'){
                // TODO add calibration and calculations
                Vsns = Vsns*11; // Vpwr passes through 100/10 resistor divider, this calculation ondoes that.
                snprintf(ret, TX_BUFFER_SIZE, "Main board supply is at %i.%0*i V", (int)Vsns, 2, (int)getDecimals(Vsns, 2));
            }
            else if (channel == 't'){
                float temperature;
                temperature = (Vsns - (*TS_CAL1_ADDR *3./4096.)) *
                (TS_CAL2_TEMP - TS_CAL1_TEMP) / 
                ((*TS_CAL2_ADDR - *TS_CAL1_ADDR)*3./4096.) + 
                TS_CAL1_TEMP;
                snprintf(ret, TX_BUFFER_SIZE, "MCU core temperature is %i.%0*i C", (int)temperature, 2, (int)getDecimals(temperature, 2));
            }
        }
    }
}
void setDac(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]){

    if(countOfParams != 1){
        snprintf(ret, TX_BUFFER_SIZE, "E: Incorrect number of parameters (required 1, got %d)", countOfParams);
        return;
    }
    int num = atoi(splitCmd[1]);
    
    // Sanity check for valid range
    if ((num<0) || (num>=(1<<12))){
        snprintf(ret, TX_BUFFER_SIZE, "Parameter 1 out of range, must be [0,4095] (is %i)", num);
    }
    else {
        HAL_StatusTypeDef status;
        status  = HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, num);
        status |= HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, num);
        // This error handling if iffy at best
        if (status){
            snprintf(ret, TX_BUFFER_SIZE, "Setting DAC returned an error (0x%x)", status);
        }
        else{
            snprintf(ret, TX_BUFFER_SIZE, "DACs set to %i", num);
        }
    }
}

void set(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]){
    if(countOfParams != 2){
        snprintf(ret, TX_BUFFER_SIZE, "E: Incorrect number of parameters (required 2, got %d)", countOfParams);
        return;
    }
    if(strlen(splitCmd[1]) != 1){
        snprintf(ret, TX_BUFFER_SIZE, "E: Parameter 1 must only be one character (required 1, got %d)", strlen(splitCmd[1]));
        return;
    }

    double value = atof(splitCmd[2]);
    char selector = toupper(splitCmd[1][0]);
    HAL_StatusTypeDef status;
    int32_t dacValue;
    uint32_t channel;
    char unit;
    // TODO: must be tested
    if(selector == 'U'){
        dacValue = value*4095/12; // Converts 0-12 -> 0-4095
        channel = DAC_CHANNEL_1;
        unit = 'V';
    } else if(selector == 'I'){
        dacValue = value*4095/166; // Converts 0-166 -> 0-4095
        channel = DAC_CHANNEL_2;
        unit = 'A';
    } else {
        snprintf(ret, TX_BUFFER_SIZE, "E: Invalid parameter 1 (must be U or I, got %c)", splitCmd[1][0]);
        return;
    }

    if ((dacValue<0) || (dacValue>=(1<<12))){
        snprintf(ret, TX_BUFFER_SIZE, "Parameter 2 out of range, must be [0,%d] (is %i.%0*i)", (selector == 'U')? 12 : 166, (int)value, 2, (int)getDecimals(value, 2));
        return;
    }

    status = HAL_DAC_SetValue(&hdac1, channel, DAC_ALIGN_12B_R, dacValue);

    if (status){
        snprintf(ret, TX_BUFFER_SIZE, "Setting DAC returned an error (0x%x)", status);
    } else {
        snprintf(ret, TX_BUFFER_SIZE, "DAC channal %c set to %ldPSU set to %c = %i.%0*i %c", selector, dacValue, selector, (int)value, 2, (int)getDecimals(value, 2), unit);
    }
    
    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
}

void enable(char *splitCmd[MAX_PARAMS], int countOfParams, char ret[TX_BUFFER_SIZE]){
   if(countOfParams != 1){
        snprintf(ret, TX_BUFFER_SIZE, "E: Incorrect number of parameters (required 1, got %d)", countOfParams);

        return;
    }
    
    if(!strcmp(splitCmd[1], "true")){
        LL_GPIO_ResetOutputPin(OUT_EN_GPIO_Port, OUT_EN_Pin);
        snprintf(ret, TX_BUFFER_SIZE, "Output enabled");
    } else if(!strcmp(splitCmd[1], "false")){
        LL_GPIO_SetOutputPin(OUT_EN_GPIO_Port, OUT_EN_Pin);
        snprintf(ret, TX_BUFFER_SIZE, "Output disabled");
    } else{
        snprintf(ret, TX_BUFFER_SIZE, "Parameter should be true or false (got %s)", splitCmd[1]);
    }

}
