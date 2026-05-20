## 1. HECHO

Utilizando CMSIS escriba y comente un código que genere una onda del tipo trapezoidal a la salida del DAC como se muestra en la figura. Para ello el DAC comenzará, a partir de cero, a incrementar su valor de a un bits hasta llegar a un valor máximo que se mantendrá un tiempo dado y luego decrementará de a un bits hasta volver a cero nuevamente. Los controles del valor máximo y los tiempos de duración de los escalones de subida y bajada están organizados en la posición de memoria 0x10004000 de la siguiente forma:

- bits 0 a 7: valor máximo a alcanzar por el DAC.
- bits 8 a 15: valor a utilizar en una función de demora que define el tiempo que se mantiene el valor máximo.
- bits 16 a 23: valor a utilizar en una función de demora para definir el tiempo que se mantiene cada incremento de 1 bits en la subida.
- bits 24 a 31: valor a utilizar en una función de demora para definir el tiempo que se mantiene cada decremento de 1 bits en bajada.
  ![Pasted image 20260517223636.png](/parciales/2P/docs/Pasted%20image%2020260517223636.png)

## 2.

Utilizando CMSIS y considerando una frecuencia de cpu de 100 Mhz, escriba y comente un código que configure una recepción por interrupciones UART con una trama que cumpla las siguientes características: Baudrate de 19200 baudios, paridad par, largo de palabra de datos de 8 bits y 1 bit de stop.

El programa debe permitir recibir por UART, 32 bits que van a ser guardados en la posición de memoria 0x10006000. Una vez finalizada toda la recepción se deshabilitará su interrupción.

A modo de ejemplo, realice un diagrama temporal de las dos primeras tramas recibidas, identificando cada una de las partes que constituyen dichas tramas.

## 3. HECHO

Considerando que se tiene un bloque de datos comprendidos entre las posiciones de memorias, dirección inicial= 0x10000800 a la dirección final= 0x10001000 ambas inclusive y se desea trasladar este bloque de datos una nueva zona de memoria comprendida entre la dirección inicial= 0x10002800 y la dirección Final=0x10003000 (en el mismo orden). Teniendo en cuenta además que los datos contenidos dentro de la zona de memoria son de 16 bits (AHB Master endianness configuration - por defecto) y que estos deben moverse de a uno (1) en cada evento de DMA, se sincronizará la transmisión con evento de match0 del timer1.

Se pide que Enumere y explique los puntos a tener en cuenta para configurar correctamente el controlador DMA.
