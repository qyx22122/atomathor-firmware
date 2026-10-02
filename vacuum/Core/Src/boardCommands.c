#include "boardCommands.h"
#include "main.h"
#include "stm32g051xx.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stm32g0xx_hal.h"
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


void fcn1(const char *params)
{
    strncpy(tx_buff, "fcn1", TX_BUFFER_SIZE);
    txcnt = rxcnt;
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
}

void fcn2(const char *params)
{
    int nextParam;
    nextParam = findNextCommandParameter(params);
    int num = atoi(params+nextParam);
    //strncpy(tx_buff, "fcn1", TX_BUFFER_SIZE);
    txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "fcn2, %i\n", num);
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
}

static const Command commands[] = {
    { fcn1, "fn1" },
    { fcn2, "fn2" },
    { readAdc, "readAdc"},
    { checkPower, "pwr"},
    { readSensors, "sns"},
    { readSensorsRaw, "snsRaw"},
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

int findNextCommandParameter(const char *str){
    // Assumes null termination, will work weird otherwise
    char *separators = " \t,;";

    // If the first character is a whitespace, the code must skip over to the first non-white space character
    uint8_t startedOnWhitespace = 0;

    // int lastSeparator = 0;
    // Go through maximum command size of characters (but stop if 0 is reached)
    for(int i=0; i<RX_BUFFER_SIZE; i++){
        if (str[i] == 0){
            return 0;
        }
        // if the current character is whitespace, return that index unless the code stareted on whitespace
        for(int j=0; j<(sizeof(separators)/sizeof(separators[0])-1); j++){  //-1 in count calculation to ellimiante \0
            if (str[i] == separators[j]){
                // lastSeparator = i;
                // break;
                if (i==0){
                    startedOnWhitespace = 1;    // Although it might be more efficient to do this check in advance, this way we avoid code duplication
                }
                if (startedOnWhitespace){
                    break;
                }
                return i;
            }
        }
        if (startedOnWhitespace && (i != 0)){
            return i;
        }
        // return i;
    }
    return 0;
}

void processCommand(void){
    // if (commandReceived){
    //     // loopback
    //     strncpy(tx_buff, rx_buff, rxcnt);
    //     txcnt = rxcnt;
    //     txidx = 0;
    //     rxcnt = 0;
    //     rxidx = 0;
    //     commandReceived = 0;
    //     USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
    // }
    int endOfCommand;
    int comparison;
    if (commandReceived){
        for (int i = 0; i < command_count; i++) {
            endOfCommand = findNextCommandParameter(rx_buff);
            if (endOfCommand){
                comparison = strncmp(commands[i].name, rx_buff, endOfCommand);
            }
            else{
                comparison = strcmp(commands[i].name, rx_buff);
            }
            if (comparison == 0) {
                commands[i].cmd(rx_buff + endOfCommand);
                return;
            }
        }
        // If no command match was found, do at least clear the buffer
        rxcnt = 0;
        rxidx = 0;
        commandReceived = 0;
    }
}

void readAdc(const char *params){
    int nextParam;
    nextParam = findNextCommandParameter(params);
    //int num = atoi(params+nextParam);
    char channel = params[nextParam];
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
        txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "Chennel should be U, I, P, or T (is %c)\n", channel);
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
            txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "Reading ADC failed with error 0x%x\n", status);
        }
        
        else{
            Vsns = ADCmsmnt*(2.5/(1<<16));
            // %f formatting can be enabled by adding "set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -u _printf_float")" to "cmake\gcc-arm-none-eabi.cmake", but it consumes 12 kB of FLASH
            if (channel == 'u'){
                // TODO add calibration and calculations
                txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "Vsns = %i.%0*i V\n", (int)Vsns, 2, (int)getDecimals(Vsns, 2));
            }
            else if (channel == 'i'){
                // TODO add calibration and calculations
                txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "Isns = %i.%0*i V\n", (int)Vsns, 2, (int)getDecimals(Vsns, 2));
            }
            if (channel == 'p'){
                // TODO add calibration and calculations
                Vsns = Vsns*11; // Vpwr passes through 100/10 resistor divider, this calculation ondoes that.
                txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "Main board supply is at %i.%0*i V\n", (int)Vsns, 2, (int)getDecimals(Vsns, 2));
            }
            else if (channel == 't'){
                float temperature;
                temperature = (Vsns - (*TS_CAL1_ADDR *3./4096.)) *
                (TS_CAL2_TEMP - TS_CAL1_TEMP) / 
                ((*TS_CAL2_ADDR - *TS_CAL1_ADDR)*3./4096.) + 
                TS_CAL1_TEMP;
                txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "MCU core temperature is %i.%0*i C\n", (int)temperature, 2, (int)getDecimals(temperature, 2));
            }
        }
    }

    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
}

void checkPower(const char *params){
    HAL_StatusTypeDef status = 0;
    uint16_t ADCmsmnt;
    float Vsns1, Vsns2;

    ADC1->CHSELR = 1 << ADC_CHSELR_SQ1_Pos; // Primary power is on ADC1
    status |= HAL_ADC_Start(&hadc1);
    status |= HAL_ADC_PollForConversion(&hadc1, 10);
    ADCmsmnt = HAL_ADC_GetValue(&hadc1);
    Vsns1 = ADCmsmnt*(2.5/(1<<16))*11;

    ADC1->CHSELR = 0 << ADC_CHSELR_SQ1_Pos; // Relay power is on ADC0
    status |= HAL_ADC_Start(&hadc1);
    status |= HAL_ADC_PollForConversion(&hadc1, 10);
    ADCmsmnt = HAL_ADC_GetValue(&hadc1);
    Vsns2 = ADCmsmnt*(2.5/(1<<16))*11;
    // This error handling if iffy at best
    if (status){
        txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "Reading ADC failed with error 0x%x\n", status);
    }
    
    txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "V1 = %i.%0*i V, V2 = %i.%0*i V\n", (int)Vsns1, 2, (int)getDecimals(Vsns1, 2), (int)Vsns2, 2, (int)getDecimals(Vsns2, 2));
    
    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
}

void readSensors(const char *params){
    HAL_StatusTypeDef status = 0;
    uint16_t ADCmsmnt;
    float Vsns1, Vsns2, id1, id2;

    // HAL_Delay(100);
    // ADC1->CHSELR = 6 << ADC_CHSELR_SQ1_Pos; // Primary power is on ADC1
    // status |= HAL_ADC_Start(&hadc1);
    // status |= HAL_ADC_PollForConversion(&hadc1, 10);
    // // ADCmsmnt = HAL_ADC_GetValue(&hadc1);
    // ADCmsmnt = ADC1->DR;
    // Vsns1 = ADCmsmnt*(2.5/(1<<16))*4;

    // HAL_Delay(100);
    // ADC1->CHSELR = 2 << ADC_CHSELR_SQ1_Pos; // Relay power is on ADC0
    // status |= HAL_ADC_Start(&hadc1);
    // status |= HAL_ADC_PollForConversion(&hadc1, 10);
    // // ADCmsmnt = HAL_ADC_GetValue(&hadc1);
    // ADCmsmnt = ADC1->DR;
    // Vsns2 = ADCmsmnt*(2.5/(1<<16))*4;

    // HAL_Delay(100);
    // ADC1->CHSELR = 7 << ADC_CHSELR_SQ1_Pos; // Relay power is on ADC0
    // status |= HAL_ADC_Start(&hadc1);
    // status |= HAL_ADC_PollForConversion(&hadc1, 10);
    // // ADCmsmnt = HAL_ADC_GetValue(&hadc1);
    // ADCmsmnt = ADC1->DR;
    // id1 = ADCmsmnt*(2.5/(1<<16));

    // HAL_Delay(100);
    // ADC1->CHSELR = 3 << ADC_CHSELR_SQ1_Pos; // Relay power is on ADC0
    // status |= HAL_ADC_Start(&hadc1);
    // status |= HAL_ADC_PollForConversion(&hadc1, 10);
    // // ADCmsmnt = HAL_ADC_GetValue(&hadc1);
    // ADCmsmnt = ADC1->DR;
    // id2 = ADCmsmnt*(2.5/(1<<16));
    uint16_t readings[4];
    readPirani(readings);

    Vsns1 = readings[0]*(2.5/(1<<16))*4;
    Vsns2 = readings[1]*(2.5/(1<<16))*4;
    id1 = readings[2]*(2.5/(1<<16));
        id1 = (id1*11-3.3)/(3.3-id1);   // sensed ID resistance in kOhm
    id2 = readings[3]*(2.5/(1<<16));
        id2 = (id2*11-3.3)/(3.3-id2);   // sensed ID resistance in kOhm

    // This error handling if iffy at best
    if (status){
        txcnt = snprintf(tx_buff, TX_BUFFER_SIZE, "Reading ADC failed with error 0x%x\n", status);
    }
    else{
        // txcnt = snprintf(tx_buff, TX_BUFFER_SIZE,  "id1 = %i.%0*i, V, V1 = %i.%0*i V, id2 = %i.%0*i, V2 = %i.%0*i V\n", 
        //                                             (int)id1, 2, (int)getDecimals(id1, 2),
        //                                             (int)Vsns1, 2, (int)getDecimals(Vsns1, 2),
        //                                             (int)id2, 2, (int)getDecimals(id2, 2),
        //                                             (int)Vsns2, 2, (int)getDecimals(Vsns2, 2));
        txcnt = snprintf(tx_buff, TX_BUFFER_SIZE,  "id1 = %i.%0*ik, V, V1 = %i.%0*i V, id2 = %i.%0*ik, V2 = %i.%0*i V\n", 
                                                    (int)id1, 2, (int)getDecimals(id1, 2),
                                                    (int)Vsns1, 2, (int)getDecimals(Vsns1, 2),
                                                    (int)id2, 2, (int)getDecimals(id2, 2),
                                                    (int)Vsns2, 2, (int)getDecimals(Vsns2, 2));
    }
    
    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
}

void readSensorsRaw(const char *params){
    HAL_StatusTypeDef status = 0;
    uint16_t ADCmsmnt;
    float Vsns1, Vsns2, id1, id2;

    uint16_t readings[4];
    readPirani(readings);
    // No error handling...


    txcnt = snprintf(tx_buff, TX_BUFFER_SIZE,  "%i, %i, %i, %i\n", 
                                                readings[0],
                                                readings[1],
                                                readings[2],
                                                readings[3]);

    
    USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;  // Enable Tx interrupts
    txidx = 0;
    rxcnt = 0;
    rxidx = 0;
    commandReceived = 0;
}

int readPirani(uint16_t readings[4]){
    HAL_StatusTypeDef status = 0;
    uint16_t ADCmsmnt;
    uint8_t chIdx[] = {6,2,7,3};    // Vsns1, Vsns2, id1, id2

    for (int i=0; i<4; i++){
        HAL_Delay(100);
        ADC1->CHSELR = chIdx[i] << ADC_CHSELR_SQ1_Pos;
        status |= HAL_ADC_Start(&hadc1);
        status |= HAL_ADC_PollForConversion(&hadc1, 10);
        // ADCmsmnt = HAL_ADC_GetValue(&hadc1);
        ADCmsmnt = ADC1->DR;
        readings[i] = ADCmsmnt;
    }

    return status;
}