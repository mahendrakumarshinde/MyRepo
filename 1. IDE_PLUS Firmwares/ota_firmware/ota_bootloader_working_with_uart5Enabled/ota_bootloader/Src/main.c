/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "boot_flash.h"
#include "boot_uart.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
static uint16_t Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint16_t BufferLength);
static uint16_t Buffercmp32(uint32_t* pBuffer1, uint32_t* pBuffer2, uint16_t BufferLength);
void Buffercpy(uint32_t* sBuffer, uint32_t* dBuffer, uint16_t BufferLength);
void uart_debug();
int boot_pins_read();
void boot_uart_read(uint8_t* rx_buffer);
void led_blink(void);
void write_flag_init();
void read_all_flags();
void write_all_flags();


uint8_t Boot_MFW_Flag;
uint8_t Boot_FFW_Flag;
uint8_t Boot_RB_MFW_Flag;
uint8_t Debug_UART_Flag;


/*uint32_t MAIN_FW_TEMP = 0;
uint32_t FACTORY_FW_TEMP = 0;
uint32_t MFW_VER_TEMP = 0;
uint32_t FW_VALIDATION_TEMP = 0;
uint32_t FW_ROLLBACK_TEMP = 0;
uint32_t STABLE_FW_TEMP = 0;
uint32_t ESP_FW_VER_TEMP = 0;
uint32_t ESP_FW_UPGRAD_TEMP = 0;
uint32_t ESP_RUNNING_VER_TEMP = 0;
uint32_t ESP_ROLLBACK_TEMP = 0;*/

uint32_t all_flags[32] ;


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart5;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_UART5_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

int boot_value = 3 ;
uint8_t  FlashValue = 0;
uint8_t FlagAddr;

  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_UART5_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  //--------------------------------------led toggle----------------------------------------//

  led_blink();

  //----------------------------------------------------------------------------------------//
  //---------------------------------------Debug UART---------------------------------------//
  uart_transmit_str((uint8_t*)"\n\r================================\n\r");
  uart_transmit_str((uint8_t*)"IU Bootloader v0.01 \n\r");
  uart_transmit_str((uint8_t*)"================================\n\r\n\r");
  //uart_transmit_str((uint8_t*)"Waiting for the input.............. \n\r");

  //----------------------------------------------------------------------------------------//
//  Bootloader_Init();
//  Flag_Erase_All();
//  Bootloader_FlashBegin();
//  Write_Flag(MAIN_FW,1);
//  Write_Flag(FACTORY_FW,0x3A);
//  Write_Flag(MFW_VER,0X4B5E);
//  Bootloader_FlashEnd();

//   FlashValue = *(uint8_t*)(FLAG_ADDRESS);
//   uint8_t FlashValue_1 = *(uint8_t*)(FLAG_ADDRESS+8);
//   uint8_t FlashValue_2 = *(uint8_t*)(FLAG_ADDRESS+16);
//  uint32_t FlashValue_1 = Read_Flag(MAIN_FW);
//  uint32_t FlashValue_2 = Read_Flag(FACTORY_FW);
//  uint32_t FlashValue_3 = Read_Flag(MFW_VER);
  //---------------------------------------GPIO INPUT---------------------------------------//
  boot_value =  boot_pins_read();
  boot_value = 1; //forcing to uart


  switch(boot_value)
  {
  case 0: Boot_FFW_Flag = 1 ;//set Factory Image;
	  break;
  case 1: Debug_UART_Flag = 1; //uart_debug(); //pass control to UART
	  break;
  case 2: Boot_RB_MFW_Flag = 1; //roll back main firmware
	  break;
  default :  Boot_MFW_Flag =1 ;//normal boot / boot main firmware (default)
  }
  if(Debug_UART_Flag == 1)
  {
/*
	  uint8_t rx_buffer_temp[50];

	  boot_uart_read(&rx_buffer_temp);
	  HAL_Delay(1000);
*/

	  uart_debug();
  }

  if(Boot_RB_MFW_Flag == 1)
  {

	  update_flag(FW_ROLLBACK, 1);
	  uart_transmit_str((uint8_t*)"Jumping to ROLLBACK MFW.... \n\r");
	  HAL_Delay(1000);
	  flash_jump_to_factory_firmware();

  }else if(Boot_FFW_Flag == 1)
  {
	  uart_transmit_str((uint8_t*)"Jumping to FFW.... \n\r");
	  HAL_Delay(1000);
	  flash_jump_to_factory_firmware();

  }else if(Boot_MFW_Flag == 1)
  {
	  uart_transmit_str((uint8_t*)"Jumping to MFW.... \n\r");
	  HAL_Delay(1000);
	  flash_jump_to_main_firmware();
  }



  //----------------------------------------------------------------------------------------//
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  HAL_Delay(1000);
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void update_flag(uint8_t flag_addr, uint32_t flag_data)
{
	  read_all_flags();
	  all_flags[flag_addr] = flag_data;
	  write_flag_init();
	  write_all_flags();
	  Bootloader_FlashEnd();

}
void read_all_flags()
{
//	   MAIN_FW_TEMP 		= Read_Flag(MAIN_FW);
//	   FACTORY_FW_TEMP 		= Read_Flag(FACTORY_FW);
//	   MFW_VER_TEMP 		= Read_Flag(MFW_VER);
//	   FW_VALIDATION_TEMP	= Read_Flag(FW_VALIDATION);
//	   FW_ROLLBACK_TEMP 	= Read_Flag(FW_ROLLBACK);
//	   STABLE_FW_TEMP 		= Read_Flag(STABLE_FW);
//	   ESP_FW_VER_TEMP 		= Read_Flag(ESP_FW_VER);
//	   ESP_FW_UPGRAD_TEMP 	= Read_Flag(ESP_FW_UPGRAD);
//	   ESP_RUNNING_VER_TEMP = Read_Flag(ESP_RUNNING_VER);
//	   ESP_ROLLBACK_TEMP 	= Read_Flag(ESP_ROLLBACK);

	for(int i =0 ; i<32 ; i++)
	{

		all_flags[i] = Read_Flag(i);
	}


}
void write_all_flags()
{
//		Write_Flag(MAIN_FW,MAIN_FW_TEMP);
//
//		Write_Flag(FACTORY_FW,FACTORY_FW_TEMP);
//
//		Write_Flag(MFW_VER,MFW_VER_TEMP);
//
//		Write_Flag(FW_VALIDATION,FW_VALIDATION_TEMP);
//
//		Write_Flag(FW_ROLLBACK,FW_ROLLBACK_TEMP);
//
//		Write_Flag(STABLE_FW,STABLE_FW_TEMP);
//
//		Write_Flag(ESP_FW_VER,ESP_FW_VER_TEMP);
//
//		Write_Flag(ESP_FW_UPGRAD,ESP_FW_UPGRAD_TEMP);
//
//		Write_Flag(ESP_RUNNING_VER,ESP_RUNNING_VER_TEMP);
//
//		Write_Flag(ESP_ROLLBACK,ESP_ROLLBACK_TEMP);


	uint32_t all_flag_temp[32];

	Buffercpy(all_flags ,all_flag_temp, 32 );

	for(int i =0 ; i<32 ; i++)
	{
		//Write_Flag(i,all_flags[i]);
		//HAL_Delay(1000);
	}
	read_all_flags();

	if(Buffercmp32(all_flag_temp , all_flags, 32))
	{
		uart_transmit_str((uint8_t*)"Success.... \n\r");

	}else
	{
		uart_transmit_str((uint8_t*)"Failed.... \n\r");

	}

}
void write_flag_init()
{
	Bootloader_Init();
	Flag_Erase_All();
	Bootloader_FlashBegin();
}
int boot_pins_read()
{
int boot_0 = 1;
int boot_1 = 1;
int boot_value = 3;

_Bool boot0_1 = HAL_GPIO_ReadPin(Boot_button_port, Boot_button_0);
_Bool boot1_1 = HAL_GPIO_ReadPin(Boot_button_port, Boot_button_1);
	HAL_Delay(1);
_Bool boot0_2 = HAL_GPIO_ReadPin(Boot_button_port, Boot_button_0);
_Bool boot1_2 = HAL_GPIO_ReadPin(Boot_button_port, Boot_button_1);
	HAL_Delay(1);
_Bool boot0_3 = HAL_GPIO_ReadPin(Boot_button_port, Boot_button_0);
_Bool boot1_3 = HAL_GPIO_ReadPin(Boot_button_port, Boot_button_1);
	HAL_Delay(1);
_Bool boot0_4 = HAL_GPIO_ReadPin(Boot_button_port, Boot_button_0);
_Bool boot1_4 = HAL_GPIO_ReadPin(Boot_button_port, Boot_button_1);
	HAL_Delay(1);


if(boot0_1 && boot0_2 && boot0_3 && boot0_4)
	{
	boot_0 = 1;
	}else if (!boot0_1 && !boot0_2 && !boot0_3 && !boot0_4)
	{
		boot_0 = 0;
	}else{
		boot_0 = 2;
	}
if(boot1_1 && boot1_2 && boot1_3 && boot1_4)
	{
	boot_1 = 1;
	}else if (!boot1_1 && !boot1_2 && !boot1_3 && !boot1_4)
	{
		boot_1 = 0;
	}else
	{
		boot_1 = 2;
	}
if((boot_0 >= 2) && (boot_1 >= 2) )
	{
	 boot_value = boot_0 *2 + boot_1;
	}else
	{
		boot_value = 3;
	}
return boot_value;
}
void uart_debug()
{
	unsigned char rx_buffer[50];
	unsigned char rx_flag_address_buffer[50];
	unsigned char rx_data_buffer[50];
	unsigned char cmd_0[3]={'B','M','F'};
	unsigned char cmd_1[3]={'B','F','F'};
	unsigned char cmd_2[3]={'R','B','M'};
	unsigned char cmd_3[3]={'R','F','L'};
	unsigned char cmd_4[3]={'W','F','L'};
	unsigned char cmd_5[4]={'H','E','L','P'};

label1:uart_transmit_str((uint8_t*)"Waiting for command");
	boot_uart_read((uint8_t*)rx_buffer);

	  if(Buffercmp((uint8_t*)cmd_0,(uint8_t*)rx_buffer,3))
	  {
		  uart_transmit_str((uint8_t*)"received BMF.......\n\r");
		  Boot_MFW_Flag = 1; // Boot main firmware.

	  }else if(Buffercmp((uint8_t*)cmd_1,(uint8_t*)rx_buffer,3))
	  {
		  uart_transmit_str((uint8_t*)"received BFF.......\n\r");
		  Boot_FFW_Flag = 1; // Boot Factory firmware.


	  }else if(Buffercmp((uint8_t*)cmd_2,(uint8_t*)rx_buffer,3))
	  {
		  uart_transmit_str((uint8_t*)"Received RBM.......\n\r");
		  Boot_RB_MFW_Flag = 1; // Rollback main firmware.

	  }else if(Buffercmp((uint8_t*)cmd_3,(uint8_t*)rx_buffer,3))
	  {
		  uart_transmit_str((uint8_t*)"Received RFL.......\n\r");
		  uart_transmit_str((uint8_t*)"Enter the flag address...");
		  boot_uart_read((uint8_t*)rx_flag_address_buffer);
		  uart_transmit_str((uint8_t*)"Received address :");
		  uart_transmit_str(rx_flag_address_buffer);
		  uart_transmit_str((uint8_t*)"Flag value at");
		  uart_transmit_str(rx_flag_address_buffer);
		  uart_transmit_str((uint8_t*)"is ");
		  //
		  goto label1;
	  }else if(Buffercmp((uint8_t*)cmd_4,(uint8_t*)rx_buffer,3))
	  {
		  uart_transmit_str((uint8_t*)"Received WFL.......\n\r");
		  uart_transmit_str((uint8_t*)"Enter the flag address...");
		  boot_uart_read((uint8_t*)rx_flag_address_buffer);
		  uart_transmit_str((uint8_t*)"received address :");
		  uart_transmit_str(rx_flag_address_buffer);
		  uart_transmit_str((uint8_t*)"Enter the flag value...");
		  boot_uart_read((uint8_t*)rx_data_buffer);
		  uart_transmit_str((uint8_t*)"received data :");
		  uart_transmit_str(rx_data_buffer);
		  //
		  goto label1;
	  }else if(Buffercmp((uint8_t*)cmd_5,(uint8_t*)rx_buffer,4))
	  {
		  uart_transmit_str((uint8_t*)"WFL : Write flag\n\r");


	  }
	  else
	  {
		  uart_transmit_str((uint8_t*)"invalid command.......\n\r");
		  //uart_transmit_str((uint8_t*)"Waiting for command.......\n\r");
		  goto label1;
	  }


}
void boot_uart_read(uint8_t* rx_buffer)
{

int uart_receive_timout_1 =0;

//uart_transmit_str((uint8_t*)"waiting for command.......");
	for(int i =0 ;i<=50; i++)
		{
		rx_buffer[i] = '\0';
		 }
	while(rx_buffer[0] == '\0')
		{
		uart_transmit_str((uint8_t*)".");
		HAL_UART_Receive(&DEBUG_UART, &rx_buffer[0], 50, 1000);
		uart_receive_timout_1++;
		if(uart_receive_timout_1 >=60)
		 	 {
		 	 	uart_transmit_str((uint8_t*)"uart timeout.......\n\r");
		 	 	break;
		 	 }
		 if(rx_buffer[0]!='\0')
		 	 {
			 	 uart_receive_timout_1 = 0;
		 		uart_transmit_str((uint8_t*)"\n\r command received !\n\r");
		 		break;

		 	 }
		  }// end of loop

}
void led_blink(void)
{
	  HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);
	  HAL_Delay(500);
	  HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);
	  HAL_Delay(500);
	  HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);
	  HAL_Delay(500);
	  HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);
	  HAL_Delay(500);
	  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, 0);

}
static uint16_t Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint16_t BufferLength)
{
  while (BufferLength--)
  {
    if ((*pBuffer1) != *pBuffer2)
    {
      return 0;
    }
    pBuffer1++;
    pBuffer2++;
  }

  return 1;
}
static uint16_t Buffercmp32(uint32_t* pBuffer1, uint32_t* pBuffer2, uint16_t BufferLength)
{
  while (BufferLength--)
  {
    if ((*pBuffer1) != *pBuffer2)
    {
      return 0;
    }
    pBuffer1++;
    pBuffer2++;
  }

  return 1;
}
void Buffercpy(uint32_t* sBuffer, uint32_t* dBuffer, uint16_t BufferLength)
{
	for(uint16_t i=0 ; i< BufferLength ; i++)
	{

		dBuffer[i] = sBuffer[i];
	}

}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3|RCC_PERIPHCLK_UART5;
  //PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInit.Uart5ClockSelection = RCC_UART5CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the main internal regulator output voltage 
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  huart5.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart5.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : PC0 PC1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : RED_LED_Pin */
  GPIO_InitStruct.Pin = RED_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RED_LED_GPIO_Port, &GPIO_InitStruct);

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
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
