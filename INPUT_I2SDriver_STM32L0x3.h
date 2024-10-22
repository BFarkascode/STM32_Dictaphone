/*
 *  Created on: Sep 20, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_Dictaphone
 *  Processor: STM32L053R8
 *  Program version: 1.0
 *  Source file: I2SDriver_STM32L0x3.h
 *  Change history:
 *
 */

#ifndef INC_INPUT_I2SDRIVER_STM32L0X3_H_
#define INC_INPUT_I2SDRIVER_STM32L0X3_H_

#include "stdint.h"
#include "stm32l053xx.h"

//LOCAL CONSTANT

//LOCAL VARIABLE

//EXTERNAL VARIABLE
extern uint16_t I2S_frame_buf[];
extern uint8_t I2S_frame_buffer_loaded;
extern uint8_t I2S_frame_buffer_half_full;
extern uint16_t I2S_DMA_transfer_width;

//FUNCTION PROTOTYPES
void I2S_init(void);
void I2S_shutdown(void);

void DMAI2SInit(void);
void DMAI2SConfig(void);
void DMAI2SIRQPriorEnable(void);

#endif /* INC_INPUT_I2SDRIVER_STM32L0X3_H_ */
