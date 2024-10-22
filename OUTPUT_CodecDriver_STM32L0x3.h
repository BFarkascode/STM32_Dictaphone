/*
 *  Created on: Oct 1, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_Dictaphone
 *  Processor: STM32L053R8
 *  Program version: 1.0
 *  Source file: CodecDriver_STM32L0x3.h
 *  Change history:
 *
 */

#ifndef INC_OUTPUT_CODECDRIVER_STM32L0X3_H_
#define INC_OUTPUT_CODECDRIVER_STM32L0X3_H_

#include "SDcard_SPI.h"

//LOCAL CONSTANT
//---MODE---//
static uint8_t SCI_READ_MODE[2] 			= {0x03, 0x00};
static uint8_t SCI_WRITE_MODE_NORMAL[4] 	= {0x02, 0x00, 0x08, 0x00};			//this is the reset state, minus the mic
static uint8_t SCI_WRITE_MODE_SOFT_reset[4] = {0x02, 0x00, 0x08, 0x04};
static uint8_t SCI_WRITE_MODE_CANCEL[4] 	= {0x02, 0x00, 0x08, 0x08};
											//bit 0 is DIFF - generates stereo by inverting the left channel
											//bit 2 is software reset
											//bit 3 is cancel decoding
											//bit 5 is SDI test
											//bit 6 is STREAM mode
											//bit 9 is BIT ORDER
											//bit 11 is SDINEW
											//bit 15 is CLK_RANGE - clock divider on the clock input, 1 means division by 2
											//Note: SM_SDINEW must be kept HIGH to allow functioning!
											//Note: it is possible to salvage an extra pin if XCS and XDCS are merged within the mode register

//---STATUS---//
static uint8_t SCI_READ_STATUS[2]	  	  	= {0x03, 0x01};						//reset to 0x0040
																				//Note: follow this up with a read command

//---CLOKF---//
static uint8_t SCI_WRITE_CLOCKF[4]		  	= {0x02, 0x03, 0x88, 0x00};			//clocking up to 42 MHz
static uint8_t SCI_READ_CLOCKF[2]		  	= {0x03, 0x03};

//---AUDATA---//
//static uint8_t SCI_READ_AUDATA[2]	  	 	= {0x03, 0x05};						//decoded audio data properties
																				//[15:1] is samplerate divided by 2
																				//[0] is 0 for mono, 1 for stereo

//---HDATA---//
//static uint8_t SCI_READ_HDAT0[2]	  	  	= {0x03, 0x08};						//file header information
//static uint8_t SCI_READ_HDAT1[2]	  	  	= {0x03, 0x09};						//file header information
																				//for WAV, this will be 0x7665
																				//the two registers above store the properties for the ongoing file decoding

//---DECODE TIME---//
//static uint8_t SCI_READ_DECODETIME[2]	  	 = {0x03, 0x04};

//---VOLUME---//
static uint8_t SCI_WRITE_VOLUME[4]	  	 	 = {0x02, 0x0B, 0x00, 0xFE};		//each LSB accounts for -0.5 dB of volume change
																				//0x28 should be 20 dB, which is 10x gain
																				//we only use the left channel

//---POWER DOWN---//
static uint8_t SCI_DISABLE_ANALOG_DRIVERS[4] = {0x02, 0x0B, 0xFF, 0xFF};

static uint8_t SCI_READ_ENDFILLBYTE[1]	  	  = {0x03, 0x08};					//file header information
																				//when a file is finished, 2052 bytes of endfillbyte should be sent to the chip
																				//followed by an SCI_MODE set to CANCEL
																				//followed by another 32 bytes of endfillbyte
																				//read SCI_MODE and repeat sending 32 bytes of endfill until CANCEL is gone

//---Wav header---//

static uint8_t wav_header_w_padding[64] = {0x52, 0x49, 0x46, 0x46,
						 0xFF, 0xFF, 0xFF, 0xFF,		//WAV mode indefinitely
						 0x57, 0x41, 0x56, 0x45,
						 0x66, 0x6D, 0x74, 0x20,
						 0x10, 0x00, 0x00, 0x00,
						 0x01, 0x00,
						 0x01, 0x00,
						 0x22, 0x56, 0x00, 0x00,
						 0x44, 0xAC, 0x00, 0x00,
						 0x02, 0x00,
						 0x10, 0x00,
						 0x64, 0x61, 0x74, 0x61,
						 0xFF, 0xFF, 0xFF, 0xFF,
						 0x10, 0x00, 0x00, 0x00,
						 0x10, 0x00, 0x00, 0x00,
						 0x10, 0x00, 0x00, 0x00,
						 0x10, 0x00, 0x00, 0x00,
						 0x10, 0x00, 0x00, 0x00};		//WAV mode indefinitely

//LOCAL VARIABLE

static uint8_t dummy[2] = { 0xFF, 0xFF };
static uint8_t Codec_readout_buf[2] = { 0, 0 };  							//this buffer stores the readout of each SCI command
										  	  	    						//readout is always 16-bits, plus the dummy 0xFF at the start? (To be checked)

static uint8_t endfillbyte[1] = {0x00};										//endfillbyte is assumed to be 0x00

//EXTERNAL VARIABLE
extern uint8_t DREQ_flag;
extern uint8_t SDcard_readout_buf[];
extern uint16_t file_size_in_sectors;
extern uint8_t* SDcard_readout_ptr;

//FUNCTION PROTOTYPES

void Codec_GPIO_Config(void);
void Codec_Select_SCI_bus(void);
void Codec_DeSelect_SCI_bus(void);
void Codec_Select_SDI_bus(void);
void Codec_DeSelect_SDI_bus(void);
void Codec_SCI_Config(uint8_t codec_volume);

void Codec_Sinetest_SDI(uint8_t codec_volume);
void Codec_Debug_SCI(void);

void Codec_Start_Decoding(void);
void Codec_Finish_Decoding(void);

#endif /* INC_OUTPUT_CODECDRIVER_STM32L0X3_H_ */
