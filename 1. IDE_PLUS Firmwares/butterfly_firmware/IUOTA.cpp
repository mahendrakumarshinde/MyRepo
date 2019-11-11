#include "IUOTA.h"
#include "Conductor.h"
#include "IUFlash.h"

#define MAX_FILE_RW_SIZE    512
/* =============================================================================
    Constructors and destructors
============================================================================= */

/**
 * Set device mode = OTA
 */
void IUOTA::otaFileDownload()
{
    conductor.changeUsageMode(UsageMode::OTA);
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
        default:
            iuWiFi.sendMSPCommand(MSPCommand::OTA_PACKET_ACK);
            break;
    }
}
