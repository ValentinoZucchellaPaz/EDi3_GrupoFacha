#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"

#define DMA_SIZE 60
#define NUM_SINE_SAMPLE 60
#define SINE_FREQ_IN_HZ 50
#define PCLK_DAC_IN_MHZ 25 // CCLK divided by 4

void confDMA(void);
void confDac(void);
volatile uint32_t dac_sine_lut[NUM_SINE_SAMPLE];

int main(void)
{
	// create mem array with sine samples
	uint32_t i;
	uint32_t sin_0_to_90_16_samples[16] = {
		0, 1045, 2079, 3090, 4067,
		5000, 5877, 6691, 7431, 8090,
		8660, 9135, 9510, 9781, 9945, 10000};
	for (i = 0; i < NUM_SINE_SAMPLE; i++)
	{
		if (i <= 15)
		{
			dac_sine_lut[i] = 512 + 512 * sin_0_to_90_16_samples[i] / 10000;
			if (i == 15)
				dac_sine_lut[i] = 1023;
		}
		else if (i <= 30)
		{
			dac_sine_lut[i] = 512 + 512 * sin_0_to_90_16_samples[30 - i] / 10000;
		}
		else if (i <= 45)
		{
			dac_sine_lut[i] = 512 - 512 * sin_0_to_90_16_samples[i - 30] / 10000;
		}
		else
		{
			dac_sine_lut[i] = 512 - 512 * sin_0_to_90_16_samples[60 - i] / 10000;
		}
		dac_sine_lut[i] = (dac_sine_lut[i] << 6);
	}

	// configure and start dma & dac
	confDac();
	confDMA();
	while (1)
	{
	}
	return 0;
}

void confDMA(void)
{
	// lli struct
	GPDMA_LLI_T DMA_LLI_Struct;
	DMA_LLI_Struct.srcAddr = (uint32_t)dac_sine_lut;
	DMA_LLI_Struct.dstAddr = (uint32_t)&(LPC_DAC->DACR);
	DMA_LLI_Struct.nextLLI = (uint32_t)&DMA_LLI_Struct;
	DMA_LLI_Struct.control = DMA_SIZE | (2 << 18) // source width 32 bit
							 | (2 << 21)		  // dest. width 32 bit
							 | (1 << 26)		  // source increment
		;

	// struct for config src and dst transfers
	// 32 bits data and increment src on transfer (move one array position forward)
	GPDMA_Endpoint_T srcc = {burst : GPDMA_BSIZE_1, width : GPDMA_WORD, increment : ENABLE};
	GPDMA_Endpoint_T dstc = {burst : GPDMA_BSIZE_1, width : GPDMA_WORD, increment : DISABLE};

	// dma config struct
	GPDMA_Channel_CFG_T dmaConfig;
	dmaConfig.channelNum = GPDMA_CH_0; // dma channel
	dmaConfig.transferSize = DMA_SIZE; // array data
	dmaConfig.type = GPDMA_M2P;
	dmaConfig.src = srcc;
	dmaConfig.dst = dstc;
	dmaConfig.srcConn = DISABLE;
	dmaConfig.dstConn = GPDMA_DAC;					  // dma trigger by dac
	dmaConfig.intErr = DISABLE;						  // sin int por error
	dmaConfig.intTC = DISABLE;						  // sin int cuando termina de transf
	dmaConfig.linkedList = (uint32_t)&DMA_LLI_Struct; // asigno lli
	// mem address directions
	dmaConfig.srcMemAddr = (uint32_t)dac_sine_lut;
	dmaConfig.dstMemAddr = (uint32_t)&LPC_DAC->DACR;

	GPDMA_Init();					// enable
	GPDMA_SetupChannel(&dmaConfig); // config
	GPDMA_ChannelStart(GPDMA_CH_0); // start
	return;
}

void confDac(void)
{
	uint32_t tmp;
	DAC_CONVERTER_CFG_T DAC_ConverterConfigStruct;
	DAC_ConverterConfigStruct.dmaCounter = ENABLE;
	DAC_ConverterConfigStruct.dmaRequest = ENABLE;
	DAC_ConverterConfigStruct.doubleBuffer = DISABLE;

	DAC_Init();
	DAC_SetBias(DAC_700uA);

	/* set time out for DAC*/
	tmp = (PCLK_DAC_IN_MHZ * 1000000) / (SINE_FREQ_IN_HZ * NUM_SINE_SAMPLE);
	DAC_SetDMATimeOut(tmp);
	DAC_ConfigDAConverterControl(&DAC_ConverterConfigStruct);
}