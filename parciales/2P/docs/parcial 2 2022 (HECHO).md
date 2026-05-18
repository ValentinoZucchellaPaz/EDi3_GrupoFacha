## 1. HECHO

Programar el microcontrolador LPC1769 para que mediante su ADC digitalice dos señales analógicas cuyos anchos de bandas son de 10 Khz cada una. Los canales utilizados deben ser el 2 y el 4 y los datos deben ser guardados en dos regiones de memorias distintas que permitan contar con los últimos 20 datos de cada canal. Suponer una frecuencia de core cclk de 100 Mhz. El código debe estar debidamente comentado.

## 2. HECHO

Utilizando el timer0, un dac, interrupciones y el driver del LPC1769 , escribir un código que permita generar una señal triangular periódica simétrica, que tenga el mínimo periodo posible, la máxima excursión de voltaje pico a pico posible y el mínimo incremento de señal posible por el dac. Suponer una frecuencia de core cclk de 100 Mhz. El código debe estar debidamente comentado.

## 3. HECHO

En la siguiente sección de código se realiza la configuración de un canal de DMA
a - Explique detalladamente cómo queda configurado el canal, que tipo de transferencia está realizando.
b - ¿Qué datos se transfieren, de qué posición a cuál y cuántas veces?
c- ¿Cómo se define el tiempo de "Interrup DMA request"  o el tiempo de transferencia de c/dato? 

**RTA**

a)
El canal 0 queda configurado para mover datos (TOTAL_SAMPLE cantidad de datos de 8bits) es una transferencia de memoria a periferico desde la memoria (datos_1_global) hacia la el periferico (los drivers van a ignorar lo que se ponga en dstMemAddr y van a usar el periferico especificado en dstConn). Finalmente, el trigger para hacer una nueva transferencia viene del DAC (se debe configurar este para que haga la request y configurar el contador del timeout para setear el intervalo en el que van a llegar las muestras).

Cuando termine de hacerse esa primera transferencia va a volver a configurarse con la LLI, en este caso la lli0 la cual es la una lli circular que configura la transferencia de datos_1_global hacia el DAC, transfiriendo TOTAL_SAMPLES cant de datos de 16bits, incrementando la fuente para recorrer el array de muestras de memoria y siempre mandarlos al registro de carga del DAC.

b)
Se transfieren datos de memoria hacia el DAC para que se transfieran TOTAL_SAMPLE cantidad de datos.

c)
Este tiempo se configura en el DAC, se debe setear los bits del DACCTRL Register de CNT_ENA y DMA_ENA, luego cargar el registro DACCNTVAL con un valor de 16bits.
Este es un contador de 16bits descendente el cual va a descender en cada flanco del PCLK y cuando llegue a 0 va a lanzar la request al DMA.

![Pasted image 20260517224552.png](/parciales/2P/docs/Pasted%20image%2020260517224552.png)
