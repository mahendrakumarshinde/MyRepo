#ifndef IUOTA_H
#define IUOTA_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "IUMD5Builder.h"
#include <IUMessage.h>
#include "MSPCommands.h"
#include "MacAddress.h"
#include <FS.h>


#define  OTA_MAIN_FW1 "/iuTempFirmware/vEdge_main.bin"
#define  OTA_MAIN_FW2 "/iuRollbackFirmware/vEdge_main.bin"
#define  OTA_MAIN_FW3 "/iuBackupFirmware/vEdge_main.bin"
#define  OTA_MAIN_CHK1 "/iuTempFirmware/vEdge_main.md5"
#define  OTA_MAIN_CHK2 "/iuRollbackFirmware/vEdge_main.md5"
#define  OTA_MAIN_CHK3 "/iuBackupFirmware/vEdge_main.md5"

#define  OTA_WIFI_FW1 "/iuTempFirmware/vEdge_wifi.bin"
#define  OTA_WIFI_FW2 "/iuRollbackFirmware/vEdge_wifi.bin"
#define  OTA_WIFI_FW3 "/iuBackupFirmware/vEdge_wifi.bin"
#define  OTA_WIFI_CHK1 "/iuTempFirmware/vEdge_wifi.md5"
#define  OTA_WIFI_CHK2 "/iuRollbackFirmware/vEdge_wifi.md5"
#define  OTA_WIFI_CHK3 "/iuBackupFirmware/vEdge_wifi.md5"



#define  OTA_DOWNLOAD_SUCCESS   0
#define  OTA_INVALID_MQTT       1
#define  OTA_CHECKSUM_FAIL      2
#define  OTA_WIFI_DISCONNECT    3
#define  OTA_FLASH_RDWR_FAIL    4
#define  OTA_DOWNLOAD_TMOUT     5
#define  OTA_VALIDATION_FAILED  6
#define  OTA_UPGRADE_FAIL       7
#define  OTA_INT_RLBK_FAIL      8
#define  OTA_FORCED_RLBK_FAIL   9
#define  OTA_FILE_MISSING       10

#define OTA_VALIDATION_SUCCESS  0
#define OTA_VALIDATION_RETRY    1
#define OTA_VALIDATION_FAIL     2
#define OTA_VALIDATION_WIFI     3


#define BL1_ADDRESS     (uint32_t)0x08000000    /* Start address of Bootloader 1 */
#define BL2_ADDRESS     (uint32_t)0x08010000    /* Start address of Bootloader 2 */
#define FFW_ADDRESS     (uint32_t)0x08036000    /* Start address of Facotry Firmware */
#define MFW_ADDRESS     (uint32_t)0x08060000    /* Start address of Main Firmware */
#define FLAG_ADDRESS    (uint32_t)0x080FF800    /* Start address of FLAG location*/
// #define CONFIG_MQTT_FLASH_ADDRESS    (uint32_t)0x080FE800    /* Start address of MQTT Config location*/
// #define CONFIG_HTTP_FLASH_ADDRESS    (uint32_t)0x080FE000    /* Start address of HTTP Config location*/

#define OTA_MAX_VALIDATION_RETRY    2

/* OTA Status flag location in memory @FLAG_ADDRESS */
#define OTA_STATUS_FLAG_LOC     0  // (0x080FF800)
#define OTA_RETRY_FLAG_LOC      1  // (0x080FF804)
#define OTA_VLDN_RETRY_FLAG_LOC 2  // (0x080FF808)

#define OTA_DEVICE_TYPE         "vEdge"
#define OTA_RESPONE_OK          "OTA-RCA-0000"
#define OTA_REQ_ACK             "OTA-INIT-ACK"
                                
#define OTA_DOWNLOAD_START      "OTA-FDW-START"                         
#define OTA_DOWNLOAD_OK         "OTA-FDW-SUCCESS"
#define OTA_DOWNLOAD_ERR        "OTA-ERR-FDW-ABORT"

#define OTA_UPGRADE_START       "OTA-FUG-START"
#define OTA_UPGRADE_ERR         "OTA-ERR-FUG-ABORT"
#define OTA_UPGRADE_OK          "OTA-FUG-SUCCESS"


#define vEdge_Main_FW_BIN           "vEdge_main.bin"
#define vEdge_Main_FW_MD5           "vEdge_main.md5"
#define vEdge_Wifi_FW_BIN           "vEdge_wifi.bin"
#define vEdge_Wifi_FW_MD5           "vEdge_wifi.md5"

/* OTA Status flag Values */
#define OTA_FW_VALIDATION_SUCCESS   0  // OTA FW Validation Success, continue with new OTA FW
#define OTA_FW_DOWNLOAD_SUCCESS     1  // OTA FW Download Success, Bootloader L2 shall perform Upgrade to new FW
#define OTA_FW_UPGRADE_FAILED       2  // OTA FW Upgrade Failed, Bootloader L2 shall perform retry, internal rollback
#define OTA_FW_UPGRADE_SUCCESS      3  // OTA FW Upgrade Success, New FW shall perform validation
#define OTA_FW_INTERNAL_ROLLBACK    4  // OTA FW Validation failed,Bootloader L2 shall perform internal rollback
#define OTA_FW_FORCED_ROLLBACK      5  // OTA FW Forced rollback,Bootloader L2 shall perform Forced rollback
#define OTA_FW_FILE_CHKSUM_ERROR    6  // OTA FW File checksum failed, OTA can not be performed.
#define OTA_FW_FILE_SYS_ERROR       7  // OTA FW File system error, OTA can not be performed.
//#define OTA_FW_FACTORY_FW         8  // OTA FW Facotry Firmware case handling
#define OTA_FW_DOWNLOAD_FAILED      9 // OTA FW Download failed. Send status message to Server
class IUOTA
{
    public:
        
        IUOTA() {}
        ~IUOTA() {}

        void otaFileDownload();
        bool otaFwBinWrite(char *folderName,char *fileName, char *buff, uint16_t size);
        bool otaMD5Write(char *folderName,char *fileName, char *md5);
        bool otaFwBinRead(char *folderName,char *fileName);//), char *readBuff, uint16_t *readSize);
        bool otaFileRemove(char *folderName,char *fileName);
        
        bool otaFileCopy(char *destFilePath,char *srcFilePath, char *filename);
        bool otaSendResponse(MSPCommand::command resp, const char *otaResponse);
        String file_md5 (File & f);
        bool otaGetMD5(char *folderName,char *fileName, char* md5HashRet);
        String getOtaRca(int error);
        void updateOtaFlag(uint8_t flag_addr , uint8_t flag_data);
        uint8_t getOtaFlagValue(uint8_t flag_addr) {return OtaStatusFlag[flag_addr*8]; }
        void readOtaFlag(void);

	protected:
        uint8_t OtaStatusFlag[128];
};

#endif
