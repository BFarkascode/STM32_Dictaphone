/*
 *  Created on: Aug 9, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_Dictaphone
 *  Processor: STM32L053R8
 *  Program version: 1.1
 *  Source file: SDcard_image_capture.c
 *  Change history:
 *
 *  v.1.1 Changed file type from bmp to wav
 *
 */


#include <INPUT_File_capture.h>

FATFS fs;             //file system
FIL fil;              //file
FILINFO filinfo;
FRESULT fresult;
char char_buffer[100];
uint8_t hex_buffer[100];

UINT br, bw;

FATFS *pfs;
DWORD fre_clust;
uint32_t total, free_space;

//1)
void SDcard_start(void){

	/*
	 * Activate card
	 */

	  fresult = f_mount(&fs, "", 0);                   													//mount card
	  if (fresult != FR_OK) printf("Card mounting failed... \r\n");
	  else ("Card mounted... \r\n");

	  //--Capacity check--//
	  f_getfree("", &fre_clust, &pfs);

	  total = (uint32_t) ((pfs->n_fatent - 2) * pfs->csize * 0.5);
	  printf("SD card total size is: \r\n");
	  printf("%d",total);

	  free_space = (uint32_t) (fre_clust * pfs->csize * 0.5);
	  printf("SD card free space is: \r\n");
	  printf("%d",free_space);

}

//2)
void FILE_wav_create(void){

	/*
	 * We create a wav file by writing a header
	 */

	  uint8_t file_cnt = 0;

	  //NOTE: this part below checks the file names and see what the newest one should be called
	  //We start from the name "000.wav"

	  fresult = f_stat(INPUT_SIDE_file_name, &filinfo);													//we check if the file exists already
	  while(fresult != FR_NO_FILE){

		  file_cnt++;
		  INPUT_SIDE_file_name[2] = file_cnt%10 + '0';
		  INPUT_SIDE_file_name[1] = (file_cnt%100)/10 + '0';
		  INPUT_SIDE_file_name[0] = file_cnt/100 + '0';
		  fresult = f_stat(INPUT_SIDE_file_name, &filinfo);												//we check if the file exists already

	  }

	  //we create a 16-bit 22050 Hz sampling rate mono wav file
	  fresult = f_stat(INPUT_SIDE_file_name, &filinfo);													//we check if the file exists already
	  if(fresult == FR_NO_FILE){																		//if it does not exist

		  f_open(&fil, INPUT_SIDE_file_name, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
		  hex_buffer[0] = 0x52;		   															    	// RIFF signature
		  hex_buffer[1] = 0x49;
		  hex_buffer[2] = 0x46;
		  hex_buffer[3] = 0x46;

		  hex_buffer[4] = 0xBC;		   															    	// FileLength (Header + data chunk)
		  hex_buffer[5] = 0xC5;
		  hex_buffer[6] = 0x06;
		  hex_buffer[7] = 0x00;


		  hex_buffer[8] = 0x57;		   															    	//WAVE
		  hex_buffer[9] = 0x41;
		  hex_buffer[10] = 0x56;
		  hex_buffer[11] = 0x45;

		  hex_buffer[12] = 0x66;		   															    //fmt
		  hex_buffer[13] = 0x6D;
		  hex_buffer[14] = 0x74;
		  hex_buffer[15] = 0x20;

		  hex_buffer[16] = 0x10;	   															    	//fmtChunkSize
		  hex_buffer[17] = 0x00;
		  hex_buffer[18] = 0x00;
		  hex_buffer[19] = 0x00;

		  hex_buffer[20] = 0x01;	   															    	//formatTag (PCM)
		  hex_buffer[21] = 0x00;

		  hex_buffer[22] = 0x01;		   															    //channel number (mono)
		  hex_buffer[23] = 0x00;


		  hex_buffer[24] = 0x22;		   															    //sampling rate (22050)
		  hex_buffer[25] = 0x56;
		  hex_buffer[26] = 0x00;
		  hex_buffer[27] = 0x00;

		  hex_buffer[28] = 0x44;		   															    //byterate (44100 - because we have 16 bits PCM, so 1 byte comes in at 44 kHz)
		  hex_buffer[29] = 0xAC;
		  hex_buffer[30] = 0x00;
		  hex_buffer[31] = 0x00;

		  hex_buffer[32] = 0x02;		   															    //blockalign/block size (2 - because we have 2 bytes)
		  hex_buffer[33] = 0x00;

		  hex_buffer[34] = 0x10;		   															    //bitspersample (16)
		  hex_buffer[35] = 0x00;

		  hex_buffer[36] = 0x64;  		   															    //data
		  hex_buffer[37] = 0x61;
		  hex_buffer[38] = 0x74;
		  hex_buffer[39] = 0x61;

		  hex_buffer[40] = 0x00;		   															    //datachunkSize for 5 sec with the data width defined above (5*44100*2)
		  hex_buffer[41] = 0xB8;
		  hex_buffer[42] = 0x06;
		  hex_buffer[43] = 0x00;

		  f_write(&fil, hex_buffer, 44, &bw);
		  bufclear();

		  f_close(&fil);

	  }

}

//3)
void bufclear (void)                       																//wipe buffer
{

  for (int i = 0; i < 100; i++){

    char_buffer[i] = '\0';
    hex_buffer[i] = 0x00;

  }

}

//4)
void FILE_Capture_audio(void){
	/*
	 * Capture audio data
	 * This is the function where we can do software filtering, amplification as well as data removal
	 *
	 */

	  f_open(&fil, INPUT_SIDE_file_name, FA_OPEN_ALWAYS | FA_READ | FA_WRITE | FA_OPEN_APPEND);			//we open the file we have created

	  uint8_t* data_capture_ptr = &mono_frame_buf[0];													//we place the capture pointer onto the frame buffer

	  DMA1_Channel6->CCR |= (1<<0);																		//we enable the DMA channel
	  SPI2->I2SCFGR |= (1<<10);																			//enable I2S
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	//Note: these two should occur just before data capture to time the data flow to the processing
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	//Note: we will capture 4 times 16 bits (a full frame) but that should always be in the first three 16 bits!

	  while(blue_button_pushed){																		//we record as long as the recording button - on the L053R8 nucleo, the blue push button - is being pressed

		  uint16_t* I2S_buf_read_ptr;
		  uint8_t* mono_buf_write_ptr;

		  uint8_t ptr_pos = 0;

		  if(I2S_frame_buffer_half_full == 1){

			  if(gen_file_no == 0){																		//there is a bug where the pointer needs to be placed one position later after the first file has been captured

				  I2S_buf_read_ptr = &I2S_frame_buf[0];

			  } else {

				  I2S_buf_read_ptr = &I2S_frame_buf[1];

			  }

			  mono_buf_write_ptr = &mono_frame_buf[0];

			  for(uint16_t i = 0; i < 64; i++){															//we process half the captured frames
				  																						//we need an endian swap on the incoming mic data
				  																						//we are also amplifying the data by 8. This means we amplify the noise as well. To be adjusted later if this is necessary

				  *mono_buf_write_ptr = *I2S_buf_read_ptr << 3;											//we copy the LSBs to the mono buffer
				  mono_buf_write_ptr++;																	//we move to the next mono buf value
				  *mono_buf_write_ptr = (*I2S_buf_read_ptr) >> 5;										//we copy the MSBs to the mono buffer
				  mono_buf_write_ptr++;																	//we move to the next mono buf value

				  I2S_buf_read_ptr += 4;																//we jump to the next frame

			  }

			  I2S_frame_buffer_half_full = 0;

		  } else {

			  //do nothing

		  }

		  if(I2S_frame_buffer_loaded == 1){																//if the blue button is pushed

			  if(gen_file_no == 0){																		//there is a bug where the pointer needs to be placed one position later after the first file has been captured

					  I2S_buf_read_ptr = &I2S_frame_buf[256];

			  } else {

					  I2S_buf_read_ptr = &I2S_frame_buf[257];

			  }

			  mono_buf_write_ptr = &mono_frame_buf[128];

			  for(uint16_t i = 0; i < 64; i++){

				  *mono_buf_write_ptr = *I2S_buf_read_ptr << 3;											//we copy the LSBs to the mono buffer
				  mono_buf_write_ptr++;																	//we move to the next mono buf value
				  *mono_buf_write_ptr = (*I2S_buf_read_ptr) >> 5;										//we copy the MSBs to the mono buffer
				  mono_buf_write_ptr++;																	//we move to the next mono buf value

				  //Note: we need an endian swap

				  I2S_buf_read_ptr += 4;																//we jump to the next frame

			  }

			  f_write(&fil, data_capture_ptr, 256, &bw);												//we write 4 times 8 bites (half a frame)
			  data_capture_ptr = &mono_frame_buf[0];
			  I2S_frame_buffer_loaded = 0 ;
//			  byte_counter -= 256;																		//we decrease the byte count

		  } else {

			  //do nothing

		  }

	  }

	  f_close(&fil);

	  I2S_shutdown();																					 //we turn off the I2S and reset the DMA to prepare for the next file

}

//5)
void FILE_Get_start_sector(char readout_INPUT_SIDE_file_name[]){

/*
 * This function is to extract the exact position of a file within SDcard memory.
 *
 */

	fresult = f_stat(readout_INPUT_SIDE_file_name, &filinfo);											//we check if teh file exists

	if(fresult != FR_OK) {

		while(1){

			//Error. File not found...

		}

	}

	file_size_in_sectors = (filinfo.fsize / 512);														//we divide the file size by 512 to get the net sector number the file takes up on the card

	f_open(&fil, readout_INPUT_SIDE_file_name, FA_OPEN_ALWAYS | FA_READ | FA_OPEN_APPEND);				//we use the FA_OPEN_APPEND to put the file pointer to the very end of the file

	uint32_t final_sector_of_file = fil.sect;															//we get the actual position of the file pointer regarding memory (in sectors!)
																										//Note: this may not be valid value if the pointer is on a boundary. Should not be the case if we ensure that the file size is not divided by 512.

	f_close(&fil);

	start_sector_of_file = final_sector_of_file - file_size_in_sectors;									//we calculate the start of the file by removing the net number of sectors from the value of the final sector

}

//6)
void FILE_Read_single_sector(uint16_t offset){

	/*
	 * This is to read out one entire sector from the SDcard
	 * It assumes that a file is a continuous block of sectors.
	 * Offset value defines, which sector of the file should be read out.
	 *
	 */

	SDCard_enable();

	SDCard_read_single_block(start_sector_of_file + offset, SDcard_readout_ptr);

	SDCard_disable();

}
