/*
 *  Created on: Aug 7, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_Dictaphone
 *  Processor: STM32L053R8
 *  Program version: 1.0
 *  Header file: SDcard_SPI.h
 *  Change history:
 */

#ifndef INC_SDCARD_SPI_H_
#define INC_SDCARD_SPI_H_

#include "stdint.h"
#include "stm32l053xx.h"
#include "ClockDriver_STM32L0x3.h"

//LOCAL CONSTANT

//LOCAL VARIABLE

//EXTERNAL VARIABLE

//FUNCTION PROTOTYPES
void SPI1_w_o_DMA_500KHZ_init(void);
void SPI1_w_o_DMA_8MHZ_init(void);
void SPI1_Master_SD_write(uint8_t *tx_buffer_ptr, uint16_t len);
void SPI1_Master_SD_read(uint8_t *rx_buffer_ptr, uint16_t len);

#endif /* INC_SDCARD_SPI_H_ */
