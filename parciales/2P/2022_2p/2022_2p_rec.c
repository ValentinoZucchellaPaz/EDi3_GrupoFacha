/**
 * ITEM 1:
 * El siguiente Handler realiza el filtrado de la señal digitalizada por el ADC. Suponiendo una frecuencia de core de 40 Mhz se pide:
 * 1. Utilizando el driver del LPC1769 escribir el código que configura al Timer (solo el Timer) de tal forma que permita al ADC muestrear la señal a una frecuencia de muestreo de 100 Hz.
 * 2. Detallar de forma clara y concisa cómo ud. llevaría a cabo el cálculo para determinar los valores a guardar en los registros PR, MRx y los correspondientes bits del PCLK del Timer.
 * 3. Explique con sus propias palabras el proceso que se está llevando a cabo en el Handler del ADC.
 *
 * RTA :
 * En la ISR se mantiene un buffer tipo stack que guarda los ultimos 11 datos que se convirtieron, siendo el del indice 0 el mas nuevo.
 * En este array se guardan los datos convertidos a su nivel de voltaje y no el numero digital que devuelve el ADC.
 * Al final, hace la convolucion del array de los valores de la señal del ADC junto con otros datos (array de 11 elementos del filtro) y guarda toda la integral en una variable tipo float.
 *
 * ITEM 2:
 * Considerando el código del ejercicio anterior se pide:
 * 1. Utilizando el driver del LPC1769 escribir el código que configura al ADC y a sus correspondientes pines para que funcione de acuerdo a lo desarrollado en el punto “a” del ejercicio anterior.
 * 2. Reemplazar la línea de código 76 por la línea de código equivalente que hace uso del driver.
 * 3. Escribir las líneas de código que deben agregarse en el Handler para generar un diezmado por N a la salida del filtro, siendo N una variable global uint8_t. Esto es, si N es igual a 10, solo se deben tener en cuenta una de cada 10 muestras que salen del filtro. Las últimas 20 muestras diezmadas deben ser guardadas en una memoria de tipo circular.
 * 4. Explique con sus propias palabras qué sucede y que consideraciones hay que tener en cuenta desde el punto de vista de la señales a digitalizar si, manteniendo la frecuencia de trabajo del ADC fija, se van agregando mas canales de conversión.
 *
 * RTA :
 * El ADC funciona de forma lineal en las conversiones, si hay mas de un canal activo primero convierte en el canal 0, luego en el 1 y asi hasta terminar con los canales activos en ese trigger de conversión (el mismo trigger hace que convierta todos los canales).
 *
 * Si nuestro ADC funciona en modo burst o continuo, la frecuencia de conversión se reducirá a la mitad por cada nuevo canal agregado. Se hace una conversión cada 65 ciclos de CLK, ciclos de PCLK dividido por el divisor de CLK para lograr la frecuencia deseada. Entonces si seteo ADC_Init(200000) (200kHz de frecuencia de muestreo) el primer canal va a convertir con esa frecuencia, pero luego el segundo canal va a convertir tmb antes de volver al primero, haciendo que el primer canal solo tome muestras con frecuencia de 100kHz
 *
 * Ese es el caso del ADC en modo continuo, pero no es nuestro caso, ya que nosotros hacemos trigger manual cada 10ms usando MAT0.1 y el timer. De esta forma el ADC no pierda frecuencia de trabajo si convierte con frec de muestreo 200kHz.
 * Cuando venga un flanco que comience la conversion (cada 10ms), el ADC va a convertir en el primer canal a 200kHz y luego pasar al segundo, en 5us va a terminar las dos conversiones y luego esperar al siguiente flanco del MAT0.1, sin perder frecuencia de muestreo real.
 *
 * ITEM 3:
 * Considerando que se tiene un bloque de datos comprendidos entre las posiciones de memorias, dirección inicial= 0x10000800 a la dirección final= 0x10001000 ambas inclusive y se desea trasladar este bloque de datos a una nueva zona de memoria comprendida entre la dirección inicial= 0x10002800 y la dirección Final=0x10003000 (en el mismo orden). Teniendo en cuenta además que los datos contenidos dentro de la zona de memoria son de 16 bits (AHB Master endianness configuration - por defecto) y que estos deben moverse de a uno (1) en cada evento de DMA, se sincronizará la transmisión con evento de match0 del timer1.
 *
 * Se pide que Enumere y explique los puntos a tener en cuenta para configurar correctamente el controlador DMA.
 *
 * RTA :
 * Las transmisiones memoria a memoria son continuas, el GPDMA va a transferir todo cuando le de start y no puedo decirle que en cada evento de match dispare una nueva transferencia.
 * Si quiero sincronizar cada transferencia con el timer haciendo una transf M2M, puedo hacer una interrupcion de match0 que reconfigue el DMA (encargandose de mover los punteros de las direcciones de memoria).
 *
 * El DMA se debe configurar de la siguiente forma
 * 1. Encender modulo GPDMA en PCONP : GPDMA_Init();
 * 2. Configurar canal de transmisión: GPDMA_SetupChannel(const GPDMA_Channel_CFG_T* dmaCfg);
 *      Se van a transferir 512 bytes de memoria con ancho de 16bits se harán 256 transferencias (pero hago size 1 xq se van a mover de a 1 los datos)
 *      Selecciono el numero de canal: 0
 *      Selecciono el tamaño de la transferencia: 1
 *      Selecciono el tamaño de la palabra a transferis HALFWORD (16bits)
 *      Selecciono el tipo de transferencia: memoria a memroria
 *      Selecciono direccion de comienzo y destino: 0x10000800 -> 0x10002800 (estos valores se van a reconfiguar cuando se lance una int de timer, manejado por el isr del timer)
 *      Hago la struct Endpoint para src y dst: especifico width (halfword), increment (disable) y burst (1 por el ejercicio)
 *      No es necesaria una LLI
 * 3. Configuro el Timer y su ISR.
 *      Se van a usar punteros de uint16_t los cuales apunten a los comienzos de las direcciones de mem.
 *      En ISR por match configuro la transferencia de DMA (lugares de origen y destino solamente para no volver a configurar todo)
 *      Doy channel start para comenzar transferencia.
 *      Aumento punteros de mem y evaluo si debo hacer mas transferencias o no, en caso que no uso flag para que prox match no dispare la transferencia de nuevo.
 */

#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"

#define N 10
#define BUFFER_SIZE 20

volatile float savedFilteredSamples[BUFFER_SIZE];
volatile uint32_t writeIdx = 0;

void confTimer(void)
{
    // pclk viene por defecto como cclk/4=10MHz
    // seteo prescaler pq cuente 1 milisegundo (driver se encargan del modo)
    // match cuenta 5ms y hace toggle de MAT01
    // adc comienza conversion en rising edge de match (cada 10ms -> 100Hz)
    TIM_TIMERCFG_T timerCfg = {.prescaleOpt = TIM_US, .prescaleValue = 10000};
    TIM_MATCHCFG_T matchCfg = {
        .channel = TIM_MATCH_1,
        .extOpt = TIM_TOGGLE,
        .matchValue = 5,
        .intEn = DISABLE,
        .resetEn = ENABLE,
        .stopEn = DISABLE};

    TIM_InitTimer(LPC_TIM0, &timerCfg);
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);
    TIM_Enable(LPC_TIM0);
    // no es necesario configurar el pin de match para el correcto funcionamiento del toggle y conversion de adc
}

void confADC(void)
{
    ADC_Init(200000); // convierta usando la max frec posible
    ADC_PinConfig(ADC_CHANNEL_0);
    ADC_ChannelEnable(ADC_CHANNEL_0);
    ADC_BurstDisable();
    ADC_StartCmd(TIM_MAT0_0_P1_28);
    ADC_EdgeStartConfig(ADC_START_ON_RISING);
    ADC_IntEnable(ADC_INT_CH0);
    ADC_PowerUp();
}

int main(void)
{

    confADC();
    confTimer();
    while (1)
    {
    }

    return 0;
}

void ADC_ISRHandler(void)
{
    static float ADCVolt[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    static float coeff[11] = {};
    static int counter = 0;
    float filtSignal = 0;

    float ADCVal = ADC_ChannelGetData(ADC_CHANNEL_0);

    for (int i = 10; i >= 1; i--)
        ADCVolt[i] = ADCVolt[i - 1];

    ADCVolt[0] = (ADCVal / 4096) * 3.3;

    for (int i = 0; i < 10; i++)
        filtSignal += coeff[i] * (ADCVolt[i] + ADCVolt[10 - i]);

    // Ejercicio 2, pto 3: guardar 1 de cada N datos filtrados en array circular de 20 elementos
    if (counter == N)
    {
        savedFilteredSamples[writeIdx] = filtSignal;
        writeIdx = (writeIdx + 1) % BUFFER_SIZE;
        counter = 0;
    }
    else
        counter++;

    return;
}
