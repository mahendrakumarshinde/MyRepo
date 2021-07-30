#ifndef BOOTLOADERCODES_H
#define BOOTLOADERCODES_H


const char BOOTLOADER2_VERSION[8] = "2.1.0";

// OTA Firmware Path deteails

#define TEST_CHUNK_SIZE	                512                         // Need to update in final run
#define STM_MAIN_FIRMWARE               "iuTempFirmware/vEdge_main.bin"
#define STM_ROLLBACK_FIRMWARE           "iuRollbackFirmware/vEdge_main.bin"
#define STM_FORCED_ROLLBACK_FIRMWARE    "iuBackupFirmware/vEdge_main.bin"
#define STM_FACTORY_FIRMWARE            "iuFactoryFirmware/vEdge_main.bin"

#define STM_MFW_1_SUM                   "iuTempFirmware/vEdge_main.md5"
#define STM_MFW_2_SUM                   "iuRollbackFirmware/vEdge_main.md5"
#define STM_MFW_3_SUM                   "iuBackupFirmware/vEdge_main.md5"
#define STM_FFW_1_SUM                   "iuFactoryFirmware/vEdge_main.md5"


#define ESP_MAIN_FIRMWARE1               "iuTempFirmware/vEdge_wifi.bin"
#define ESP_ROLLBACK_FIRMWARE1           "iuRollbackFirmware/vEdge_wifi.bin"
#define ESP_FORCED_ROLLBACK_FIRMWARE1    "iuBackupFirmware/vEdge_wifi.bin"
#define ESP_MFW_1_SUM                   "iuTempFirmware/vEdge_wifi.md5"
#define ESP_MFW_2_SUM                   "iuRollbackFirmware/vEdge_wifi.md5"
#define ESP_MFW_3_SUM                   "iuBackupFirmware/vEdge_wifi.md5"

#define ESP_TEMP_BOOT_APP_BIN           "iuTempFirmware/vEdge_wifi_boot.bin"
#define ESP_TEMP_BOOT_LODR_BIN          "iuTempFirmware/vEdge_wifi_bootloader.bin"
#define ESP_TEMP_PART_BIN               "iuTempFirmware/vEdge_wifi_partition.bin"
#define ESP_TEMP_BOOT_APP_SUM           "iuTempFirmware/vEdge_wifi_boot.md5"
#define ESP_TEMP_BOOT_LODR_SUM          "iuTempFirmware/vEdge_wifi_bootloader.md5"
#define ESP_TEMP_PART_SUM               "iuTempFirmware/vEdge_wifi_partition.md5"

#define ESP_RLBK_BOOT_APP_BIN           "iuRollbackFirmware/vEdge_wifi_boot.bin"
#define ESP_RLBK_BOOT_LODR_BIN          "iuRollbackFirmware/vEdge_wifi_bootloader.bin"
#define ESP_RLBK_PART_BIN               "iuRollbackFirmware/vEdge_wifi_partition.bin"
#define ESP_RLBK_BOOT_APP_SUM           "iuRollbackFirmware/vEdge_wifi_boot.md5"
#define ESP_RLBK_BOOT_LODR_SUM          "iuRollbackFirmware/vEdge_wifi_bootloader.md5"
#define ESP_RLBK_PART_SUM               "iuRollbackFirmware/vEdge_wifi_partition.md5"

#define ESP_BKUP_BOOT_APP_BIN           "iuBackupFirmware/vEdge_wifi_boot.bin"
#define ESP_BKUP_BOOT_LODR_BIN          "iuBackupFirmware/vEdge_wifi_bootloader.bin"
#define ESP_BKUP_PART_BIN               "iuBackupFirmware/vEdge_wifi_partition.bin"
#define ESP_BKUP_BOOT_APP_SUM           "iuBackupFirmware/vEdge_wifi_boot.md5"
#define ESP_BKUP_BOOT_LODR_SUM          "iuBackupFirmware/vEdge_wifi_bootloader.md5"
#define ESP_BKUP_PART_SUM               "iuBackupFirmware/vEdge_wifi_partition.md5"

#define ESP_MAIN_FIRMWARE               "iuTempFirmware"
#define ESP_ROLLBACK_FIRMWARE           "iuRollbackFirmware"
#define ESP_FORCED_ROLLBACK_FIRMWARE    "iuBackupFirmware"
#define ESP_FACTORY_FIRMWARE            "iuFactoryFirmware"
#define ESP_FIRMWARE_FILENAME           "vEdge_wifi.bin"

#define ESP_BOOT_APP_FILENAME            "vEdge_wifi_boot.bin"
#define ESP_BOOT_LDR_FILENAME            "vEdge_wifi_bootloader.bin"
#define ESP_PART_TBL_FILENAME            "vEdge_wifi_partition.bin"

//#define TEST_READ_FILE STM_MFW_1

/* OTA Status flag Values */
#define OTA_FW_SUCCESS              0  // OTA FW Success, continue with new OTA FW
#define OTA_FW_DOWNLOAD_SUCCESS     1  // OTA FW Download Success, Bootloader L2 shall perform Upgrade to new FW
#define OTA_FW_UPGRADE_FAILED       2  // OTA FW Upgrade Failed, Bootloader L2 shall perform retry, internal rollback
#define OTA_FW_UPGRADE_SUCCESS      3  // OTA FW Upgrade Success, New FW shall perform validation
#define OTA_FW_INTERNAL_ROLLBACK    4  // OTA FW Validation failed,Bootloader L2 shall perform internal rollback
#define OTA_FW_FORCED_ROLLBACK      5  // OTA FW Forced rollback,Bootloader L2 shall perform Forced rollback
#define OTA_FW_FILE_CHKSUM_ERROR    6  // OTA FW File checksum failed, OTA can not be performed.
#define OTA_FW_FILE_SYS_ERROR       7  // OTA FW File system error, OTA can not be performed.
#define OTA_FW_FACTORY_IMAGE        8  // OTA FW Factory Firmware bootup

/***************************************************************************************************************************************/

#define SELF_FW_UPGRADE_SUCCESS      4  // Self FW Upgrade Success

// Status Codes

#define MFW_FLASH_FLAG                  0
#define RETRY_FLAG                      1
#define RETRY_VALIDATION                2
#define MFW_VER                         3
#define FW_VALIDATION                   4
#define FW_ROLLBACK                     5
#define STABLE_FW                       6

#define OTA_PEND_STATUS_MSG_LOC         3  // (0x080FF810)  reuse of // #define MFW_VER                 3
#define SELF_UPGRD_STATUS_MSG_LOC       4  // (0x080FF820)  reuse of // #define FW_VALIDATION           4

// ESP Status Codes

#define ESP_FW_VER      7
#define ESP_FW_UPGRAD   8
#define ESP_RUNNING_VER 9
#define ESP_ROLLBACK    10


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
