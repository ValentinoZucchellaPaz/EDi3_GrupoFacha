## 1. HECHO
Mediante la utilización del microcontrolador LPC1769, implemente el software necesario para cumplir con los siguientes requerimientos:
1. Usar el hardware GPDMA para transferir datos en palabras de 2 bytes desde la primera mitad de la SRAM del banco 1 hacia la segunda mitad de la SRAM del banco 0
2. Una vez finalizada la transferencia, mediante la interrupción por cuenta terminal del GPDMA, se debe encender un LED indicador conectado a P0.22.
3. Determine usted Cuántos bytes fueron transferidos

## 2. HECHO
Por un pin del ADC del microcontrolador LPC1769 ingresa una tensión de rango dinámico 0 a 3,3[v] proveniente de un sensor de temperatura. Debido a la baja tasa de variación de la señal, se pide tomar una muestra cada 30[s]. Pasados los 2[min] se debe promediar las últimas 4 muestras y en función de este valor, tomar una decisión sobre una salida digital de la placa:
- Si el valor es <1 [V] colocar la salida en 0 (0[V]).
- Si el valor es >= 1[V] y <=2[V] sacar una señal intermitente de F= ????
- Si el valor es > 2[V] colocar la salida en 1 (3,3[V]).
CCLOCK= 100Mhz

## 3. HECHO
Describa el funcionamiento de la función BIAS del DAC, como habilitarla y demás consideraciones a tener en cuenta.

**RTA**
El bit BIAS del DAC permite cambiar entre el modo de alto consumo y bajo consumo.

Con el bit en 0 se está en alto consumo, permitiendo un tiempo de establecimiento de la señal convertida de 1us, y hace que el DAC consuma hasta 700uA. En conjunto se puede tener una tasa de conversion de 1MHz máximo.

Con el bit en 1 se está en bajo consumo, permite tiempo de establecimiento de la señal convertida de 2.5us, y y hace que el DAC consuma hasta 350uA. En conjunto se puede tener una tasa de conversion de 400KHz máximo.

En resumen sacrificio velocidad de conversion por consumo de corriente.