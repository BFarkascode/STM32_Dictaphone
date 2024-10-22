/*
 *  Created on: Sep 20, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_Dictaphone
 *  Processor: STM32L053R8
 *  Program version: 1.0
 *  Source file: I2SDriver_STM32L0x3.c
 *  Change history:
 *
 */

#include "INPUT_I2SDriver_STM32L0x3.h"

//1)
void I2S_init(void){

	/*
	 * Initialize the I2S peripheral and the connected GPIOs
	 * On an L053R8, only SPI2 can be set up for I2S.
	 * PB12 is WS (word select/side select), PB13 is clock,  PB15 is SD (data in/out),
	 *
	 */

	//1)Set GPIOs
	RCC->IOPENR |= (1<<1);													//PORTB clocking allowed

	GPIOB->MODER &= ~(1<<24);												//PB12 alternate - I2S WS
	GPIOB->MODER &= ~(1<<26);												//PB13 alternate - I2S SCK
	GPIOB->MODER &= ~(1<<30);												//PB15 alternate - DATA

	GPIOB->OSPEEDR |= (3<<24);												//PB12 very high speed
	GPIOB->OSPEEDR |= (3<<26);												//PB13 very high speed
	GPIOB->OSPEEDR |= (3<<30);												//PB15 very high speed

	GPIOB->AFR[1] &= ~(15<<16);												//PB12 - AF0
	GPIOB->AFR[1] &= ~(15<<20);												//PB13 - AF0
	GPIOB->AFR[1] &= ~(15<<28);												//PB15 - AF0

	//2)Enable SPI2 clocking

	RCC->APB1ENR |= (1<<14);

	//3)Set I2S up
	SPI2->CR2 |= (1<<0);													//DMA enabled on RX
	SPI2->I2SCFGR |= (1<<11);												//enable I2S mode for SPI2
	SPI2->I2SCFGR |= (3<<8);												//master Rx mode
																			//I2S Philips mode - 00 on bits 4 and 5
																			//CKPOL is kept LOW
	SPI2->I2SCFGR &= ~(1<<2);												//data length 24 bit (the mic we have gives a 24 bit I2S Philips output)
	SPI2->I2SCFGR |= (1<<1);												//data length 24 bit (the mic we have gives a 24 bit I2S Philips output)
	SPI2->I2SCFGR |= (1<<0);												//channel length 32-bit


	//4)Baud rate
	SPI2->I2SPR = 0x10B;													//value for 22kHz sampling at 32 MHz APB1

}

//2)
void I2S_shutdown(void){

	/*
	 * Shut off I2S
	 * Note: we need to reset the DMA's transfer counter since it does not reset during an interrupted DMA transfer
	 *
	 */

	DMA1_Channel6->CCR &= ~(1<<0);											//we disable the DMA channel

	while((SPI2->SR & (1<<1)) != (1<<1));									//stay in loop while TXE is not 1
	while((SPI2->SR & (1<<7)) == (1<<7));									//stay in loop while BSY is 1
	SPI2->I2SCFGR &= ~(1<<10);												//disable I2S

	DMA1_Channel6->CNDTR = I2S_DMA_transfer_width;

}

//3)
void DMAI2SInit(void){
	/* Initialize DMA
	 *
	 * What happens here? Well not much, we simly:
	 *
	 * 1)Enable clocking
	 * 2)Remap channels to be used
	 *
	 * Remapping is done by writing a specific sequence of bits to the CSELR register in the DMA. For which sequence defined what, check Table 51.
	 *
	 * Note: SPI2 runs I2S so we need to connect that to the DMA
	 *
	 * */

	//1)
	RCC->AHBENR |= (1<<0);														//enable DMA clocking
																				//clocking is directly through the AHB

	//2)
	DMA1_CSELR->CSELR |= (1<<21);												//DMA1_Channel6: will be requested at 4'b0010 for SPI2/I2S
}

//4)
void DMAI2SConfig(void){
	/*
	 * Configure the 6th DMA channel as SPI2 to transfer the I2S data
	 * We will capture the incoming data using the DMA
	 * Note: since this is going to lead to a constant flow of data, we should always time it to the data capture, otherwise we may misalign the data processing part during file capture.
	 *
	 */

	//1)
	DMA1_Channel6->CCR &= ~(1<<0);												//we disable the DMA channel

	//2)
	DMA1_Channel6->CCR |= (1<<1);												//we enable the IRQ on the transfer complete
	DMA1_Channel6->CCR |= (1<<2);												//we enable the IRQ on the half transfer
	DMA1_Channel6->CCR |= (1<<3);												//we enable the error interrupt within the DMA channel
	DMA1_Channel6->CCR &= ~(1<<4);												//we read form the peripheral
	DMA1_Channel6->CCR |= (1<<5);												//circular mode is on
																				//peripheral increment is not used
	DMA1_Channel6->CCR |= (1<<7);												//memory increment is is used
	DMA1_Channel6->CCR |= (1<<8);												//peri side data length is 16 bits
	DMA1_Channel6->CCR |= (1<<10);												//mem side data length is 16 bits
																				//priority level is kept at LOW
																				//mem-to-mem is not used
	//3)
	DMA1_Channel6->CPAR = (uint32_t) (&(SPI2->DR));								//we want the data to be extracted from the ADC's DR register

	DMA1_Channel6->CMAR = &I2S_frame_buf[0];									//this is the address (!) of the memory buffer we want to funnel data into

	//4)
	DMA1_Channel6->CNDTR |= (I2S_DMA_transfer_width<<0);								//we want to have an element burst of "transfer_width"
																				//Note: circular mode will automatically reset this register
}

//5)
void DMA1_Channel4_5_6_7_IRQHandler (void){
	/*
	 * Note: we are running the DMA in circular mode
	 *
	 * */

	//1)
	if ((DMA1->ISR & (1<<21)) == (1<<21)) {										//if we had the full transmission triggered on channel 6

		I2S_frame_buffer_loaded = 1;

	} else if ((DMA1->ISR & (1<<22)) == (1<<22)){								//if we had half transmission on channel 6

		I2S_frame_buffer_half_full = 1;

	} else if ((DMA1->ISR & (1<<3)) == (1<<3)){									//if we had an error trigger on channel 6

		printf("DMA transmission error!");
		while(1);

	} else {
		//do nothing
	}

	DMA1->IFCR |= (1<<20);														//we remove all the interrupt flags from channel 6

}

//6)
void DMAI2SIRQPriorEnable(void) {
	/*
	 * We call the two special CMSIS functions to set up/enable the IRQ.
	 *
	 * */
	NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 1);									//IRQ priority for channel 6
	NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);										//IRQ enable for channel 6
}
