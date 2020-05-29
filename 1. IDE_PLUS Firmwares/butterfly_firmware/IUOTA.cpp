#include "IUOTA.h"
#include "Conductor.h"
#include "IUFlash.h"

#define MAX_FILE_RW_SIZE    512
#define FLASH_BLOCK_SIZE    4096




/* =============================================================================
    Constructors and destructors
============================================================================= */

extern "C" {
extern void stm32l4_flash_lock(void);
extern bool stm32l4_flash_unlock(void);
extern bool stm32l4_flash_erase(uint32_t address, uint32_t count);
extern bool stm32l4_flash_program(uint32_t address, const uint8_t *data, uint32_t count);
}

/**
 * Write OTA FW binary MD5 in to external flash as .MD5
 */
bool IUOTA::otaMD5Write(char *folderName,char *fileName, char *md5)
{
    if(md5 == NULL || fileName == NULL)
    {
        if (debugMode) {
            debugPrint(F("fw md5 file write param error !"));
            if(fileName)
                debugPrint(fileName);
        } 
        return false;
    }
    char filepath[40];
    snprintf(filepath, 40, "%s/%s", folderName, fileName);
    if (debugMode) {
        debugPrint(filepath);
    }
    File md5File;
    md5File = DOSFS.open(filepath,"w");
    if(md5File)
    {
        md5File.write((uint8_t *)md5,33);
        md5File.close();
        return true;
    }
    else
    {
        if (debugMode) {
            debugPrint(F("Error Creating MD5 file:"),false);
            debugPrint(filepath);
        }
        return false;
    }
}


/**
 * Write OTA FW binary data in to external flash as .bin
 */
bool IUOTA::otaFwBinWrite(char *folderName,char *fileName, char *buff, uint16_t size)
{
    uint16_t writeLen = 0;
    uint16_t writeSize = MAX_FILE_RW_SIZE;    
    if(buff == NULL || size == 0 || fileName == NULL)
    {
        if (debugMode) {
            debugPrint(F("fw bin file write param error !"));
        }
        return false;
    }
    char filepath[40];
    snprintf(filepath, 40, "%s/%s", folderName, fileName);
    if (debugMode) {
        debugPrint(filepath);
    }
    File fwFile;
    if(DOSFS.exists(filepath))
    {
        fwFile = DOSFS.open(filepath,"a+");
    }
    else
    {
        if (debugMode) {
            debugPrint(F("Creating FW download file:"),false);
            debugPrint(filepath);
        }
        fwFile = DOSFS.open(filepath,"w");
    } 
    if(fwFile)
    {
     //   readReq = size;
        if(size < MAX_FILE_RW_SIZE)
            writeSize = size;
        else
            writeSize = MAX_FILE_RW_SIZE;
        
        writeLen = 0;
        do
        {           
            fwFile.write((uint8_t *)&buff[writeLen],writeSize);
            writeLen = writeLen + writeSize;
            if((size-writeLen) == 0)
                break;
            else if((size-writeLen) < MAX_FILE_RW_SIZE)
                writeSize = (size-writeLen);

        } while (1); 
        fwFile.close();
    }
    else
    {
        if (debugMode) {
            debugPrint(F("Error opening FW download file:"),false);
            debugPrint(filepath);
        }
        return false;
    }
    return true;
}

/**
 * Read OTA FW binary data in to external flash as .bin
 */
bool IUOTA::otaFwBinRead(char *folderName,char *fileName)
{
    char readBuf[FLASH_BLOCK_SIZE];
    uint32_t readIdx = 0;
    uint32_t fileSize = 0;
 
    if(folderName == NULL || fileName == NULL)
    {
        if (debugMode) {
            debugPrint(F("fw bin file read param error !"));
        }
        return false;
    }
    char filepath[40];
    snprintf(filepath, 40, "%s/%s", folderName, fileName);
    if (debugMode) {
        debugPrint(filepath);
    }
    if(loopDebugMode){
        debugPrint(F("File Path"),false);
        debugPrint(filepath);
    }
    File fwFile;
    if(DOSFS.exists(filepath))
    {
        fwFile = DOSFS.open(filepath,"r");
        if(fwFile)
        {
            unsigned char fileBuf[MAX_FILE_RW_SIZE];
            fileSize = fwFile.size();
            if (debugMode) {
                debugPrint(F("File Size:"),false);
                debugPrint(fileSize);
            }
            readIdx = 0;
            if(fileSize > 0)
            {
                do
                {                    
                    if(fileSize >= FLASH_BLOCK_SIZE)
                    {
                        readIdx = 0;
                        for(int i = 0; i < (FLASH_BLOCK_SIZE/MAX_FILE_RW_SIZE); i++)
                        {
                            memset(fileBuf,'\0',MAX_FILE_RW_SIZE);
                            fwFile.read(fileBuf,MAX_FILE_RW_SIZE);
                            memcpy(&readBuf[readIdx],&fileBuf[0],MAX_FILE_RW_SIZE);
                            readIdx = readIdx + MAX_FILE_RW_SIZE;
                            fileSize = fileSize - MAX_FILE_RW_SIZE;                    
                        }
                        if (debugMode) {
                            debugPrint(F("File Read Size:"),false);
                            debugPrint(readIdx);
                            for(int i = 0; i < FLASH_BLOCK_SIZE; i++)
                                debugPrint(readBuf[i]);
                        }
                        memset(readBuf,'\0',FLASH_BLOCK_SIZE);            
                    }
                    else
                    {     
                        readIdx = 0;                   
                        do
                        {                            
                            if(fileSize >= MAX_FILE_RW_SIZE) {
                                memset(fileBuf,'\0',MAX_FILE_RW_SIZE);
                                fwFile.read(fileBuf,MAX_FILE_RW_SIZE);
                                memcpy(&readBuf[readIdx],&fileBuf[0],MAX_FILE_RW_SIZE);
                                readIdx = readIdx + MAX_FILE_RW_SIZE;
                                fileSize = fileSize - MAX_FILE_RW_SIZE;
                            }
                            else
                            {
                                memset(fileBuf,'\0',MAX_FILE_RW_SIZE);
                                fwFile.read(fileBuf,fileSize);
                                memcpy(&readBuf[readIdx],&fileBuf[0],fileSize);
                                readIdx = readIdx + fileSize;
                                fileSize = 0; 
                            }                                              
                        } while(fileSize);
                        if (debugMode) {
                            debugPrint(F("File Read Size:"),false);
                            debugPrint(readIdx);
                            for(int i = 0; i < readIdx; i++)
                                debugPrint(readBuf[i]);
                        }
                        fileSize = 0;                    
                    }                        
                } while (fileSize);
                fwFile.close();
                return true;                
            }
            else
            {
                if (debugMode) {
                    debugPrint(F("File size error, size = 0"),false);
                    debugPrint(filepath);
                }
                fwFile.close();
                return false;
            }
        }
        else
        {
            if (debugMode) {
                debugPrint(F("Error opening FW download file:"),false);
                debugPrint(filepath);
            }
            return false;
        }
    }
    else
    {
        if (debugMode) {
            debugPrint(F("FW download file doesn't exists !"),false);
            debugPrint(filepath);
        }
        return false;
    }
}

String IUOTA::file_md5 (File & f)
{
    if (!f) {
        return String();
    }

    if (f.seek(0, SeekSet)) {

        MD5Builder md5;
        md5.begin();
        md5.addStream(f, f.size());
        md5.calculate();
        return md5.toString();
    } 
return String();

}

#if 1
bool IUOTA::otaGetMD5(char *folderName,char *fileName, char *md5HashRet)
{
    char filepath[40];
    uint32_t fileSize = 0;
    String md5hash = "";
    File fwFile;
    snprintf(filepath, 40, "%s/%s", folderName, fileName);
    if (debugMode) {
        debugPrint(F("Get MD5 of file:"),false);
        debugPrint(filepath);
    }
    if(DOSFS.exists(filepath))
    {
        fwFile = DOSFS.open(filepath,"r");
        if(fwFile)
        {        
            fileSize = fwFile.size();
            if (debugMode) {
                debugPrint(F("File Size:"),false);
                debugPrint(fileSize);
            }
            if(fileSize > 0)
            {
                md5hash = file_md5(fwFile);
                if (debugMode) {
                    debugPrint(F("File Name:"),false);
                    debugPrint(fileName,false);
                    debugPrint(F(" of size:"),false);
                    debugPrint(fileSize);
                    debugPrint(F("MD5 Hash :"),false);
                    debugPrint(md5hash);
                }
                strcpy(md5HashRet,md5hash.c_str());
            }
            fwFile.close();
            return true;
        }
    }
    return false;
}
#endif

/**
 * Copy bin files to actual OTA folder
 */
bool IUOTA::otaFileRemove(char *folderName,char *fileName)
{
    char filepath[40];
    snprintf(filepath, 40, "%s/%s", folderName, fileName);
    if (debugMode) {
        debugPrint(F("Remove file:"),false);
        debugPrint(filepath);
    }
    if(DOSFS.exists(filepath))
    { // Remove existing file
        DOSFS.remove(filepath);
    }
}


/**
 * Copy bin files to actual OTA folder
 */
bool IUOTA::otaFileCopy(char *dstFolderPath,char *srcFolderPath, char *filename)
{
    char dstFilePath[40];
    char srcFilePath[40];

    // Check Source and Destination directories are present
    if(DOSFS.exists(dstFolderPath) && DOSFS.exists(srcFolderPath))
    {
        File fwSrcFile;
        File fwDstFile;
        otaFileRemove(dstFolderPath,filename);
        snprintf(dstFilePath, 40, "%s/%s", dstFolderPath, filename);
        snprintf(srcFilePath, 40, "%s/%s", srcFolderPath, filename);
        if (debugMode) {
            debugPrint(F("File Copy Dest FilePath:"),false);
            debugPrint(dstFilePath);
            debugPrint(F("File Copy Source FilePath:"),false);
            debugPrint(srcFilePath);
        }       
        if(DOSFS.exists(srcFilePath))
        {
            fwSrcFile = DOSFS.open(srcFilePath,"r");
            fwDstFile = DOSFS.open(dstFilePath,"a+");
            if(fwSrcFile && fwDstFile)
            {
                unsigned char fileBuf[MAX_FILE_RW_SIZE];
                uint32_t fileSize = fwSrcFile.size();
                if (debugMode) {
                    debugPrint(F("File Size:"),false);
                    debugPrint(fileSize);
                }
                if(fileSize > 0)
                {
                    do
                    {
                        memset(fileBuf,'\0',MAX_FILE_RW_SIZE);
                        if(fileSize >= MAX_FILE_RW_SIZE)
                        {
                            fwSrcFile.read(fileBuf,MAX_FILE_RW_SIZE);
                            fwDstFile.write(fileBuf,MAX_FILE_RW_SIZE);
                            fileSize = fileSize - MAX_FILE_RW_SIZE;
                        }
                        else
                        {
                            fwSrcFile.read(fileBuf,fileSize);
                            fwDstFile.write(fileBuf,fileSize);
                            fileSize = 0;                    
                        }                        
                    } while (fileSize);                    
                }
                fwSrcFile.close();
                fwDstFile.close();
            }
            else
            {
                if (loopDebugMode) {
                    debugPrint(F("Source/Dest File Open Fail"),false);
                    debugPrint(srcFilePath);
                    debugPrint(dstFilePath);
                }
                return false;
            }
        }        
    }
}


/**
 * Sent Init/ACK/NAK/Download status/FW upgrade status
 */
bool IUOTA::otaSendResponse(MSPCommand::command resp, const char *otaResponse)
{
    switch(resp)
    {
        case MSPCommand::OTA_INIT_ACK:            
            iuWiFi.sendMSPCommand(MSPCommand::OTA_INIT_ACK,otaResponse);
            break;
        case MSPCommand::OTA_PACKET_ACK:
            iuWiFi.sendMSPCommand(MSPCommand::OTA_PACKET_ACK);
            break;
 //       case MSPCommand::OTA_PACKET_NAK:
  //          iuWiFi.sendMSPCommand(MSPCommand::OTA_FDW_ABORT,otaResponse);
   //         break;
        case MSPCommand::OTA_FDW_START:
            iuWiFi.sendMSPCommand(MSPCommand::OTA_FDW_START,otaResponse);
            break;
        case MSPCommand::OTA_FDW_SUCCESS:
            iuWiFi.sendMSPCommand(MSPCommand::OTA_FDW_SUCCESS,otaResponse);
            break;
        case MSPCommand::OTA_FDW_ABORT:
            iuWiFi.sendMSPCommand(MSPCommand::OTA_FDW_ABORT,otaResponse);
            break;    
        case MSPCommand::OTA_FUG_START:
            iuWiFi.sendMSPCommand(MSPCommand::OTA_FUG_START,otaResponse);
            break;
        case MSPCommand::OTA_FUG_SUCCESS:
            iuWiFi.sendMSPCommand(MSPCommand::OTA_FUG_SUCCESS,otaResponse);
            break;
        case MSPCommand::OTA_FUG_ABORT:
            iuWiFi.sendMSPCommand(MSPCommand::OTA_FUG_ABORT,otaResponse);
            break;
        case MSPCommand::TLS_INIT_ACK:
            iuWiFi.sendMSPCommand(MSPCommand::TLS_INIT_ACK,otaResponse);
            break;
        case MSPCommand::CERT_DOWNLOAD_INIT_ACK:
             iuWiFi.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_INIT_ACK,otaResponse);
             break;
        case MSPCommand::DOWNLOAD_TLS_SSL_START:
            iuWiFi.sendMSPCommand(MSPCommand::DOWNLOAD_TLS_SSL_START,otaResponse);
            break;
        case MSPCommand::CERT_DOWNLOAD_SUCCESS:
            iuWiFi.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_SUCCESS,otaResponse);
            break;    
        case MSPCommand::CERT_DOWNLOAD_ABORT:
            iuWiFi.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT,otaResponse);
            break;
        case MSPCommand::CERT_UPGRADE_SUCCESS:
            iuWiFi.sendMSPCommand(MSPCommand::CERT_UPGRADE_SUCCESS,otaResponse);
            break;
        case MSPCommand::CERT_UPGRADE_ABORT:
            iuWiFi.sendMSPCommand(MSPCommand::CERT_UPGRADE_ABORT,otaResponse);
            break;
        case MSPCommand::ALL_MQTT_CONNECT_ATTEMPT_FAILED:
            iuWiFi.sendMSPCommand(MSPCommand::ALL_MQTT_CONNECT_ATTEMPT_FAILED,otaResponse);
            break;
        default:
            iuWiFi.sendMSPCommand(MSPCommand::OTA_PACKET_ACK);
            break;
    }
}

/**
 * Get RCA Reason code based on HTTP error code value
 * @param error int
 * @return String
 * OTA-RCA-0001 to OTA-RCA-0011 - Used at STM code for sending OTA Failure reason code
 * CERT-RCA-0000 to CERT-RCA-0041 -used at ESP code for sending CERT error, reason code
 */
String IUOTA::getOtaRca(int error)
{
    switch(error) {
    case OTA_DOWNLOAD_SUCCESS:
        return F("OTA-RCA-0000");
    case OTA_INVALID_MQTT:
        return F("OTA-RCA-0001");
    case OTA_CHECKSUM_FAIL:
        return F("OTA-RCA-0002");
    case OTA_WIFI_DISCONNECT:
        return F("OTA-RCA-0003");
    case OTA_FLASH_RDWR_FAIL:
        return F("OTA-RCA-0004");
    case OTA_DOWNLOAD_TMOUT:
        return F("OTA-RCA-0005"); 
    case OTA_VALIDATION_FAILED:
        return F("OTA-RCA-0006");
    case OTA_UPGRADE_FAIL:
        return F("OTA-RCA-0007");
    case OTA_INT_RLBK_FAIL:
        return F("OTA-RCA-0008");
    case OTA_FORCED_RLBK_FAIL:
        return F("OTA-RCA-0009");
    case OTA_FILE_MISSING:
        return F("OTA-RCA-0010");
    case OTA_MQTT_DISCONNECT:
        return F("OTA-RCA-0011");
    case CERT_DOWNLOAD_INIT_REQ_ACK:
        return F("CERT-RCA-0042");
    case CERT_DOWNLOAD_START:
        return F("CERT-RCA-0043");
    default:
        return F("OTA-RCA-1111");
    }
}

/**
 * Read OTA status flag
 * @param error int
 * @return String
 * OTA-RCA-0001 to OTA-RCA-0010 - Used at STM code for sending OTA Failure reason code
 */
void IUOTA:: readOtaFlag(void)
{
  for(int i = 0 ; i <128 ; i++) {
    OtaStatusFlag[i] = 0;
  }
  for (int i = 0 ; i < 128; i= i+8){
    OtaStatusFlag[i] = *(uint8_t*)(FLAG_ADDRESS + i);
  }    
}

/**
 * Update OTA status flag
 * @param error int
 * @return String
 * OTA-RCA-0001 to OTA-RCA-0010 - Used at STM code for sending OTA Failure reason code
 */
void IUOTA::updateOtaFlag(uint8_t flag_addr , uint8_t flag_data)
{
    uint8_t retry = 2;
    if(loopDebugMode) {
        debugPrint("Updating flag:",false);
        debugPrint(flag_addr,false);
        debugPrint(" Value:",false);
        debugPrint(flag_data);
    }
    uint8_t flag_addr_temp = flag_addr * 8;
    readOtaFlag();
    OtaStatusFlag[flag_addr_temp] = flag_data;
    
    // DEBUG_SERIAL.println("Flash Erased...");

    for(int i=0;i<retry;i++){
        stm32l4_flash_unlock();
        stm32l4_flash_erase((uint32_t)FLAG_ADDRESS, 2048);
        stm32l4_flash_program((uint32_t)FLAG_ADDRESS, OtaStatusFlag, sizeof(OtaStatusFlag));
        stm32l4_flash_lock();
    }
}
