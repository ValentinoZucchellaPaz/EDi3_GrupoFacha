/*
 * Copyright 2022 NXP
 * NXP confidential.
 * This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms.  By expressly accepting
 * such terms or by downloading, installing, activating and/or otherwise using
 * the software, you are agreeing that you have read, and that you agree to
 * comply with and are bound by, such license terms.  If you do not agree to
 * be bound by the applicable license terms, then you may not retain, install,
 * activate or otherwise use the software.
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include <stdio.h>

volatile uint32_t i = 100;
void sysTick_Initialize(uint32_t ticks){
	SysTick -> CTRL = 0;

	SysTick -> LOAD = ticks - 1;

	SysTick -> VAL = 0;

	SysTick -> CTRL |= (1<<2) | (1<<1) | 1;
}
int main(void) {
    sysTick_Initialize(1000000);
    LPC_GPIO0->FIODIR |= (1<<22);
	LPC_GPIO0->FIOSET |= (1<<22);
	while(1){};
    return 0;
}
void SysTick_Handler(void){
	i--;
	if (i==0) {
		i=100;
		LPC_GPIO0->FIOPIN ^= (1<<22);
	}
}
