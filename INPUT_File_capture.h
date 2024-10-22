/*
 *  Created on: Aug 9, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_Dictaphone
 *  Processor: STM32L053R8
 *  Program version: 1.0
 *  Header file: SDcard_image_capture.h
 *  Change history:
 */

#ifndef INC_SDCARD_IMAGE_CAPTURE_H_
#define INC_SDCARD_IMAGE_CAPTURE_H_

#include "INPUT_I2SDriver_STM32L0x3.h"
#include "SDcard_diskio.h"
#include "stdlib.h"
#include "ff.h"

//LOCAL CONSTANT

//LOCAL VARIABLE
static uint8_t mono_frame_buf[256];

static uint32_t start_sector_of_file;

//EXTERNAL VARIABLE

extern char INPUT_SIDE_file_name[];
extern uint16_t I2S_frame_buf[];
extern uint8_t I2S_frame_buffer_half_full;
extern uint8_t I2S_frame_buffer_loaded;

extern uint8_t SDcard_readout_buf[];
extern uint16_t file_size_in_sectors;
extern uint8_t* SDcard_readout_ptr;

extern uint8_t blue_button_pushed;
extern uint8_t gen_file_no;

//FUNCTION PROTOTYPES

void SDcard_start(void);

void FILE_wav_create(void);
void FILE_Capture_audio(void);
void FILE_Get_start_sector(char readout_file_name[]);
void FILE_Read_single_sector(uint16_t offset);

#endif /* INC_SDCARD_IMAGE_CAPTURE_H_ */
