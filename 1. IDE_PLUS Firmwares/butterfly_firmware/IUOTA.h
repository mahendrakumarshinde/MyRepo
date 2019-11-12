#ifndef IUOTA_H
#define IUOTA_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "IUMD5Builder.h"
#include <IUMessage.h>
#include "MSPCommands.h"
#include "MacAddress.h"
#include <FS.h>

#define  OTA_DOWNLOAD_SUCCESS 00
#define  OTA_INVALID_MQTT     01
#define  OTA_CHECKSUM_FAIL    02
#define  OTA_WIFI_DISCONNECT  03
#define  OTA_FLASH_RDWR_FAIL  04
#define  OTA_DOWNLOAD_TMOUT   05


class IUOTA
{
    public:
        
        IUOTA() {}
        ~IUOTA() {}

        void otaFileDownload();
        bool otaFwBinWrite(char *folderName,char *fileName, char *buff, uint16_t size);
        bool otaFileRemove(char *folderName,char *fileName);
        
        bool otaFileCopy(char *destFilePath,char *srcFilePath, char *filename);
        bool otaSendResponse(MSPCommand::command resp, const char *otaResponse);
        String file_md5 (File & f);
        bool otaGetMD5(char *folderName,char *fileName, char* md5HashRet);
        String getOtaRca(int error);
    //    const char *otaGetStmUri() { return m_otaStmUri; }
    //    const char *otaGetEspUri() { return m_ota_EspUri; }

	protected:
        /***** Core *****/
    //    MacAddress m_macAddress;        
    //    char m_otaStmUri[512];
    //    char m_otaEspUri[512];
     //   char m_type1[8];
     //   char m_type2[8];
     //   char m_otaMsgId[32];
     //   char m_otaMsgType[16];
     //   char m_otaFwVer[16];
     //   char fwBinFileName[32]; 
};

#endif
