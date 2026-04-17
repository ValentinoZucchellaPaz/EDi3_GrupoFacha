## Ejercicio de Repaso: Sistema de Monitoreo Dual con Registro de Actividad

**Contexto**: Se requiere diseñar un sistema de monitoreo para el microcontrolador LPC1769 que alterne entre un modo de visualización de alta resolución y un modo de alerta de bajo consumo,
registrando la duración efectiva de cada sesión de uso.

### 1. Periféricos y Conectividad

- Salidas: 8 LEDs conectados en el puerto P1.0 a P1.7 .
- Entrada Analógica: Señal de entrada por el canal ADC0.3 (12 bits).
- Entrada Digital: Pulsador configurado como interrupción externa EINT0 (flanco de bajada).
- Captura: Pin CAP1.0 conectado físicamente al mismo pulsador de EINT0 (para registrar el evento de cambio).

### 2. Modos de Operación

El sistema debe alternar entre dos modos mediante la pulsación del botón:

#### Modo A: Vúmetro en Tiempo Real

- Muestreo: Disparado por la interrupción del SysTick cada 50 ms.
- Visualización: Los 8 LEDs representan el nivel de la señal analógica (Barra de niveles).
- Lógica: El valor de 12 bits debe escalarse para representar 8 niveles de intensidad.

#### Modo B: Alerta de Umbral (Low Power Monitoring)

- Muestreo: Disparado por la interrupción del Timer 0 cada 1 segundo (Match Interrupt).
- Visualización: Solo se utilizan 3 LEDs ( P1.0 , P1.1 , P1.2 ) para indicar niveles críticos (Bajo, Medio, Alto).
- Lógica: El sistema solo actualiza los LEDs una vez por segundo, ignorando variaciones rápidas.

### 3. Registro de Tiempo Efectivo (Módulo de Captura)

Utilice el Timer 1 configurado con una resolución de 1 segundo para medir la permanencia en cada modo:
El Timer debe correr continuamente desde el inicio del programa.
Ante cada interrupción por Capture ( CAP1.0 ):

1. Se debe leer el valor actual del registro de captura (CR0).
2. Se debe calcular el tiempo transcurrido (Delta) restando el valor capturado actual menos el valor capturado en el cambio anterior.
3. El resultado (tiempo efectivo que duró el modo que acaba de finalizar) debe guardarse en un array circular en memoria (10 posiciones).

### 4. Requerimientos de Software

Al cambiar de modo, se deben deshabilitar las fuentes de interrupción que no correspondan al modo activo (evitar disparos de ADC innecesarios).
El código debe ser modular, utilizando funciones de inicialización independientes para cada periférico.
