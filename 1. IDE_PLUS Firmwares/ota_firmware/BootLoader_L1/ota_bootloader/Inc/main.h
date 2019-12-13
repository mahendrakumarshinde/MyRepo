/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

#define RED_LED_Pin GPIO_PIN_2
#define RED_LED_GPIO_Port GPIOB


#define Boot_button_0 GPIO_PIN_0
#define Boot_button_1 GPIO_PIN_1
#define Boot_button_port GPIOA
//-----------------------------------------UART--------------------------------------------------//
//----------------------------------------------------------------------------------------------//
//-----------------------------FLAG SETTING ADDRESS----------------------------------------------//
#define MFW_FLASH_FLAG          0
#define RETRY_FLAG              1
#define RETRY_VALIDATION        2
#define MFW_VER                 3
#define FW_VALIDATION           4
#define FW_ROLLBACK             5
#define STABLE_FW               6

#define ESP_FW_VER              7
#define ESP_FW_UPGRAD           8
#define ESP_RUNNING_VER         9
#define ESP_ROLLBACK            10
//-----------------------------------------------------------------------------------------------//
#define MAX_RETRY_VAL           5
#define MAX_RETRY_FLAG          3
//-----------------------------------------------------------------------------------------------//
// OTA Flags

#define OTA_FW_SUCCESS              0  // OTA FW Success, continue with new OTA FW
#define OTA_FW_DOWNLOAD_SUCCESS     1  // OTA FW Download Success, Bootloader L2 shall perform Upgrade to new FW
#define OTA_FW_UPGRADE_FAILED       2  // OTA FW Upgrade Failed, Bootloader L2 shall perform retry, internal rollback
#define OTA_FW_UPGRADE_SUCCESS      3  // OTA FW Upgrade Success, New FW shall perform validation
#define OTA_FW_INTERNAL_ROLLBACK    4  // OTA FW Validation failed,Bootloader L2 shall perform internal rollback
#define OTA_FW_FORCED_ROLLBACK      5  // OTA FW Forced rollback,Bootloader L2 shall perform Forced rollback
#define OTA_FW_FILE_CHKSUM_ERROR    6  // OTA FW File checksum failed, OTA can not be performed.
#define OTA_FW_FILE_SYS_ERROR       7  // OTA FW File system error, OTA can not be performed.
#define OTA_FW_FACTORY_IMAGE        8  // OTA FW Factory Firmware bootup
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
