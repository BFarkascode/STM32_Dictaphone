/*
 *  Created on: Oct 1, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_Dictaphone
 *  Processor: STM32L053R8
 *  Program version: 1.0
 *  Source file: CodecDriver_STM32L0x3.c
 *  Change history:
 *
 */


//Codec driver for the VS1053B chip
//control and data transfer done using SPI


#include <OUTPUT_CodecDriver_STM32L0x3.h>


//1
void Codec_GPIO_Config(void){
	/*
	 * Configure the additional GPIO pins for the VS1053B
	 * XCS is active LOW and selects the SCI command bus - output on the MCU with pullup
	 * XDCS is active LOW and selects the SDI data bus - output on the MCU with pullup
	 * DREQ is LOW when the chip is busy or when the SDI bus FIFO is too full - input on the MCU with EXTI
	 *
	 */

	//XCS line config - PA0
	  RCC->IOPENR |= (1<<0);																						//PORTA clocking
	  GPIOA->MODER |= (1<<0);																						//GPIO output for PA0
	  GPIOA->MODER &= ~(1<<1);																						//GPIO output for PA0
	  GPIOA->PUPDR |= (1<<0);																						//pullup on PA0
	  GPIOA->PUPDR &= ~(1<<1);																						//pullup on PA0

	//XDCS line config - PA1
	  GPIOA->MODER |= (1<<2);																						//GPIO output for PA1
	  GPIOA->MODER &= ~(1<<3);																						//GPIO output for PA1
	  GPIOA->PUPDR |= (1<<2);																						//pullup on PA1
	  GPIOA->PUPDR &= ~(1<<3);																						//pullup on PA1

	//DREQ line config - PA4
	  GPIOA->MODER &= ~(1<<8);																						//GPIO input for PA4
	  GPIOA->MODER &= ~(1<<9);																						//GPIO input for PA4

	  RCC->APB2ENR |= (1<<0);																						//SYSCFG enable is on the APB2 enable register
	  SYSCFG->EXTICR[1] &= ~(15<<0);																				//since we want to use PA4, we write 0000b to the [3:0] position of the second element of the register array - SYSCFG_EXTICR4
	  EXTI->IMR |= (1<<4);																							//EXTI4 IMR unmasked
	  EXTI->RTSR &= ~(1<<4);																						//we disable the rising edge
	  EXTI->FTSR |= (1<<4);																							//we enable the falling edge

	  Delay_us(1);																									//this delay is necessary, otherwise GPIO setup may freeze

//	  NVIC_SetPriority(EXTI4_15_IRQn, 1);																			//we set the interrupt priority as 1 so it will be lower than the already running DMA IRQ for the SPI
//	  NVIC_EnableIRQ(EXTI4_15_IRQn);

	  Codec_DeSelect_SCI_bus();
	  Codec_DeSelect_SDI_bus();
}

/*
//Note: the EXTI handler is already set for EXTI13. We added the DREQ line handling into that function - see EXTIDriver source file
*/

//2)
void Codec_Select_SCI_bus(void){
	/*
	 * Activate SCI bus
	 * DREQ is LOW when the chip is busy or when the SDI bus FIFO is too full - input on the MCU with EXTI
	 * We MUST not proceed with SCI commands unless DREQ is HIGH indicating that the chip is idle
	 *
	 */

	  GPIOA->BSRR |= (1<<16);																						//drive XCS pin LOW

	  while(!DREQ_flag);																							//we wait until DREQ flag goes HIGH (and DREQ LOW), indicating an empty FIFO or a successful command execution
	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  										//this is mostly here to avoid SCI command overrun

	  DREQ_flag = 0;																								//at the start of SCI command, DREQ flag should be checked and set to 0 if it is 1
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 										//DREQ goes HIGH if we had a successful SCI command or the SDI FIFO is empty

}

//3)
void Codec_DeSelect_SCI_bus(void){
	/*
	 * Deactivate SCI bus
	 *
	 */

	  GPIOA->BSRR |= (1<<0);																						//drive XCS pin HIGH

}

//4)
void Codec_Select_SDI_bus(void){
	/*
	 * Activate SDI bus
	 * DREQ is LOW when the chip 2048 byte FIFO is full. Once DREQ is triggered, it stays as such until the FIFO is empty.
	 * Ideally all SDI actions should occur without tripping DREQ
	 *
	 */

	  GPIOA->BSRR |= (1<<17);																						//drive XDCS pin LOW

}

//5)
void Codec_DeSelect_SDI_bus(void){
	/*
	 * Deactivate SDI bus
	 *
	 */

	  GPIOA->BSRR |= (1<<1);																						//drive XDCS pin HIGH

}

//6)
void Codec_SCI_Config(uint8_t codec_volume){
	/*
	 * Configure the SCI bus elements in the codec.
	 *
	 * Note: here we stay at 400 kHz with the SPI until the very end to allow register readout. We do change SPI speed eventually.
	 *
	 */

	SPI1->CR1 &= ~(1<<6);

	SPI1_w_o_DMA_500KHZ_init();																						//init should be done with the 500 kHz SPI clock to set the clocking properly

	SPI1->CR1 |= (1<<6);																							//SPI enabled. SCK is enabled

	//0)software reset to wipe internal counters (such as DECODE_TIME or HDAT or AUDATA)
	//Note: SDI test mode needs a hardware reset, software reset isn't enough
	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_WRITE_MODE_SOFT_reset[0], 4);															//normal mode should be the reset value, minus the mic activated

	Codec_DeSelect_SCI_bus();

	//1)here we read the STATUS register
	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_READ_STATUS[0], 2);																	//commands are always a direction (write or read) followed by an address

	SPI1_Master_SD_read(&Codec_readout_buf[0], 2);

	Codec_DeSelect_SCI_bus();																						//Note: it is recommended to cycle the XCS pin after each SCI command

	//2)here we change the internal clocking from 12 MHz to 42 MHz (3.5x multiplier)
	//Note: since the SPI frequency for SDI/SCI (WRITE) is maximum CLKI/4 and for SCI (READ) is CLKI/7, we need to clock the chip faster to deal with higher SPI speed
	//This setup should allow us to write to the chip at 8 MHz...but WILL NOT allow to SCI (READ) at 8 MHz!!!
	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_WRITE_CLOCKF[0], 4);

	Codec_DeSelect_SCI_bus();

	//3)here we select the mode of the chip
	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_WRITE_MODE_NORMAL[0], 4);																//normal mode should be the reset value, minus the mic activated

	Codec_DeSelect_SCI_bus();

	//4)here we set the volume
	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_WRITE_VOLUME[0], 4);																	//reset value is full volume

	Codec_DeSelect_SCI_bus();

	//5)set volume
	Codec_Select_SCI_bus();

	SCI_WRITE_VOLUME[2] = codec_volume;																				//0x00 is maximum volume, 0xFE is minimum. 0xFF forces analog shutdown

	SPI1_Master_SD_write(&SCI_WRITE_VOLUME[0], 4);

	Codec_DeSelect_SCI_bus();

	//6)Increase SPI speed
	SPI1->CR1 |= (1<<6);																							//SPI disabled. SCK is enabled

	SPI1_w_o_DMA_8MHZ_init();

	SPI1->CR1 |= (1<<6);																							//SPI enabled. SCK is enabled

}

//7) DEBUG FUNCTION!
void Codec_Sinetest_SDI(uint8_t codec_volume){

	/*
	 * Below we have a function to generate a sine wave on the VS1053
	 * We also have volume control included for testing purposes
	 * Both SCI and SDI busses are running at full speed (here 8 MHz) after clock adjustment
	 *
	 * Note: the VS1053 should be hard reset with the button before this command is sent.
	 * Note: if the SCI readout is used, the SPI clocking should be kept at 400 kHz!
	 *
	 */

	//1)command lines to be sent are below
	uint8_t SCI_WRITE_MODE_SDI_test[4] 	= {0x02, 0x00, 0x08, 0x20};													//we select SDI_test mode
																													//Note: SM_SDINEW must be kept HIGH to allow functioning!

	uint8_t SDI_Start_sine_test[8] = {0x53, 0xEF, 0x6e, 0x7E , 0x00, 0x00, 0x00, 0x00};
	//Note: the 4th MSB defines the sine wave type. Here it will be a 22050 Hz sampled 5168 Hz sine wave

	uint8_t SDI_End_sine_test[8] = {0x45, 0x78, 0x69, 0x74 , 0x00, 0x00, 0x00, 0x00};

	//2)enable SPI at the proper speed - 400 kHz
	SPI1->CR1 &= ~(1<<6);;																							//SPI disabled. SCK is enabled

	SPI1_w_o_DMA_500KHZ_init();																						//init should be done with the 500 kHz SPI clock to set the clocking properly

	SPI1->CR1 |= (1<<6);																							//SPI enabled. SCK is enabled

	//3)Change CLKI to 42 MHz
	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_WRITE_CLOCKF[0], 4);

	Codec_DeSelect_SCI_bus();

	//4)Increase SPI speed
	SPI1->CR1 &= ~(1<<6);																							//SPI disabled. SCK is enabled

	SPI1_w_o_DMA_8MHZ_init();

	SPI1->CR1 |= (1<<6);																							//SPI enabled. SCK is enabled

	//5)here we select the mode of the chip (with optional readout) - SCI bus

	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_WRITE_MODE_SDI_test[0], 4);															//we set the

	Codec_DeSelect_SCI_bus();

#ifdef SCI_readout_used
	//Note: this part only works if SPI clocks at 400 kHz!

	SPI1->CR1 &= ~(1<<6);												//SPI disabled. SCK is enabled

	SPI1_w_o_DMA_400KHZ_init();

	SPI1->CR1 |= (1<<6);

	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_READ_MODE[0], 2);

	SPI1_Master_SD_read(&Codec_readout_buf[0], 2);

	Codec_DeSelect_SCI_bus();
#endif

	//6)here we select the volume of the output - SCI bus
	Codec_Select_SCI_bus();

	SCI_WRITE_VOLUME[2] = codec_volume;																				//0x00 is maximum volume, 0xFE is minimum. 0xFF forces analog shutdown

	SPI1_Master_SD_write(&SCI_WRITE_VOLUME[0], 4);

	Codec_DeSelect_SCI_bus();

	//7)here we select the SDI test command - SDI (!!!) bus
	Codec_Select_SDI_bus();

	SPI1_Master_SD_write(&SDI_Start_sine_test[0], 8);

	Codec_DeSelect_SDI_bus();

	//8) Disable SPI
	SPI1->CR1 &= ~(1<<6);																							//SPI disabled. SCK is disabled

}


//8) DEBUG FUNCTION!
void Codec_Debug_SCI(void){

	/*
	 * This function is to check the register values of a decoding.
	 * It should shows the decode time ticking changing, HDAT1 the file format and AUDATA the decoded output sampling rate (plus stereo/mono)
	 * Endfillbyte should also be extracted if it is not known already.
	 * SPI stays at 400 kHz to enable SCI read.
	 *
	 */

	DREQ_flag = 1;																									//should be set as 1 before debugging!

	//slow speed, need to read SCI
	SPI1->CR1 &= ~(1<<6);;																							//SPI disabled. SCK is disabled

	SPI1_w_o_DMA_500KHZ_init();

	SPI1->CR1 |= (1<<6);


	//---AUDATA---//
	uint8_t SCI_READ_AUDATA[2]	  	 	= {0x03, 0x05};																//decoded audio data properties
																																//[15:1] is samplerate divided by 2
																																//[0] is 0 for mono, 1 for stereo

	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_READ_AUDATA[0], 2);																	//commands are always a direction (write or read) followed by an address

	SPI1_Master_SD_read(&Codec_readout_buf[0], 2);																	//0x5622 for 22050 Hz sample rate, mono
																													//0xAC45 for 44100 Hz stereo

	Codec_DeSelect_SCI_bus();


	//---HDAT---//
	uint8_t SCI_READ_HDAT0[2]	  	  	= {0x03, 0x08};																//file header information
	uint8_t SCI_READ_HDAT1[2]	  	  	= {0x03, 0x09};																//file header information
																													//for WAV, this will be 0x7665
																													//the two registers above store the properties for the ongoing file decoding

	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_READ_HDAT0[0], 2);																	//commands are always a direction (write or read) followed by an address

	SPI1_Master_SD_read(&Codec_readout_buf[0], 2);																	//seems to be 0x0000 on start, will be measured during conversion

	Codec_DeSelect_SCI_bus();																						//Note: it is recommended to cycle the XCS pin after each SCI command

	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_READ_HDAT1[0], 2);																	//commands are always a direction (write or read) followed by an address

	SPI1_Master_SD_read(&Codec_readout_buf[0], 2);																	//for a WAV file, this should be 0x7665

	Codec_DeSelect_SCI_bus();																						//Note: it is recommended to cycle the XCS pin after each SCI command


	//---DECODE TIME---//
	//Note: this shows decoding time in SECONDS!
	//Decode time is only reset if the chip os software/hardware reset
	//This means that in order to have a value here, at least 1 second worth of data must be streamed over!

	uint8_t SCI_READ_DECODETIME[2]	  	 = {0x03, 0x04};

	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_READ_DECODETIME[0], 2);																//commands are always a direction (write or read) followed by an address

	SPI1_Master_SD_read(&Codec_readout_buf[0], 2);																	//currently 0x0000, will be calculated during conversion

	Codec_DeSelect_SCI_bus();

	//---VOLUME---//
	//REad the volume value on teh two channels

	uint8_t SCI_READ_VOLUME[2]	  	 	= {0x03, 0x0B};																//decoded audio data properties

	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_READ_VOLUME[0], 2);

	SPI1_Master_SD_read(&Codec_readout_buf[0], 2);

	Codec_DeSelect_SCI_bus();

	//---ENDFILLBYTE---//
	uint8_t SCI_WRITE_WRAMADDR[4] 		= {0x02, 0x07, 0x1e, 0x06};													//we write to WRAM address 0x1e06
	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 												//changing the last two LSBs reads out another parameter
	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 												//Example: 0x1e29 is resynch and will be 0x7FFF

	uint8_t SCI_READ_WRAM[2]		    = {0x03, 0x06};																//we read out WRAM value from WRAM address

	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_WRITE_WRAMADDR[0],4);																	//send the address to read out from memory

	Codec_DeSelect_SCI_bus();

	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_READ_WRAM[0], 2);

	SPI1_Master_SD_read(&Codec_readout_buf[0], 2);

	endfillbyte[0] = Codec_readout_buf[1];																			//endfillbyte should be the LSB of the 16-bits read
	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 												//endfillbyte seems to be 0

	Codec_DeSelect_SCI_bus();

}

//10
void Codec_Start_Decoding(void){

	/*
	 * This is the function to send the audio data to the codec
	 * Note: the codec and the SDcard are on the same SPI bus, so we can't read and write at the same time. We need to alternate between reading out from the SDcard and writing to the codec.
	 * Note: this is a BLOCKING function!
	 *
	 */

	 //we preload the readout (ping-pong) buffer
	 SDcard_readout_ptr = &SDcard_readout_buf[0];
	 FILE_Read_single_sector(0);
	 SDcard_readout_ptr = &SDcard_readout_buf[512];
	 FILE_Read_single_sector(1);

	 while(!DREQ_flag);																								//we reset DREQ (should be left set after the last SCI command)
	 DREQ_flag = 0;

	 uint16_t sector_cnt = 0;																						//count through the sectors of the file being decoded
	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 										//Note: we have "one plus" sectors since we already have 44 bytes of header

	 while(sector_cnt <= (file_size_in_sectors + 1)){																//we do the data transfer as long as DREQ is not tripped
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	//if DREQ is tripped, we need to wait until the FIFO becomes empty enough again to accept data

		 //transfer the head of the ping-pong buffer
		 for(uint8_t i = 0; i < 16; i++){																			//we send the first sector in 32 byte chunks to the codec

			Codec_Select_SDI_bus();

			if(DREQ_flag == 0){

				//If DREQ flag is low, it means that that the FIFO should still have enough place so business as usual

				SPI1_Master_SD_write(&SDcard_readout_buf[(i*32)], 32);												//we transfer 32 bytes from our readout ping-pong buffer

				Codec_DeSelect_SDI_bus();

			} else {

				//If the DREQ flag is tripped, we need to wait until DREQ goes back HIGH (the input DREQ, NOT the flag) indicating an "empty enough" FIFO
				//We will keep on polling the PA4 pin - the DREQ input pin - for a HIGH value
				while(((GPIOA->IDR & (1<<4)) != (1<<4)));															//we stay in the loop until PA4 is polled as HIGH
				DREQ_flag = 0;																						//then we reset the DREQ flag

				SPI1_Master_SD_write(&SDcard_readout_buf[(i*32)], 32);												//and carry on

				Codec_DeSelect_SDI_bus();

			}

		 }

		 SDcard_readout_ptr = &SDcard_readout_buf[0];
		 FILE_Read_single_sector(sector_cnt + 2);																	//we load the third sector in place of the first one

		 //transfer the tail of the ping-pong buffer
		 //this is the same as before, just on the tail on the ping-pong buffer
		 for(uint8_t j = 0; j < 16; j++){																			//we send the second sector in 32 byte chunks to the codec

			Codec_Select_SDI_bus();


			if(DREQ_flag == 0){

				SPI1_Master_SD_write(&SDcard_readout_buf[512 + (j*32)], 32);										//steps of 32 on the second half of the buffer

				Codec_DeSelect_SDI_bus();

			} else {

				while(((GPIOA->IDR & (1<<4)) != (1<<4)));
				DREQ_flag = 0;

				SPI1_Master_SD_write(&SDcard_readout_buf[512 + (j*32)], 32);

				Codec_DeSelect_SDI_bus();

			}

		 }

		 SDcard_readout_ptr = &SDcard_readout_buf[512];
		 FILE_Read_single_sector(sector_cnt + 3);																	//we load the fourth sector in place of the second one
		 sector_cnt += 2;

	 }

}

//11)
void Codec_Finish_Decoding(void){

	/*
	 * Below is the command sequence to indicate the end of file to the codec.
	 *
	 */
																		//endfillbyte should be extracted during decoding seemingly...
//	while(!DREQ_flag);													//we wait until DREQ goes LOW indicating that the SDI FIFO is empty and/or SCI commands have been executed
																		//DREQ flag is put HIGH if DREQ goes LOW using an EXTI
																		//as such, DREQ flag should always be LOW. Nevertheless, let's add this here as a safeguard.
																		//DREQ flag is always reset at the start of an SCI command
																		//we do a DREQ test since we may want to have an asynch CANCEL in the future

																		//DREQ is ALWAYS reset at the start of an SCI command
//	DREQ_flag = 0;

	for (uint16_t i = 0; i < 2052; i++){								//we send 2052 endfillbytes to the chip to indicate the end of the file

		 Codec_Select_SDI_bus();										//is this on the SDI bus or the SCI bus?
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	//information from the vs1053 sdfat library for Arduino suggests that this goes on the SDI bus
		 SPI1_Master_SD_write(&endfillbyte[0], 1);

		 Codec_DeSelect_SDI_bus();

	}

	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_WRITE_MODE_CANCEL[0],4);					//we send the cancel command

	Codec_DeSelect_SCI_bus();

	for (uint16_t i = 0; i < 64; i++){									//we send another 64 endfillbytes to ensure we are ending the decoding

		 Codec_Select_SDI_bus();

		 SPI1_Master_SD_write(&endfillbyte[0], 1);

		 Codec_DeSelect_SDI_bus();

	}

	//change to slow speed, need to read SCI bus!
	SPI1->CR1 &= ~(1<<6);;												//SPI disabled. SCK is enabled

	SPI1_w_o_DMA_500KHZ_init();

	SPI1->CR1 |= (1<<6);

	//read SCI_MODE
	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_READ_MODE[0], 2);							//we read the MODE register and check the CANCEL bit

	SPI1_Master_SD_read(&Codec_readout_buf[0], 2);

	if((Codec_readout_buf[1] & (1<<3)) == (1<<3)) {						//if the CANCEL bit is still HIGH
																		//response is MSB first, so that's why we check the [1] element for the CANCEL bit
																		//Note: this should not happen often according to the datasheet

		 Codec_Select_SCI_bus();

		 SPI1_Master_SD_write(&SCI_WRITE_MODE_SOFT_reset[0], 4);		//we send a software reset

		 Codec_DeSelect_SCI_bus();

	}

	//we read HDAT1 to ensure that the codec is reset

	uint8_t SCI_READ_HDAT1[2]	  	  	= {0x03, 0x09};					//file header information

	Codec_Select_SCI_bus();

	SPI1_Master_SD_write(&SCI_READ_HDAT1[0], 2);

	SPI1_Master_SD_read(&Codec_readout_buf[0], 2);						//for a WAV file, this should be 0x7665 - after reset, should be 0x0000

	Codec_DeSelect_SCI_bus();

	if(Codec_readout_buf[0] != 0) {										//if HDAT1 is not reset to 0

		while(1){

			//ERROR, codec is not properly reset...

		}

	}

	//reset the SPI bus to full speed
	SPI1->CR1 &= ~(1<<6);;												//SPI disabled. SCK is enabled

	SPI1_w_o_DMA_8MHZ_init();

	SPI1->CR1 |= (1<<6);

	DREQ_flag = 1;														//and we reset the DREQ flag

}

