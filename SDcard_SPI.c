/*
 *  Created on: Aug 7, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_Dictaphone
 *  Processor: STM32L053R8
 *  Program version: 1.1
 *  Source file: SDcard_SPI.c
 *  Change history:
 *
 *  v.1.1: Changed SPI speed to 500 kHz and 8 MHz
 *
 */

#include "SDcard_SPI.h"


//1)SERCOM SPI init at 8 MHz
void SPI1_w_o_DMA_8MHZ_init(void) {

	/*
	 * Set the SPI1 speed for 8 MHz
	 */

	//1)
	RCC->APB2ENR |= (1<<12);													//enable SPI1 interface (CHANGED)

	RCC->IOPENR |= (1<<0);														//PORTA clocking (CHANGED)
	RCC->IOPENR |= (1<<1);														//PORTB clocking (CHANGED)

	//2)
	GPIOA->MODER &= ~(1<<10);													//alternate function for PA5 (CHANGED)
	GPIOA->MODER |= (1<<11);													//alternate function for PA5 (CHANGED)
	GPIOA->MODER &= ~(1<<12);													//alternate function for PA6 (CHANGED)
	GPIOA->MODER |= (1<<13);													//alternate function for PA6 (CHANGED)
	GPIOA->MODER &= ~(1<<14);													//alternate function for PA7 (CHANGED)
	GPIOA->MODER |= (1<<15);													//alternate function for PA7 (CHANGED)


	GPIOA->AFR[0] &= ~(15<<20);													//high speed PA5 - AF0 - SCK (CHANGED)
	GPIOA->AFR[0] &= ~(15<<24);													//high speed PA6 - AF0 - MISO (CHANGED)
	GPIOA->AFR[0] &= ~(15<<28);													//high speed PA7 - AF0 - MOSI (CHANGED)

	GPIOA->OSPEEDR |= (3<<10);
	GPIOA->OSPEEDR |= (3<<12);
	GPIOA->OSPEEDR |= (3<<14);

	Delay_us(1);																//this delay is necessary, otherwise GPIO setup may freeze

	//3
	SPI1->CR1 |= (1<<2);														//Master mode
	SPI1->CR1 |= (1<<3);														//baud rate by diving the APB2 clock (division by 4)
	SPI1->CR1 &= ~(1<<4);														//baud rate by diving the APB2 clock (division by 4)
	SPI1->CR1 &= ~(1<<5);														//baud rate by diving the APB2 clock (division by 4)

	SPI1->CR2 &= ~(1<<4);														//Motorola mode
	SPI1->CR1 |= (1<<9);														//SSM mode is software
	SPI1->CR1 |= (1<<8);														//we put a HIGH value on the SSI

}


//2)SPI SD write
void SPI1_Master_SD_write(uint8_t *tx_buffer_ptr, uint16_t len) {

	/*
	 * This is a blocking SPI write function
	 */

//1)We reset the flags

	uint32_t buf_junk = SPI1->DR;												//we reset the RX flag

//2)We load the message into the SPI module

	while (len)
	{
		SPI1->DR = (volatile uint8_t) *tx_buffer_ptr++;							//we load the byte into the Tx buffer
		while(!((SPI1->SR & (1<<1)) == (1<<1)));								//wait for the TXE flag to go HIGH and indicate that the TX buffer has been transferred to the shift register completely
		while(!((SPI1->SR & (1<<0)) == (1<<0)));								//we wait for the RXNE flag to go HIGH, indicating that the command has been transferred successfully
		buf_junk = SPI1->DR;													//we reset the RX flag
		len--;
	}

	while(((SPI1->SR & (1<<0)) == (1<<0)));										//we check that the Rx buffer is indeed empty
	while(!((SPI1->SR & (1<<1)) == (1<<1)));									//wait until TXE is set, meaning that Tx buffer is also empty
	while((SPI1->SR & (1<<7)) == (1<<7));

}


//3)SPI SD read
void SPI1_Master_SD_read(uint8_t *rx_buffer_ptr, uint16_t len) {

	/*
	 * This is a blocking SPI read function
	 */

//1)We reset the flags

	uint32_t buf_junk = SPI1->DR;												//we reset the RX flag

//2)We load the message into the SPI module

	while (len) {
		SPI1->DR = 0xFF;														//we load a dummy command into the DR register
		while(!((SPI1->SR & (1<<0)) == (1<<0)));
		*rx_buffer_ptr = SPI1->DR;												//we dereference the pointer, thus we can give it a value
																				//we extract the received value and by proxy reset the RX flag
		rx_buffer_ptr++;														//we step our pointer within the receiving end
		len--;

	}

	while(((SPI1->SR & (1<<0)) == (1<<0)));										//we check that the Rx buffer is indeed empty
	while(!((SPI1->SR & (1<<1)) == (1<<1)));									//wait until TXE is set, meaning that Tx buffer is also empty
	while((SPI1->SR & (1<<7)) == (1<<7));

}


//4)SERCOM SPI init at 500 kHz
void SPI1_w_o_DMA_500KHZ_init(void) {

	/*
	 * Set the SPI1 speed for 500 kHz
	 */

	//1)
	RCC->APB2ENR |= (1<<12);													//enable SPI1 interface (CHANGED)

	RCC->IOPENR |= (1<<0);														//PORTA clocking (CHANGED)
	RCC->IOPENR |= (1<<1);														//PORTB clocking (CHANGED)

	//2)
	GPIOA->MODER &= ~(1<<10);													//alternate function for PA5 (CHANGED)
	GPIOA->MODER |= (1<<11);													//alternate function for PA5 (CHANGED)
	GPIOA->MODER &= ~(1<<12);													//alternate function for PA6 (CHANGED)
	GPIOA->MODER |= (1<<13);													//alternate function for PA6 (CHANGED)
	GPIOA->MODER &= ~(1<<14);													//alternate function for PA7 (CHANGED)
	GPIOA->MODER |= (1<<15);													//alternate function for PA7 (CHANGED)


	GPIOA->AFR[0] &= ~(15<<20);													//high speed PA5 - AF0 - SCK (CHANGED)
	GPIOA->AFR[0] &= ~(15<<24);													//high speed PA6 - AF0 - MISO (CHANGED)
	GPIOA->AFR[0] &= ~(15<<28);													//high speed PA7 - AF0 - MOSI (CHANGED)

	GPIOA->OSPEEDR |= (3<<10);
	GPIOA->OSPEEDR |= (3<<12);
	GPIOA->OSPEEDR |= (3<<14);

	Delay_us(1);																//this delay is necessary, otherwise GPIO setup may freeze

	//3
	SPI1->CR1 |= (1<<2);														//Master mode
	SPI1->CR1 |= (6<<3);														//baud rate by diving the APB2 clock (division by 128)
	SPI1->CR2 &= ~(1<<4);														//Motorola mode
	SPI1->CR1 |= (1<<9);														//SSM mode is software
	SPI1->CR1 |= (1<<8);														//we put a HIGH value on the SSI

}
