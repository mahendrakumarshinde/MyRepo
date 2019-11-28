#ifndef BOOTLOADERCODES_H
#define BOOTLOADERCODES_H


// OTA Firmware Path deteails

#define TEST_CHUNK_SIZE	                512                         // Need to update in final run
#define STM_MAIN_FIRMWARE               "iuTempFirmware/vEdge_main.bin"
#define STM_ROLLBACK_FIRMWARE           "iuRollbackFirmware/vEdge_main.bin"
#define STM_FORCED_ROLLBACK_FIRMWARE    "iuBackupFirmware/vEdge_main.bin"
#define STM_FACTORY_FIRMWARE            "iuFactoryFirmware/STM-FFW.bin"

#define STM_MFW_1_SUM                   "iuTempFirmware/STM-MFW.md5"
#define STM_MFW_2_SUM                   "iuRollbackFirmware/STM-MFW.md5"
#define STM_MFW_3_SUM                   "iuBackupFirmware/STM-MFW.md5"
#define STM_FFW_1_SUM                   "iuFactoryFirmware/STM-FFW.md5"

#define ESP_MAIN_FIRMWARE               "iuTempFirmware"
#define ESP_ROLLBACK_FIRMWARE           "iuRollbackFirmware"
#define ESP_FORCED_ROLLBACK_FIRMWARE    "iuBackupFirmware"
#define ESP_FACTORY_FIRMWARE            "iuFactoryFirmware"
#define ESP_FIRMWARE_FILENAME           "vEdge_wifi.bin"

//#define TEST_READ_FILE STM_MFW_1

/***************************************************************************************************************************************/


// Status Codes

#define MFW_FLASH_FLAG                  0
#define RETRY_FLAG                      1
#define RETRY_VALIDATION                2
#define MFW_VER                         3
#define FW_VALIDATION                   4
#define FW_ROLLBACK                     5
#define STABLE_FW                       6


// ESP Status Codes

#define ESP_FW_VER 7
#define ESP_FW_UPGRAD 8
#define ESP_RUNNING_VER 9
#define ESP_ROLLBACK 10


// File Read Write Return Codes

#define FILE_READ_FAILED                1
#define FILE_OPEN_FAILED                2
#define FILE_WRITE_FAILED               3
#define FILE_CHECKSUM_FAILED            4

#define STM32_FLASH_READ_ERROR          5
#define STM32_FLASH_WRITE_ERROR         6
#define STM32_FLASH_ERASE_FAILED        7

#define STM32_FLASH_UNLOCK_FAILED       8
#define STM32_FLASH_LOCK_FAILED         9  

#define ESP32_FLASH_WRITE_FAILED        10
#define ESP32_FLASH_READ_FAILED         11
#define ESP32_SYNC_FAILED               12

#define ESP32_HASH_VERIFY_FAILED        13

#define STM32_FLASH_SUCESS              14                  

#define RETURN_SUCESS                   0
#define RETURN_FAILED                   1


#endif      // BOOTLOADERCODES_H