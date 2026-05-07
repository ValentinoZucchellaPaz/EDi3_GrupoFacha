// hacer onda serrucho con periodo 1ms y 10 cambios por periodo

#include "lpc17xx.h"
#include "lpc17xx_dac.h"

uint16_t * values[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

void configDAC(void);

int main(void) {
    while(1){

    }
    return 1;
}

void configDAC(){
	DAC_Init();
	DAC_CONVERTER_CFG_T dacdm= {
		doubleBuffer: ENABLE, dmaCounter: ENABLE , dmaRequest:ENABLE 
	};
	DAC_SetBias(DAC_350uA);
	DAC_UpdateValue(0);
}