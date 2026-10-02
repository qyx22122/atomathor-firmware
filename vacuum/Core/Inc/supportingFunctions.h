#ifndef SUPPORTINGFUNCTIONS_H
#define SUPPORTINGFUNCTIONS_H

#include <stdint.h>

int oneHotDecode(uint32_t code);
int oneHotDecode_unitTest(void);
unsigned int getDecimals(float num, uint8_t n);

#endif