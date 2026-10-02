#include "supportingFunctions.h"

/*  Takes a one-hot encoded 32b value and returns the index of the hot bit.
    If there are multiple hot bits, the left-most one (MSB side) is returned.
    If there are no hot bits, returns all 1. This error state can be detected
        by bitwise flipping return value and checking validity.
    Examples:
        decode4 = oneHotDecode(4) = 3
        decode5 = oneHotDecode(5) = 3 // ignores bit 0
        decode0 = oneHotDecode(0) = 0xFFFF
            eval(~decode0) = false
            eval(~decode4) = true
        
*/
int oneHotDecode(uint32_t code){
    // If there is no one hot, return -1 immediately
    if (!code){
        return ~0x0;
    }
    // Otherwise, do the bisection trick for first few layers.
    uint16_t pos = 0;
    if (code & 0xFFFF0000){
        pos = 16;
        code = code >> 16;
    }
    if (code & 0xFF00){
        pos += 8;
        code = code >> 8;
    }
    if (code & 0xF0){
        pos += 4;
        code = code >> 4;
    }
    // But for last 4b, this might be faster, though it wasn't tested.
    switch (code){
        case (1<<3):
            return pos+3;
        case (1<<2):
            return pos+2;
        case (1<<1):
            return pos+1;
        case (1<<0):
            return pos+0;
    }
}

// Tests if oneHotDecode(code) function works. Returns 0 if OK, and -1 on fail
int oneHotDecode_unitTest(void){
    for (int i = 0; i<32; i++){
        uint32_t code = 1<<i;
        uint16_t decode = oneHotDecode(code);
        if (i != decode){
            return -1;
        }
    }
    return 0;
}
// Calculates decimals of a float to make printing easier // sprintf(str, "%i.%0*i", (int)num, n, (int)getDecimals(num, n))
unsigned int getDecimals(float num, uint8_t n){
    num = num-(int)num; // remove the whole part
    //return num*pow(10,n); // This function is heavy on FLASH...
    for(int i=0; i<n; i++){
        num *= 10;
    }
    return (unsigned int)num;
}