/**
    Se tienen tres bloques de datos de 4KBytes de longitud cada uno en el cual se han guardado tres formas de onda.
  Cada muestra de la onda es un valor de 32 bits que se ha capturado desde el ADC. Las direcciones de inicio de cada bloque son representadas por macros del estilo DIRECCION_BLOQUE_N, con N=0,1,2.
  Se pide que, usando DMA y DAC se genere una forma de onda por la salida analógica de la LPC1769.
  La forma de onda cambiará en función de una interrupción externa conectada a la placa de la siguiente manera:
  • 1er interrupción: Forma de onda almacenada en bloque 0, con frecuencia de señal de 60[KHz].
  • 2da interrupción: Forma de onda almacenada en bloque 1 con frecuencia de señal de 120[KHz].
  • 3ra interrupción: Forma de onda almacenada en bloque 0 y bloque 2 (una a continuación de la otra) con frecuencia de señal de 450[KHz].
  • 4ta interrupción: Vuelve a comenzar con la forma de onda del bloque 0.
  En cada caso se debe utilizar el menor consumo de energía posible del DAC.

  RAZONAMIENTO :

  HAY 1K MUESTRAS por señal, y solo debo considerar los primeros 16bits ya que el resto es el resto del registro del adc.
  Cuando debo sacar 2 formas de onda, hago 2 transferencias de adc con lli (entrelazadas), primero bloque 0 luego bloque 2 y repito

  Frecuencias de DAC :

  Nyquist, saco 2 muestras por periodo
  dma_trigger = ticks / PCLK → ticks = dma_trigger * PCLK

  1- 60kHz → saco a 120kHz → pido muestra cada 8.333 us
  DAC_SetDMATimeOut(208); -> 8.32us -> 120.2kHz
  2- 120kHz → saco a 240kHz → pido muestra cada 4.167 us
  DAC_SetDMATimeOut(104); -> 4.16us -> 240.4kHz
  3- 450kHz → saco a 900kHz → pido muestra cada 1.1 us
  DAC_SetDMATimeOut(28); -> 1.12us -> 892kHz

  Para 1. y 2. debo usar BIAS=1 => bajo consumo 350uA max frec 400kHz -> 2.5us max
  Para 3. debo usar BIAS=0 => alto consumo 700uA max frec 1MHz -> 1us max
 */

#include "LPC17xx.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_exti.h"

#define DIRECCION_BLOQUE_0 0x2007C000
#define DIRECCION_BLOQUE_1 0x2007D000
#define DIRECCION_BLOQUE_2 0x2007E000

typedef enum
{
    ESPERA,
    ONDA0,
    ONDA1,
    ONDA0y2
} estado_t;

estado_t estado = ESPERA;

void confEXTI(void);
void confDAC(void);
void confDMA(GPDMA_LLI_T *lli_onda_0, GPDMA_LLI_T *lli_onda_1, GPDMA_LLI_T *lli_onda_2);

int main(void)
{
    GPDMA_LLI_T onda0, onda1, onda0_a_2, onda2_a_0;
    onda0.srcAddr = (uint32_t)DIRECCION_BLOQUE_0;
    onda0.dstAddr = (uint32_t)&LPC_DAC->DACR;
    onda0.nextLLI = (uint32_t)&onda0;
    onda0.control = 1024 | (2 << 18) | (1 << 21) | (1 << 26); // 1024 transf de 32bits en la src, 16 bits en dst e incrementa la src
    onda1 = onda0;
    onda1.srcAddr = (uint32_t)DIRECCION_BLOQUE_1;
    onda1.nextLLI = (uint32_t)&onda1;

    // lli circulares que muestran primero onda 0 y dps 2
    onda2_a_0 = onda0;
    onda2_a_0.srcAddr = (uint32_t)DIRECCION_BLOQUE_2;
    onda2_a_0.nextLLI = (uint32_t)&onda0_a_2;
    onda0_a_2 = onda0;
    onda0_a_2.srcAddr = (uint32_t)DIRECCION_BLOQUE_0;
    onda0_a_2.nextLLI = (uint32_t)&onda2_a_0;

    confEXTI();
    confDMA(&onda0, &onda1, &onda2_a_0);

    while (1)
    {
    }
    return 0;
}

void confEXTI(void)
{
    EXTI_CFG_T extiCfg;
    extiCfg.line = EXTI_EINT0;
    extiCfg.mode = EXTI_EDGE_SENSITIVE;
    extiCfg.polarity = EXTI_FALLING_EDGE;
    EXTI_Init(); // enciende
    EXTI_PinConfig(EXTI_EINT0, EXTI_NOPULL);
    EXTI_ConfigEnable(&extiCfg); // configura y habilita interrupcion
}

void confDAC(void)
{
    DAC_CONVERTER_CFG_T dacCfg;
    dacCfg.dmaCounter = ENABLE;
    dacCfg.dmaRequest = ENABLE;
    dacCfg.doubleBuffer = DISABLE;

    DAC_Init();
    DAC_SetBias(DAC_350uA);
    DAC_ConfigDAConverterControl(&dacCfg);
    DAC_SetDMATimeOut(208);
}

void confDMA(GPDMA_LLI_T *lli_onda_0, GPDMA_LLI_T *lli_onda_1, GPDMA_LLI_T *lli_onda_2)
{
    GPDMA_Channel_CFG_T onda0Cfg, onda1Cfg, onda0y2Cfg;
    onda0Cfg.channelNum = GPDMA_CH_0;
    onda0Cfg.linkedList = (uint32_t)lli_onda_0;
    onda0Cfg.transferSize = 1024;
    onda0Cfg.type = GPDMA_M2P;
    onda0Cfg.src.burst = GPDMA_BSIZE_1;
    onda0Cfg.src.increment = ENABLE;
    onda0Cfg.src.width = GPDMA_WORD;
    onda0Cfg.dst.burst = GPDMA_BSIZE_1;
    onda0Cfg.dst.increment = DISABLE;
    onda0Cfg.dst.width = GPDMA_HALFWORD;
    onda0Cfg.srcMemAddr = DIRECCION_BLOQUE_0;
    onda0Cfg.dstMemAddr = (uint32_t)&LPC_DAC->DACR;
    onda0Cfg.dstConn = GPDMA_DAC;

    onda1Cfg = onda0Cfg;
    onda1Cfg.channelNum = GPDMA_CH_1;
    onda1Cfg.srcMemAddr = DIRECCION_BLOQUE_1;
    onda1Cfg.linkedList = (uint32_t)lli_onda_1;

    onda0y2Cfg = onda0Cfg;
    onda0y2Cfg.channelNum = GPDMA_CH_2;
    onda0y2Cfg.linkedList = (uint32_t)lli_onda_2;

    GPDMA_Init();
    GPDMA_SetupChannel(&onda0Cfg);
    GPDMA_SetupChannel(&onda1Cfg);
    GPDMA_SetupChannel(&onda0y2Cfg);
}

void EXTI_IRQHandler(void)
{
    if (EXTI_GetFlag(EXTI_EINT0) == SET)
    {
        EXTI_ClearFlag(EXTI_EINT0);
        switch (estado)
        {
        case ESPERA:
        {
            estado = ONDA0;
            break;
        }
        case ONDA0:
        {
            estado = ONDA1;
            confDAC(); // DAC_SetDMATimeOut(208); -> 8.32us -> 120.2kHz
            GPDMA_ChannelGracefulStop(GPDMA_CH_1);
            GPDMA_ChannelGracefulStop(GPDMA_CH_2);
            GPDMA_ChannelStart(GPDMA_CH_0);
            break;
        }

        case ONDA1:
        {
            estado = ONDA0y2;
            DAC_SetDMATimeOut(104); // -> 4.16us -> 240.4kHz
            GPDMA_ChannelGracefulStop(GPDMA_CH_0);
            GPDMA_ChannelGracefulStop(GPDMA_CH_2);
            GPDMA_ChannelStart(GPDMA_CH_1);
            break;
        }
        case ONDA0y2:
        {
            estado = ONDA0;
            DAC_SetBias(DAC_700uA);
            DAC_SetDMATimeOut(28); // -> 1.12us -> 892kHz
            GPDMA_ChannelGracefulStop(GPDMA_CH_0);
            GPDMA_ChannelGracefulStop(GPDMA_CH_1);
            GPDMA_ChannelStart(GPDMA_CH_2);
            break;
        }

        default:
            break;
        }
    }
}
