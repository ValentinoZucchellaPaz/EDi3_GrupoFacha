## 1. HECHO
Por un pin del ADC del microcontrolador LPC1769 ingresa una tensión de rango dinámico 0 a 3,3[v] proveniente de un sensor de temperatura. Debido a la baja tasa de variación de la señal, se pide tomar una muestra cada 30[s]. Pasados los 2[min] se debe promediar las últimas 4 muestras y en función de este valor, tomar una decisión sobre una salida digital de la placa:
- Si el valor es <1 [V] colocar la salida en 0 (0[V]).
- Si el valor es >= 1[V] y <=2[V] modular una señal PWM con un Ciclo de trabajo que va desde el 50% hasta el 90% proporcional al valor de tensión, con un periodo de 20[KHz].
- Si el valor es > 2[V] colocar la salida en 1 (3,3[V]).

## 2. HECHO
Se tienen tres bloques de datos de 4KBytes de longitud cada uno en el cual se han guardado tres formas de onda.
Cada muestra de la onda es un valor de 32 bits que se ha capturado desde el ADC. Las direcciones de inicio de cada bloque son representadas por macros del estilo DIRECCION_BLOQUE_N, con N=0,1,2.
Se pide que, usando DMA y DAC se genere una forma de onda por la salida analógica de la LPC1769.
La forma de onda cambiará en función de una interrupción externa conectada a la placa de la siguiente manera:
• 1er interrupción: Forma de onda almacenada en bloque 0, con frecuencia de señal de 60[KHz].
• 2da interrupción: Forma de onda almacenada en bloque 1 con frecuencia de señal de 120[KHz].
• 3ra interrupción: Forma de onda almacenada en bloque 0 y bloque 2 (una a continuación de la otra) con frecuencia de señal de 450[KHz].
• 4ta interrupción: Vuelve a comenzar con la forma de onda del bloque 0.
En cada caso se debe utilizar el menor consumo de energía posible del DAC.

## 3. HECHO

Describa las formas de disparo o inicio de conversión del ADC del microcontrolador LPC1769.

**Rta:**
Estas formas de disparo se configuran con los bits 24 a 26 (START) del registro AD0CR del ADC en conjunto con el bit 16 (modo BURST).

Cuando el modo burst esta en 1 (activo) entonces los bits de START deben estar todos a 0, ademas el bit ADGINTEN del registro AD0INTEN debe estar a 0 para que no se lance una interrupcion y permita el funcionamiento continuo del ADC. Este modo hace que al termino de una conversion inicie otra, y se usa en conjunto con el DMA para que cuando termine la conversion hace un trigger para que guarde el dato.

Cuando trabajo en modo no burst entonces uso las otras combinaciones de los bits START, haciendo que se comience la conversion con distintas fuentes. Ademas para algunas de estas se debe especificar el flanco en el que ocurren, el cual se setea con el bit 27 EDGE del mismo registro de control.

La conversion comienza de la siguiente manera según los bits:
- 001 → manualmente cuando lo seteo
- 010 → cuando viene el flanco (bit 27) en el pin de EINT0
- 011 → cuando viene el flanco (bit 27) en el pin de CAP0.1
- 100 → cuando viene el flanco (bit 27) en el pin de MAT0.1 (no hace falta que el pin se configure como MAT0.1)
- 101 → cuando viene el flanco (bit 27) en el pin de MAT0.3 (no hace falta que el pin se configure como MAT0.3)
- 110 → cuando viene el flanco (bit 27) en el pin de MAT1.0 (no hace falta que el pin se configure como MAT1.0)
- 111 → cuando viene el flanco (bit 27) en el pin de MAT1.1 (no hace falta que el pin se configure como MAT1.1)

**___________________________________________________________________________**