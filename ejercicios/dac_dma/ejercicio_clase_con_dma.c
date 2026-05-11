// hacer onda serrucho con periodo 1ms y 10 cambios por periodo
// ahora con dma disparado por dac

#include "LPC17xx.h"

#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"

#define N   10 // cant de elementos de array

// array con valores de onda triangular
volatile uint16_t values[N];

// firmas de funciones
void configDAC(void);
void configDMA(GPDMA_LLI_T* lli);

int main(void) {
    // inicializar array
    for(int i=0;i<N;i++){
        // escalado de onda y bit-shift a posicion de carga en dac (DACR[15:6])
        values[i]=((1023/9)*i)<<6;
    }


    // config dma lli (circular)
    GPDMA_LLI_T lli;
    lli.srcAddr=(uint32_t)&values;
    lli.dstAddr=(uint32_t)LPC_DAC->DACR;
    lli.control= (10<<0)    // transfer size (cant de datos a transferir)
                | (0<<12)   // Src burst size (1)
                | (0<<15)   // Dst burst size (1)
                | (1<<18)   // Src transfer width (16 bits)
                | (1<<21)   // Dst transfer width (16 bits)
                | (1<<26)   // Src increment (si)
                | (0<<27)   // Dst increment (no)
                | (0<<31);  // Interrupt on end of transfer (no)
    lli.nextLLI=(uint32_t)&lli;

    configDMA(&lli);
    configDAC();
    while(1){
    }
    return 1;
}

void configDMA(GPDMA_LLI_T* lli){

    // estructura de config de fuente y destino de transferencia:
    // 1 dato de 16 bits y que incrementa la fuente en cada transferencia (recorro array)
    GPDMA_Endpoint_T srcc= {burst:GPDMA_BSIZE_1, width:GPDMA_HALFWORD, increment:ENABLE};
    GPDMA_Endpoint_T dstc= {burst:GPDMA_BSIZE_1, width:GPDMA_HALFWORD, increment:DISABLE};

    // estructura de config de dma
    GPDMA_Channel_CFG_T dmaConfig;
    dmaConfig.channelNum=GPDMA_CH_0;    // canal de dma
    dmaConfig.transferSize=10;          // 10 datos del array
    dmaConfig.type=GPDMA_M2P;           // del array a registro dma
    dmaConfig.src=srcc;                 // config transf: src incrementa (array), dst queda fijo (DAC)
    dmaConfig.dst=dstc;
    dmaConfig.srcConn=DISABLE;          // trigger de dma request => dac (dst)
    dmaConfig.dstConn=GPDMA_DAC;
    dmaConfig.intErr=DISABLE;           // sin int por error
    dmaConfig.intTC=DISABLE;            // sin int cuando termina de trans 10 datos
    dmaConfig.linkedList=(uint32_t)lli; // asigno lli circular (reconfigurar para enviar array de nuevo)
    // direcciones de mem src y dst
    dmaConfig.srcMemAddr=(uint32_t)&values;
    dmaConfig.dstMemAddr=(uint32_t)&LPC_DAC->DACR;


    GPDMA_Init();                       // encender
    GPDMA_SetupChannel(&dmaConfig);     // config
    GPDMA_ChannelStart(GPDMA_CH_0);     // arranque
}

void configDAC(){
    DAC_CONVERTER_CFG_T dacConfig;
    dacConfig.doubleBuffer=DISABLE;
    dacConfig.dmaCounter=ENABLE;
    dacConfig.dmaRequest=ENABLE;

	DAC_Init();                         // encender
	DAC_SetBias(DAC_350uA);             // periodo 0.1ms => 400kHz con 350uA (bajo consumo)
    DAC_SetDMATimeOut(2500);            // periodo entre request a dma => 0.1ms
    // time = ticks / 25MHz -> 2500 ticks = 0.1 ms (CORROBORAR)
    DAC_ConfigDAConverterControl(&dacConfig); // configurar trigger de dma request
}
