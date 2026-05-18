## 1.

El siguiente Handler realiza el filtrado de la señal digitalizada por el ADC. Suponiendo una frecuencia de core de 40 Mhz se pide:

1. Utilizando el driver del LPC1769 escribir el código que configura al Timer (solo el Timer) de tal forma que permita al ADC muestrear la señal a una frecuencia de muestreo de 100 Hz.
2. Detallar de forma clara y concisa cómo ud. llevaría a cabo el cálculo para determinar los valores a guardar en los registros PR, MRx y los correspondientes bits del PCLK del Timer.
3. Explique con sus propias palabras el proceso que se está llevando a cabo en el Handler del ADC.
   En la ISR se mantiene un buffer tipo stack que guarda los ultimos 11 datos que se convirtieron, siendo el del indice 0 el mas nuevo.
   En este array se guardan los datos convertidos a su nivel de voltaje y no el numero digital que devuelve el ADC.
   Al final, hace la convolucion del array de los valores de la señal del ADC junto con otros datos (array de 11 elementos del filtro) y guarda toda la integral en una variable tipo float.
   ![Pasted image 20260517224208.png](/parciales/2P/docs/Pasted%20image%2020260517223931.png)

## 2.

Considerando el código del ejercicio anterior se pide:

1. Utilizando el driver del LPC1769 escribir el código que configura al ADC y a sus correspondientes pines para que funcione de acuerdo a lo desarrollado en el punto “a” del ejercicio anterior.
2. Reemplazar la línea de código 76 por la línea de código equivalente que hace uso del driver.
3. Escribir las líneas de código que deben agregarse en el Handler para generar un diezmado por N a la salida del filtro, siendo N una variable global uint8_t. Esto es, si N es igual a 10, solo se deben tener en cuenta una de cada 10 muestras que salen del filtro. Las últimas 20 muestras diezmadas deben ser guardadas en una memoria de tipo circular.
4. Explique con sus propias palabras qué sucede y que consideraciones hay que tener en cuenta desde el punto de vista de la señales a digitalizar si, manteniendo la frecuencia de trabajo del ADC fija, se van agregando mas canales de conversión.

El ADC funciona de forma lineal en las conversiones, si hay mas de un canal activo primero convierte en el canal 0, luego en el 1 y asi hasta terminar con los canales activos en ese trigger de conversión (el mismo trigger hace que convierta todos los canales).

Si nuestro ADC funciona en modo burst o continuo, la frecuencia de conversión se reducirá a la mitad por cada nuevo canal agregado. Se hace una conversión cada 65 ciclos de CLK, ciclos de PCLK dividido por el divisor de CLK para lograr la frecuencia deseada. Entonces si seteo ADC_Init(200000) (200kHz de frecuencia de muestreo) el primer canal va a convertir con esa frecuencia, pero luego el segundo canal va a convertir tmb antes de volver al primero, haciendo que el primer canal solo tome muestras con frecuencia de 100kHz

Ese es el caso del ADC en modo continuo, pero no es nuestro caso, ya que nosotros hacemos trigger manual cada 10ms usando MAT0.1 y el timer. De esta forma el ADC no pierda frecuencia de trabajo si convierte con frec de muestreo 200kHz.
Cuando venga un flanco que comience la conversion (cada 10ms), el ADC va a convertir en el primer canal a 200kHz y luego pasar al segundo, en 5us va a terminar las dos conversiones y luego esperar al siguiente flanco del MAT0.1, sin perder frecuencia de muestreo real.

## 3.

Considerando que se tiene un bloque de datos comprendidos entre las posiciones de memorias, dirección inicial= 0x10000800 a la dirección final= 0x10001000 ambas inclusive y se desea trasladar este bloque de datos a una nueva zona de memoria comprendida entre la dirección inicial= 0x10002800 y la dirección Final=0x10003000 (en el mismo orden). Teniendo en cuenta además que los datos contenidos dentro de la zona de memoria son de 16 bits (AHB Master endianness configuration - por defecto) y que estos deben moverse de a uno (1) en cada evento de DMA, se sincronizará la transmisión con evento de match0 del timer1.

Se pide que Enumere y explique los puntos a tener en cuenta para configurar correctamente el controlador DMA.

**RTA**

Las transmisiones memoria a memoria son continuas, el GPDMA va a transferir todo cuando le de start y no puedo decirle que en cada evento de match dispare una nueva transferencia.

Si quiero sincronizar cada transferencia con el timer haciendo una transf M2M, puedo hacer una interrupcion de match0 que comience la transferencia

El DMA se debe configurar de la siguiente forma

1.  Encender modulo GPDMA en PCONP : GPDMA_Init();
2.  Configurar canal de transmisión: GPDMA_SetupChannel(const GPDMA_Channel_CFG_T\* dmaCfg);
    1. Se van a transferir 512 bytes de memoria con ancho de 16bits se harán 256 transferencias (pero hago size 1 xq se van a mover de a 1 los datos)
    2. Selecciono el numero de canal: 0
    3. Selecciono el tamaño de la transferencia: 1
    4. Selecciono el tamaño de la palabra a transferis HALFWORD (16bits)
    5. Selecciono el tipo de transferencia: memoria a memroria
    6. Selecciono direccion de comienzo y destino: 0x10000800 -> 0x10002800 (estos valores se van a reconfiguar cuando se lance una int de timer, manejado por el isr del timer)
    7. Hago la struct Endpoint para src y dst: especifico width (halfword), increment (enable) y burst (1 por el ejercicio)
    8. No es necesaria una LLI
3.  Configuro el Timer y su ISR:
    1. En ISR channel start para comenzar transferencia.

Otra forma puede ser:
hacer transferencia m2p, configurando el trigger con el toggle del MAT0.1, y la direccion de memoria que tengo. El unico problema es que se deben usar los punteros (LPC_GPDMA->DestAddr) y no los drivers por restricciones de diseño
El DMA se debe configurar de la siguiente forma

1.  Encender modulo GPDMA en PCONP : GPDMA_Init();
2.  Configurar canal de transmisión: GPDMA_SetupChannel(const GPDMA_Channel_CFG_T\* dmaCfg);
    1. Se van a transferir 512 bytes de memoria con ancho de 16bits se harán 256 transferencias
    2. Selecciono el numero de canal: 0
    3. Selecciono el tamaño de la transferencia: 256
    4. Selecciono el tamaño de la palabra a transferis HALFWORD (16bits)
    5. Selecciono el tipo de transferencia: M2P
    6. Selecciono direccion de comienzo y destino: 0x10000800 -> 0x10002800 (la direccion de destino la debo configurar usando punteros luego de configurar con los drivers)
    7. Selecciono dstConn como MAT0.0
    8. Hago la struct Endpoint para src y dst: especifico width (halfword), increment (enable) y burst (1 por el ejercicio)
    9. No es necesaria una LLI
3.  Configuro el Timer y su ISR:
    1. Configuro prescaler y match0
    2. hago que match0 haga reset y toggle del pin externo (MAT0.0)
    3. enciendo
