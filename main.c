/* USER CODE BEGIN Header */
/**
 *  *  Author: BalazsFarkas
 *  Project: STM32_Dictaphone
 *  Processor: STM32L053R8
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Program version: 1.0
 *  File: main.c
 *  Hardware description/pin distribution:
 *  					I2S/SPI2		- PB12 (WS), PB13 (SCK), PB15 (DATA)
 *  					SPI1 			- PA5 (SCK), PA6 (MISO), PA7 (MOSI)
 *  					SDcard CS		- PB6
 *  					Codec SCI CS	- PA0
 *  					Codec SDI CS	- PA1
 *  					Codec DREQ		- PA4
 *  					buttons			- PC13, PA10
 *  Change history: N/A
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  ******************************************************************************
  ******************************************************************************
  ******************************************************************************
  * This is just a simple I2S plotter, reading in the input from an I2S mic.
  * Readout from the mic is 24 bit MSB with 18 bits is valuable data (test is 0)
  * Captured data is 32*2 bits with the second 32 bits being 0 (right channel is not used. Mic can't be run mono.)
  * No DMA is used to capture here. Two frames are simply loaded into a small static buffer and that's it.
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ClockDriver_STM32L0x3.h"
#include "EXTIDriver_STM32L0x3.h"
#include "ff.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
uint8_t log_audio_flag;
uint8_t send_audio_flag;
uint8_t DREQ_flag;

uint8_t blue_button_pushed = 0;															//PC13 is the blue button. It will be used to signal recording.

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

char INPUT_SIDE_file_name[] = "000.wav";												//we place the input side file name pointer
char OUTPUT_SIDE_file_name[] = "000.wav";												//we place the output side file name pointer

uint16_t I2S_frame_buf[512];															//the I2S input buffer
uint16_t I2S_DMA_transfer_width = 512;													//I2S is read out using DMA
uint8_t I2S_frame_buffer_loaded = 0;													//DMA buffer flag
uint8_t I2S_frame_buffer_half_full = 0;													//DMA buffer flag

uint8_t gen_file_no;																	//generated file counter

uint8_t SDcard_readout_buf[1024];														//SDcard readout buffer (no DMA here!)
uint8_t* SDcard_readout_ptr;
uint16_t file_size_in_sectors;															//the size of the file we read out in sectors (file size is 512 bytes * sector number)


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
//  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  /* USER CODE BEGIN 2 */

  SysClockConfig();

  TIM6Config();																			//custom delay function

  //---------Set up push button----------//

  Push_button_Init();
  log_audio_flag = 0;

  //---------Set up push button----------//

  //---------Codec config----------//

  Codec_GPIO_Config();
  DREQ_flag = 1;
  Codec_SCI_Config(0x40);																//set the volume to 0x40. That's a good value for testing
  	  	  	  	  	  	  	  	  	  	  	  	  										//Note: the lower the value, the louder the output!

//  Codec_Sinetest_SDI(0x40);															//DEBUG function
  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  		//Note: this function above MUST be preceeded by a codec hard-reset. De-power the chip or push the reset button!

  //---------Codec config----------//


  //---------Set up SDcard----------//

  SDcard_start();
  f_unlink("000.wav");																	//we remove the five wav files from the SDcard in case we have created them already
  f_unlink("001.wav");
  f_unlink("002.wav");
  f_unlink("003.wav");
  f_unlink("004.wav");

  gen_file_no = 0;																		//we reset the file generation counter

  //---------Set up SDcard----------//


  //---------I2S config----------//
  uint16_t data_buf;
  I2S_init();

  DMAI2SInit();
  DMAI2SConfig();
  DMAI2SIRQPriorEnable();
  //---------I2S config----------//

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  if((log_audio_flag == 1) & (send_audio_flag == 0)){								//if the blue push button is clicked
		  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	//Note: recording start is a bit redundant how it is defined...

		   if(gen_file_no == 5){														//we use file_no to always limit the number of files generated to 5
			 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	    //this is necessary because sometimes fatfs can generate strange names for files. This ensures that outcome would not get out of hand
			 //do nothing

		 } else {

			 FILE_wav_create();															//we create a new wav file
			 FILE_Capture_audio();
			 gen_file_no++;

		 }

		 log_audio_flag = 0;

	  } else if((log_audio_flag == 0) & (send_audio_flag == 1)){						//if the blue button is pushed

		  FILINFO check_file;

		  if(f_stat(OUTPUT_SIDE_file_name, &check_file) == FR_OK){						//we will only read out files that exist

			 FILE_Get_start_sector(OUTPUT_SIDE_file_name);								//we get the start sector value for the file

			 Codec_Start_Decoding();													//we decode the file

//			 Codec_Debug_SCI();															//DEBUG: extract debug information as decoding concludes

			 Codec_Finish_Decoding();													//we send the end file sequence to the code

			 OUTPUT_SIDE_file_name[2] = 1 + OUTPUT_SIDE_file_name[2];					//we go to the next file

		  } else {																		//if the file doesn't exist, we go back to the first file, that is, "000.wav"
			  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	//this is to ensure that the readout doesn't bug out

			  OUTPUT_SIDE_file_name[2] = '0';											//Note: this is char '0', not value 0
			  OUTPUT_SIDE_file_name[1] = '0';
			  OUTPUT_SIDE_file_name[0] = '0';

		  }

		 send_audio_flag = 0;

	  } else if((log_audio_flag == 1) & (send_audio_flag == 1)) {

		  //in case the audio transfer flags are bugged out for some reason, we reset them and restart the main loop
		  log_audio_flag = 0;
		  send_audio_flag = 0;

	  } else {

		  //do nothing

	  }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_5;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
