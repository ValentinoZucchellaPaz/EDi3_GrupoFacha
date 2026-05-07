// ejercicio trujillo: promedio adc -> convertir señal con adc, llenar un array de 100 elementos (con dma y burst mode), hacer promedio y mostrar señal promediada por dac

#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_dac.h"


void config_DMA(GPDMA_LLI_T lli);
void config_ADC(void);
void config_DAC(void);
void config_Pins(void);

// arrays continuos de 100 posicion que estan en al inicio de AHB_SRAM
uint16_t * buffer1 = (uint16_t *)0x2007C000;
uint16_t * buffer2 = (uint16_t *)0x2007C0C8;

int main(void) {
	GPDMA_LLI_T lli1;
	GPDMA_LLI_T lli2;

	GPDMA_LLI_T lli1 = { srcAddr: &LPC_ADC->ADGDR, dstAddr:&buffer1, nextLLI:&lli2, control:100|1<<18|1<<21|1<<27|1<<31};
	GPDMA_LLI_T lli2 = { srcAddr: &LPC_ADC->ADGDR, dstAddr:&buffer1, &lli1 , nextLLI:&lli2, control:100|1<<18|1<<21|1<<27|1<<31};

	config_Pins();
	config_DMA(lli2);
	config_DAC();
	config_ADC();

    while(1) {
		// ... 
    }
    return 0 ;
}


void config_DMA(GPDMA_LLI_T lli){
	GPDMA_Endpoint_T dsrc={burst: GPDMA_HALFWORD,width: GPDMA_BSIZE_1,increment: DISABLE};
	GPDMA_Endpoint_T ddst={burst: GPDMA_HALFWORD,width: GPDMA_BSIZE_1,increment: ENABLE};
	GPDMA_DeInit();
	GPDMA_Channel_CFG_T dmaCfg;
	dmaCfg.transferSize=100;
	dmaCfg.channelNum=GPDMA_CH_0;
	dmaCfg.type= GPDMA_P2M;
	dmaCfg.srcMemAddr=&LPC_ADC->ADGDR;
	dmaCfg.dstMemAddr= &buffer1;
	dmaCfg.srcConn = (GPDMA_ADC);

	dmaCfg.src = dsrc;
	dmaCfg.dst = ddst;
	dmaCfg.intTC = ENABLE;
	dmaCfg.intErr = DISABLE;
	dmaCfg.linkedList= (uint32_t*)&lli;

	GPDMA_SetupChannel(&dmaCfg);
	GPDMA_ChannelStart(GPDMA_CH_0);
}
void configADC(){
	ADC_Init(25000);
	ADC_PinConfig(ADC_CHANNEL_4);
	ADC_ChannelEnable(ADC_CHANNEL_4);
	ADC_BurstEnable();
}
void configDAC(){
	DAC_Init();
	DAC_CONVERTER_CFG_T dacdm= {
		doubleBuffer: ENABLE, dmaCounter: ENABLE , dmaRequest:ENABLE 
	};
	DAC_SetBias(DAC_350uA);
	DAC_UpdateValue(0);
}