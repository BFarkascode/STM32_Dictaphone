/*
 *  Created on: Oct 25, 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Dictaphone
 *  Processor: STM32L053R8
 *  Program version: 1.1
 *  Source file: EXTIDriver_STM32L0x3.c
 *  Change history:*
 *
 *
 *v.1.0
 *	Below is the definition and the setup of the EXTI channels.
 *	Callback function names are taken from the device startup file.
 *	Current version uses PC13 and EXTI13 to engage the blue push button on the L053R8 nucelo board.
 *
 *v.1.1
 * Changed function name
 */


#include "EXTIDriver_STM32L0x3.h"

void Push_button_Init(void){

	/*
	 * Activate the two push buttons on PC13 and PA10
	 *
	 */

	//1)PC13
		//Note: below we use PC13 "blue push button" and thus, EXTI13
	RCC->IOPENR |= (1<<2);						//PORTC clocking allowed
	GPIOC->MODER &= ~(1<<26);					//PC13 input - EXTI13
	GPIOC->MODER &= ~(1<<27);					//PC13 input - EXTI13
												//we use push-pull

	GPIOC->PUPDR |= (1<<26);					//pullup
	GPIOC->PUPDR &= (1<<27);

	//2)
	RCC->APB2ENR |= (1<<0);						//SYSCFG enable is on the APB2 enable register

	//3)PC13
	SYSCFG->EXTICR[3] |= (1<<5);				//since we want to use PC13, we write 0010b to the [7:4] position of the fourth element of the register array - SYSCFG_EXTICR4
												//Note: EXTICR is uint32_t volatile [4], not just a 32-bit register (similar to AFR)
	//4)PC13
	EXTI->IMR |= (1<<13);						//EXTI13 IMR unmasked
												//we leave the request masked
	EXTI->RTSR |= (1<<13);						//we enable the rising edge
	EXTI->FTSR |= (1<<13);						//we enable the falling edge


	//5)PA10
		//Note: below we use PA10 for an external button and thus, EXTI10
	RCC->IOPENR |= (1<<0);						//PORTA clocking allowed
	GPIOA->MODER &= ~(1<<20);					//PA10 input - EXTI10
	GPIOA->MODER &= ~(1<<21);					//PA10 input - EXTI10

	GPIOA->PUPDR |= (1<<20);					//pullup
	GPIOA->PUPDR &= (1<<21);

	//6)PA10
	SYSCFG->EXTICR[2] &= ~(15<<8);				//since we want to use PA10, we write 0000b to the [11:8] position of the third element of the register array - SYSCFG_EXTICR3

	//7)PA10
	EXTI->IMR |= (1<<10);						//EXTI10 IMR unmasked
												//we leave the request masked
	EXTI->RTSR |= (1<<10);						//we enable the rising edge
	EXTI->FTSR &= ~(1<<10);						//we disable the falling edge

	//5)
	NVIC_SetPriority(EXTI4_15_IRQn, 1);			//we set the interrupt priority as 1 so it will be lower than the already running DMA IRQ for the SPI
	NVIC_EnableIRQ(EXTI4_15_IRQn);
}




//2)We define the callback function
void EXTI4_15_IRQHandler (void) {

	/*
	 * EXTI handler
	 */

	//1)
	if ((EXTI->PR & (1<<13)) == (1<<13)) {				//blue push button to start recording

		//Note: the blue push button has a pullup and debouncing

		Delay_ms(5);									//we implement a crude debounce function here on the blue push button anyway

		if(((GPIOC->IDR & (1<<13)) == 0)){

			blue_button_pushed = 1;						//the blue button push is technically redundant to the log_audio_flag, though I leave them separate for clarity's sake
														//flag should also help with the removal of blocking while loops, if necessary

		} else if(((GPIOC->IDR & (1<<13)) == (1<<13))){

			blue_button_pushed = 0;

		} else {

			//do nothing

		}

		log_audio_flag = 1;
		EXTI->PR |= (1<<13);							//we reset the IRQ connected to the EXTI13 by writing to the pending bit

	} else if ((EXTI->PR & (1<<10)) == (1<<10)) {		//this is for sending the audio file to the codec

		send_audio_flag = 1;
		EXTI->PR |= (1<<10);							//we reset the IRQ connected to the EXTI13 by writing to the pending bit

	} else if ((EXTI->PR & (1<<4)) == (1<<4)) {			//this is for the DREQ line

		DREQ_flag = 1;
		EXTI->PR |= (1<<4);								//we reset the IRQ connected to the EXTI13 by writing to the pending bit

	} else {

		//do nothing

	}

}

