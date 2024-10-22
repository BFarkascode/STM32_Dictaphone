/*
 *  Created on: Oct 24, 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Dictaphone
 *  Processor: STM32L053R8
 *  Program version: 1.1
 *  Source file: EXTIDriver_STM32L0x3.h
 *  Change history:
 *      This is a driver for external interrupts.
 *      It is followed by the definition of the callback function of each EXTI.
 *
 */

#ifndef INC_EXTIDRIVER_CUSTOM_H_
#define INC_EXTIDRIVER_CUSTOM_H_

#include "stm32l053xx.h"											//device specific header file for registers

//LOCAL CONSTANT

//LOCAL VARIABLE

//EXTERNAL VARIABLE
extern uint8_t log_audio_flag;
extern uint8_t send_audio_flag;
extern uint8_t DREQ_flag;

extern uint8_t blue_button_pushed;

//FUNCTION PROTOTYPES

void Push_button_Init(void);

#endif /* INC_EXTIDRIVER_CUSTOM_H_ */
