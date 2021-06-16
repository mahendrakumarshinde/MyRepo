#include "espComm.h"
#include "Arduino.h"
#include "FS.h"
#include "LedManager.h"

extern LedManager ledManager;

extern "C" uint8_t MD5_sum_file(char* readFileName, char *md5Result);
extern "C" void Read_MD5(char* TEST_FILE, char *md5Result);
extern "C" char calculatedMD5Sum[64];
extern "C" char receivedMD5Sum[64];
extern "C" char verifiedMD5Sum[64];

/* This function is used to verify and write WiFi FW binary into ESP32 flash memory
 */
uint16_t espComm::flash_esp32_verify(char* folderPath,char* fileName)
{
    bool ret = false;

    espFlashLog = DOSFS.open("esp32Response.log", "w");
    if (espFlashLog)
    {
        bl2DebugPrint("Opened ESP32 debug log file.");
        espFlashLog.println("ESP32 ROM command Logs :");
    }
    else
    {
        bl2DebugPrint("Failed to Open ESP32 debug log File !");
        return FILE_OPEN_FAILED;
    }
    bl2DebugPrint("Setting ESP32 in Downloand mode...");
    pinMode(ESP32_ENABLE_PIN, OUTPUT); // IDE1.5_PORT_CHANGE
    pinMode(ESP32_IO0,OUTPUT); // IDE1.5_PORT_CHANGE
    espReboot();
    bl2DebugPrint("Setting ESP32 Downloand mode...OK");
    for(int i=1;i<=20;++i)
    { 
        delay(100);
        bl2DebugPrint(i*5,false);
        bl2DebugPrint("%...",false);
    }
    bl2DebugPrint("\nSending SYNC command to ESP32...");
    ret = esp_SendSyncCmd(10,10);
    if(ret)  
    {
        bl2DebugPrint("Sending SYNC command to ESP32...OK");
        delay(1000);
        ret = espDetect();
        if(ret == false)
        {
            espFlashLog.println("ESP32 detect failed !");
            espFlashLog.close();
            return RETURN_FAILED;
        }
        ret = espGetFeature();
        if(ret == false)
        {
            espFlashLog.println("ESP32 get feature failed !");
            espFlashLog.close();
            return RETURN_FAILED;
        }
        ret = espGetMacId();
        if(ret == false)
        {
            espFlashLog.println("ESP32 get MAC ID failed !");
            espFlashLog.close();
            return RETURN_FAILED;
        }
        ret = espUploadStub();
        if(ret == false)
        {
            espFlashLog.println("ESP32 uploadStub failed !");
            espFlashLog.close();
            return RETURN_FAILED;
        }
        ret = espConfigureFlash();
        if(ret == false)
        {
            espFlashLog.println("ESP32 configFlash failed !");
            espFlashLog.close();
            return RETURN_FAILED;
        }
        espFlashLog.close();
        delay(100);
        espFlashLog = DOSFS.open("esp32Response.log", "a");
        espFlashLog.println("WiFi FW Flashing..:");
        ret= espBinWrite(folderPath,fileName);
        delay(100);
        if(ret == false)
        {
            bl2DebugPrint("WiFi FW flash Failed !");
            espFlashLog.println("ESP32 wifi fw flash failed !");
            espFlashLog.close();
            return RETURN_FAILED;
        }
        //espFlashLog = DOSFS.open("esp32Response.log", "a");
        delay(100);
        // bl2DebugPrint(fwhash);
        String fwhash = espGetMD5Hash();
        char *WIFI_MD5File;
        if(strcmp(ESP_MAIN_FIRMWARE,folderPath)==0)
            WIFI_MD5File = ESP_MFW_1_SUM;
        else if(strcmp(ESP_ROLLBACK_FIRMWARE,folderPath)==0)
            WIFI_MD5File = ESP_MFW_2_SUM;
        else if(strcmp(ESP_FORCED_ROLLBACK_FIRMWARE,folderPath)==0)
            WIFI_MD5File = ESP_MFW_3_SUM;        

        espReadMD5(WIFI_MD5File, receivedMD5Sum);
        bl2DebugPrint("WiFi FW MD5 Received: ",false);
        bl2DebugPrint(receivedMD5Sum);
        bl2DebugPrint("WiFi FW MD5 RD from ESP:",false);
        bl2DebugPrint(fwhash.c_str());

        if (strcmp(receivedMD5Sum, fwhash.c_str()) != 0) 
        {
            bl2DebugPrint("WiFi FW File MD5 Hash Failed !");
            espFlashLog.println("WiFi FW File MD5 Hash Failed !");
            espFlashLog.close();
            return RETURN_FAILED;
        }
        bl2DebugPrint("WiFi FW File MD5 Hash Ok");
        espFlashLog.println("WiFi FW File MD5 Hash Ok");
#if 1
        if(strcmp(ESP_MAIN_FIRMWARE,folderPath) !=0) { // flash additional wifi binaries only in case of Internal or forced rollback
        // Write Boot app table  
            espFlashLog.println("WiFi Boot app Flashing..:");
            bl2DebugPrint("WiFi Boot app Flashing...");
            ret= espBinWrite(folderPath,ESP_BOOT_APP_FILENAME);
            delay(100);
            if(ret == false)
            {
                bl2DebugPrint("WiFi Boot app flashing failed !");
                espFlashLog.println("WiFi Boot app flashing failed !");
                espFlashLog.close();
                return RETURN_FAILED;
            }
            //espFlashLog = DOSFS.open("esp32Response.log", "a");
            delay(100);
            // bl2DebugPrint(fwhash);
            String fwhash1 = espGetMD5Hash();
            char *WIFI_BOOT_MD5File;
            if(strcmp(ESP_MAIN_FIRMWARE,folderPath)==0)
                WIFI_BOOT_MD5File = ESP_TEMP_BOOT_APP_SUM;
            else if(strcmp(ESP_ROLLBACK_FIRMWARE,folderPath)==0)
                WIFI_BOOT_MD5File = ESP_RLBK_BOOT_APP_SUM;
            else if(strcmp(ESP_FORCED_ROLLBACK_FIRMWARE,folderPath)==0)
                WIFI_BOOT_MD5File = ESP_BKUP_BOOT_APP_SUM;        

            espReadMD5(WIFI_BOOT_MD5File, receivedMD5Sum);
            bl2DebugPrint("WiFi Boot app FW MD5 Received: ",false);
            bl2DebugPrint(receivedMD5Sum);
            bl2DebugPrint("WiFi Boot app FW MD5 RD from ESP:",false);
            bl2DebugPrint(fwhash1.c_str());

            if (strcmp(receivedMD5Sum, fwhash1.c_str()) != 0) 
            {
                bl2DebugPrint("WiFi Boot app FW File MD5 Hash Failed !");
                espFlashLog.println("WiFi Boot app FW File MD5 Hash Failed !");
                espFlashLog.close();
                return RETURN_FAILED;
            }
            bl2DebugPrint("WiFi Boot app FW File MD5 Hash Ok");
            espFlashLog.println("WiFi Boot app FW File MD5 Hash Ok");

            // Write Bootloader bin
            espFlashLog.println("WiFi Bootloader FW Flashing..:");
            bl2DebugPrint("WiFi Bootloader FW Flashing...");
            ret= espBinWrite(folderPath,ESP_BOOT_LDR_FILENAME);
            delay(100);
            if(ret == false)
            {
                bl2DebugPrint("WiFi Bootloader FW Flashing failed !");
                espFlashLog.println("WiFi Bootloader FW Flashing failed !");
                espFlashLog.close();
                return RETURN_FAILED;
            }
            //espFlashLog = DOSFS.open("esp32Response.log", "a");
            delay(100);
            // bl2DebugPrint(fwhash);
            String fwhash2 = espGetMD5Hash();
            char *WIFI_BTLODR_MD5File;
            if(strcmp(ESP_MAIN_FIRMWARE,folderPath)==0)
                WIFI_BTLODR_MD5File = ESP_TEMP_BOOT_LODR_SUM;
            else if(strcmp(ESP_ROLLBACK_FIRMWARE,folderPath)==0)
                WIFI_BTLODR_MD5File = ESP_RLBK_BOOT_LODR_SUM;
            else if(strcmp(ESP_FORCED_ROLLBACK_FIRMWARE,folderPath)==0)
                WIFI_BTLODR_MD5File = ESP_BKUP_BOOT_LODR_SUM;        

            espReadMD5(WIFI_BTLODR_MD5File, receivedMD5Sum);
            bl2DebugPrint("WiFi Bootloader FW MD5 Received: ",false);
            bl2DebugPrint(receivedMD5Sum);
            bl2DebugPrint("WiFi Bootloader FW MD5 RD from ESP:",false);
            bl2DebugPrint(fwhash2.c_str());

            if (strcmp(receivedMD5Sum, fwhash2.c_str()) != 0) 
            {
                bl2DebugPrint("WiFi Bootloader FW File MD5 Hash Failed !");
                espFlashLog.println("WiFi Bootloader FW File MD5 Hash Failed !");
                espFlashLog.close();
                return RETURN_FAILED;
            }
            bl2DebugPrint("WiFi Bootloader FW File MD5 Hash Ok");
            espFlashLog.println("WiFi Bootloader FW File MD5 Hash Ok");

        // Write partition table
            espFlashLog.println("WiFi PARTITION FW Flashing..:");
            bl2DebugPrint("WiFi PARTITION FW Flashing...");
            ret= espBinWrite(folderPath,ESP_PART_TBL_FILENAME);
            delay(100);
            if(ret == false)
            {
                bl2DebugPrint("WiFi PARTITION FW Flashing failed !");
                espFlashLog.println("WiFi PARTITION FW Flashing failed !");
                espFlashLog.close();
                return RETURN_FAILED;
            }
            //espFlashLog = DOSFS.open("esp32Response.log", "a");
            delay(100);
            // bl2DebugPrint(fwhash);
            String fwhash3 = espGetMD5Hash();
            char *WIFI_PART_MD5File;
            if(strcmp(ESP_MAIN_FIRMWARE,folderPath)==0)
                WIFI_PART_MD5File = ESP_TEMP_PART_SUM;
            else if(strcmp(ESP_ROLLBACK_FIRMWARE,folderPath)==0)
                WIFI_PART_MD5File = ESP_RLBK_PART_SUM;
            else if(strcmp(ESP_FORCED_ROLLBACK_FIRMWARE,folderPath)==0)
                WIFI_PART_MD5File = ESP_BKUP_PART_SUM;        

            espReadMD5(WIFI_PART_MD5File, receivedMD5Sum);
            bl2DebugPrint("WiFi PARTITION FW MD5 Received: ",false);
            bl2DebugPrint(receivedMD5Sum);
            bl2DebugPrint("WiFi PARTITION FW MD5 RD from ESP:",false);
            bl2DebugPrint(fwhash3.c_str());

            if (strcmp(receivedMD5Sum, fwhash3.c_str()) != 0) 
            {
                bl2DebugPrint("WiFi PARTITION FW File MD5 Hash Failed !");
                espFlashLog.println("WiFi PARTITION FW File MD5 Hash Failed !");
                espFlashLog.close();
                return RETURN_FAILED;
            }
            bl2DebugPrint("WiFi PARTITION FW File MD5 Hash Ok");
            espFlashLog.println("WiFi PARTITION FW File MD5 Hash Ok");
        }
#endif
        ret = espExitFlash();
        if(ret == false)
        {
            bl2DebugPrint("Exit flash Failed !");
            espFlashLog.println("Exit flash Failed !");
            espFlashLog.close();
            //return RETURN_FAILED;
        }
    }
    else
    {
        bl2DebugPrint("SYNC failed, aborting ESP32 FW download !!!");
        espFlashLog.println("SYNC failed, aborting ESP32 FW download !!!");
        espFlashLog.close();
        return RETURN_FAILED;
    }  
    espFlashLog.println("ESP32 flashing success");
    espFlashLog.close();
    return RETURN_SUCESS;
}

/* This function is used to write FW binary in the form packtes to ESP32
 */
bool espComm::espBinWrite(char *folderName,char *fileName)
{    
    uint32_t readIdx = 0;
    uint32_t fileSize = 0;
    uint16_t pktSeqNo = 0;
    bool ret = false;
    if(folderName == NULL || fileName == NULL)
    {
        bl2DebugPrint(F("fw bin file read param error !"));
        return false;
    }
    char filepath[64];
    snprintf(filepath, 64, "%s/%s", folderName, fileName);
    if(espFlashLog)
        espFlashLog.println(filepath);
    bl2DebugPrint("File path:" + String(filepath));
    File fwFile;    
    if(DOSFS.exists(filepath))
    {
        fwFile = DOSFS.open(filepath,"r");
        if(fwFile)
        { 
            unsigned char fileBuf[MAX_FILE_RW_SIZE];
            fileSize = fwFile.size();
            espTotBlock = fileSize/FLASH_BLOCK_SIZE;
            if(fileSize%FLASH_BLOCK_SIZE)
                espTotBlock = espTotBlock + 1; 
            bl2DebugPrint(F("File Size:"),false);
            bl2DebugPrint(fileSize);
            readIdx = 0;
            if(fileSize > 0)
            {
                FlashAdd[9] = (uint8_t)(fileSize & 0x000000FF);
                FlashAdd[10] = (uint8_t)((fileSize & 0x0000FF00) >> 8);
                FlashAdd[11] = (uint8_t)((fileSize & 0x00FF0000) >> 16);
                FlashAdd[12] = (uint8_t)((fileSize & 0xFF000000) >> 24);
                if(!strcmp(fileName,ESP_BOOT_APP_FILENAME))
                { // Flash Add of bin 
                    bl2DebugPrint("Writing WIFI Boot App Bin... ");
                    FlashAdd[21] = (uint8_t)0x00;
                    FlashAdd[22] = (uint8_t)0xe0;
                    FlashAdd[23] = (uint8_t)0x00;
                    FlashAdd[24] = (uint8_t)0x00;

                    md5Hash[9] = (uint8_t)0x00;
                    md5Hash[10] = (uint8_t)0xe0;
                    md5Hash[11] = (uint8_t)0x00;
                    md5Hash[12] = (uint8_t)0x00;
                }
                else if(!strcmp(fileName,ESP_BOOT_LDR_FILENAME))
                { // Flash Add of bin 
                    bl2DebugPrint("Writing WIFI Bootloader Bin... ");
                    FlashAdd[21] = (uint8_t)0x00;
                    FlashAdd[22] = (uint8_t)0x10;
                    FlashAdd[23] = (uint8_t)0x00;
                    FlashAdd[24] = (uint8_t)0x00;

                    md5Hash[9] = (uint8_t)0x00;
                    md5Hash[10] = (uint8_t)0x10;
                    md5Hash[11] = (uint8_t)0x00;
                    md5Hash[12] = (uint8_t)0x00;
                }
                else if(!strcmp(fileName,ESP_PART_TBL_FILENAME))
                { // Flash Add of bin 
                    bl2DebugPrint("Writing WIFI Parition Bin... ");
                    FlashAdd[21] = (uint8_t)0x00;
                    FlashAdd[22] = (uint8_t)0x80;
                    FlashAdd[23] = (uint8_t)0x00;
                    FlashAdd[24] = (uint8_t)0x00;

                    md5Hash[9] = (uint8_t)0x00;
                    md5Hash[10] = (uint8_t)0x80;
                    md5Hash[11] = (uint8_t)0x00;
                    md5Hash[12] = (uint8_t)0x00;
                }
                md5Hash[13] = (uint8_t)(fileSize & 0x000000FF);
                md5Hash[14] = (uint8_t)((fileSize & 0x0000FF00) >> 8);
                md5Hash[15] = (uint8_t)((fileSize & 0x00FF0000) >> 16);
                md5Hash[16] = (uint8_t)((fileSize & 0xFF000000) >> 24);
              // Set File size in Flash configure- Address set command 
                ret = espSendCmd(FlashAdd, sizeof(FlashAdd), ESP_CMD_RETRY);
                if(ret == false) {
                    fwFile.close();
                    return false;
                }
                delay(50);
                do
                {    ledManager.updateColors();                
                    if(fileSize >= FLASH_BLOCK_SIZE)
                    {
                        readIdx = 0;
                        for(int i = 0; i < (FLASH_BLOCK_SIZE/MAX_FILE_RW_SIZE); i++)
                        {
                            memset(fileBuf,'\0',MAX_FILE_RW_SIZE);
                            if(fwFile.read(fileBuf,MAX_FILE_RW_SIZE) == -1)
                            {
                                fwFile.close();
                                return false;
                            }
                            memcpy(&readBuf[readIdx],&fileBuf[0],MAX_FILE_RW_SIZE);
                            readIdx = readIdx + MAX_FILE_RW_SIZE;
                            fileSize = fileSize - MAX_FILE_RW_SIZE;                                           
                        }                        
                       // bl2DebugPrint(F("File Read Size:"));
                       // bl2DebugPrint(readIdx);
                        // for(int i = 0; i < FLASH_BLOCK_SIZE; i++)
                        //     bl2DebugPrint(readBuf[i]);  
                        ret = preparePkt(readBuf,pktSeqNo,readIdx);
                        if(ret == false)
                        {
                            fwFile.close();
                            return false;
                        }
                        pktSeqNo++;
                        memset(readBuf,'\0',FLASH_BLOCK_SIZE);            
                    }
                    else
                    {     
                        readIdx = 0;                   
                        do
                        {                            
                            if(fileSize >= MAX_FILE_RW_SIZE) {
                                memset(fileBuf,'\0',MAX_FILE_RW_SIZE);
                                if(fwFile.read(fileBuf,MAX_FILE_RW_SIZE) == -1)
                                {
                                    fwFile.close();
                                    return false;
                                }
                                memcpy(&readBuf[readIdx],&fileBuf[0],MAX_FILE_RW_SIZE);
                                readIdx = readIdx + MAX_FILE_RW_SIZE;
                                fileSize = fileSize - MAX_FILE_RW_SIZE;
                            }
                            else
                            {
                                memset(fileBuf,'\0',MAX_FILE_RW_SIZE);
                                if(fwFile.read(fileBuf,fileSize) == -1)
                                {
                                    fwFile.close();
                                    return false;
                                }
                                memcpy(&readBuf[readIdx],&fileBuf[0],fileSize);
                                readIdx = readIdx + fileSize;
                                fileSize = 0; 
                            }                                              
                        } while(fileSize);
                        for(int i = readIdx; i < FLASH_BLOCK_SIZE; i++)
                            readBuf[i] = 0xff;
                      //  bl2DebugPrint(F("File Read Size:"));
                       // bl2DebugPrint(readIdx);
                        // for(int i = 0; i < FLASH_BLOCK_SIZE; i++)
                        //     bl2DebugPrint(readBuf[i]);
                        fileSize = 0;
                        //bl2DebugPrint("Preparing packet for writing to ESP32..");
                        ret = preparePkt(readBuf,pktSeqNo,FLASH_BLOCK_SIZE);        
                        if(ret == false)
                        {
                            fwFile.close();
                            return false;
                        }
                    }                        
                } while (fileSize);
                fwFile.close();
                return true;                
            }
            else
            {
                bl2DebugPrint(F("File size error, size = 0"),false);
                bl2DebugPrint(filepath);
                if(espFlashLog)
                    espFlashLog.println("file size error ");
                fwFile.close();
                return false;
            }
        }
        else
        {
            if(espFlashLog)
                espFlashLog.println("Error opening FW download file:");
            bl2DebugPrint(F("Error opening FW download file:"),false);
            bl2DebugPrint(filepath);
            return false;
        }
    }
    else
    {
        if(espFlashLog)
            espFlashLog.println("FW download file doesn't exists !");
        bl2DebugPrint(F("FW download file doesn't exists !"),false);
        bl2DebugPrint(filepath);
        return false;
    }    
}
/* example packet data
{
0xc0,                                       ==> Start of packet 
0x00,                                       ==> Direction - Fixed to 0
0x03,                                       ==> Command Opcode
0x10,0x04,                                  ==> Packet Length (0x00000410) // 1024 + 16 
0x05,0x00,0x00,0x00,                        ==> CRC
0x00,0x04,0x00,0x00,                        ==> Data Length (0x00000400) // 1024 Byte
0x00,0x00,0x00,0x00,                        ==> Sequence Number
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,    ==> Fixed 8 byte padding (0x00)
0xe9,0x06,0x02,0x2f,0x58,0x17,0x08,0x40,    ==> Overall packet size is variable length
0xee,0x00,0x00,0x00,0x00,0x00,0x00,0x00,    ==> CRC, Sequence number, packet data matching to
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,    ==> 0xC0 is replaced by ==> 0xDB,0xDC (2 Byte data) and 
0x20,0x00,0x40,0x3f,0x40,0xa7,0x00,0x00,    ==> 0xDB is replaced by ==> 0xDB,0xDD (2 Byte data)
---------------------------------------
0x40,0xe4,0xb5,0x0d,0x40,0xe4,0xb5,0x0d,
0x40,
0xc0                                        ==> End of packet
},
*/
/* This function is used to prepare packet data to be written into ESP32 flash
 */
bool espComm::preparePkt(unsigned char* buff,uint16_t pktSeqNo, uint32_t pktsize)
{    
    uint32_t writeIdx = 0;
    uint32_t pktCrc = 0;
    bool ret = false;
    /* 0xC0 + Direction(1) + opcode(1) +  packet Length(2) */
    memcpy(&PktBuf[writeIdx],Startheader,5);
    writeIdx = writeIdx + 5;
    
    /* CRC (4 byte) - (LSB First) */
    pktCrc = espcomm_calc_checksum(buff,pktsize);
//    bl2DebugPrint("Packet CRC: 0x");
//    bl2DebugPrint(pktCrc, HEX);
    if(pktCrc == 0xDB)
    {
        PktBuf[writeIdx++] = 0xDB;
        PktBuf[writeIdx++] = 0xDD;
        PktBuf[writeIdx++] = 0x00;
        PktBuf[writeIdx++] = 0x00;
        PktBuf[writeIdx++] = 0x00;
    }
    else if(pktCrc == 0xC0)
    {
        PktBuf[writeIdx++] = 0xDB;
        PktBuf[writeIdx++] = 0xDC;
        PktBuf[writeIdx++] = 0x00;
        PktBuf[writeIdx++] = 0x00;
        PktBuf[writeIdx++] = 0x00;
    }
    else {
        PktBuf[writeIdx++] = (uint8_t)(pktCrc & 0x000000FF);
        PktBuf[writeIdx++] = (uint8_t)((pktCrc & 0x0000FF00) >> 8);
        PktBuf[writeIdx++] = (uint8_t)((pktCrc & 0x00FF0000) >> 16);
        PktBuf[writeIdx++] = (uint8_t)((pktCrc & 0xFF000000) >> 24);
    }
    /* Fixed 4 byte value - Header2 */
    memcpy(&PktBuf[writeIdx],header2,4);
    writeIdx = writeIdx + 4; // Size of header2
    
    bl2DebugPrint("WiFi FW Flash Sector: " + String(pktSeqNo+1) + "/" + String(espTotBlock));
    /* Write Sequence No in 4 byte (LSB First) */
    if(pktSeqNo == 0xDB)
    {
        PktBuf[writeIdx++] = 0xDB;
        PktBuf[writeIdx++] = 0xDD;
        PktBuf[writeIdx++] = 0x00;
        PktBuf[writeIdx++] = 0x00;
        PktBuf[writeIdx++] = 0x00;
    }
    else if(pktSeqNo == 0xC0)
    {
        PktBuf[writeIdx++] = 0xDB;
        PktBuf[writeIdx++] = 0xDC;
        PktBuf[writeIdx++] = 0x00;
        PktBuf[writeIdx++] = 0x00;
        PktBuf[writeIdx++] = 0x00;
    }
    else {
        PktBuf[writeIdx++] = (uint8_t)(pktSeqNo & 0x000000FF);
        PktBuf[writeIdx++] = (uint8_t)((pktSeqNo & 0x0000FF00) >> 8);
        PktBuf[writeIdx++] = (uint8_t)((pktSeqNo & 0x00FF0000) >> 16);
        PktBuf[writeIdx++] = (uint8_t)((pktSeqNo & 0xFF000000) >> 24);
    }
    /* Padding 0x00 X 8 byte */
    memcpy(&PktBuf[writeIdx],paddingZero,8);
    writeIdx = writeIdx + 8; // Size of header2
    
    uint32_t pktSize1 = update_C0_DB(&PktBuf[writeIdx], buff , pktsize);
  //  bl2DebugPrint("Modified packet size:" + String(pktSize1));
    writeIdx = writeIdx + pktSize1;
    PktBuf[writeIdx++] = 0xc0;
   // bl2DebugPrint("Total packet size:" + String(writeIdx));
    // if(pktSeqNo > 150) {
    //  for(int i = 0; i < writeIdx; i++) {
    //      bl2DebugPrint(PktBuf[i],HEX);
    //      bl2DebugPrint(" ");
    //  }
    // }
   ret = espSendDataPkt(PktBuf, writeIdx, ESP_CMD_RETRY,pktSeqNo);
   //delay(50);
   if(ret == false)
    return false;
   else
       return true;
}

/* This function is used to calculated checksum of data packets sent to ESP32
 */
uint32_t espComm::espcomm_calc_checksum(unsigned char *data, uint16_t data_size)
{
    uint16_t cnt;
    uint32_t result;
    result = 0xEF;
    for(cnt = 0; cnt < data_size; cnt++)
    {
        result ^= data[cnt];
    }
    return result;
}

/* This function is used to Read MD5SUM from .md5 file 
 */
void espComm:: espReadMD5(char* TEST_FILE, char *md5Result)
{
  if (DOSFS.exists(TEST_FILE)) {
    File MD5_value = DOSFS.open(TEST_FILE,"r");

    if (MD5_value) {
      memset(md5Result, 0x00, 64);
      int readLen=MD5_value.readBytesUntil('\0', md5Result, 64);
      bl2DebugPrint("Received MD5 value: ",false);bl2DebugPrint(md5Result);
      byte lastChar = strlen(md5Result);
      md5Result[lastChar] = '\0'; 
      MD5_value.close(); 
    }
  }
}


#if 0
/* This function is used to ESP32 chip erase command before writing FW binary
 */
bool espComm::espEraseFlash()
{
  espSendCmd(eraseFlash, sizeof(eraseFlash), ESP_CMD_RETRY);
  delay(100);
}
#endif
/* This function is used to ESP32 chip detect command before writing FW binary
 */
bool espComm::espDetect()
{
    bool ret = false;
    bl2DebugPrint("Sending ESP32 Detect command...");
    ret = espSendCmd(deviceType, sizeof(deviceType), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(chipId, sizeof(chipId), ESP_CMD_RETRY+ESP_CMD_RETRY);  
    delay(100);
    if(ret == false)
        return false;
}

/* This function is used to ESP32 chip get feature(WIFI,BT function details) 
 * command before writing FW binary
 */
bool espComm::espGetFeature()
{
    bool ret = false;
    bl2DebugPrint("Sending ESP32 GetFeature command...");
    ret = espSendCmd(readReg1, sizeof(readReg1), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;

    ret = espSendCmd(readReg2, sizeof(readReg2), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
}

/* This function is used to ESP32 chip get WIFI MAC Address 
 * command before writing FW binary
 */
bool espComm::espGetMacId()
{
    bool ret = false;
    bl2DebugPrint("Sending ESP32 Get MAC command...");
    ret = espSendCmd(MacIDreg1, sizeof(MacIDreg1), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;

    ret = espSendCmd(MacIDreg2, sizeof(MacIDreg2), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
}

/* This function is used to send stub code used by ESP32 for FW binary write
 */
bool espComm::espUploadStub()
{
    bool ret = false;
    bl2DebugPrint("Sending ESP32 Upload Stub command...");
    ret = espSendCmd(uploadStub, sizeof(uploadStub), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;

    ret = espSendCmd(uploadStub0, sizeof(uploadStub0), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;

    ret = espSendCmd(uploadStub1, sizeof(uploadStub1), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;

    ret = espSendCmd(uploadStub2, sizeof(uploadStub2), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    /* Send Stub Code , 2 Response packets are sent from ESP  */
    ret = espSendCmd(uploadStub3, sizeof(uploadStub3), ESP_CMD_RETRY,STUB_RESP_PKT_C0_COUNT);
    delay(100);
    if(ret == false)
        return false;
}

/* This function is used to configure ESP32 flash memory
 */
bool espComm::espConfigureFlash()
{
    bool ret = false;
    bl2DebugPrint("Sending ESP32 Configure Flash command...");
    ret = espSendCmd(configFlash1, sizeof(configFlash1), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash2, sizeof(configFlash2), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash3, sizeof(configFlash3), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash4, sizeof(configFlash4), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash5, sizeof(configFlash5), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash6, sizeof(configFlash6), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash7, sizeof(configFlash7), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash8, sizeof(configFlash8), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash9, sizeof(configFlash9), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash10, sizeof(configFlash10), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash11, sizeof(configFlash11), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash12, sizeof(configFlash12), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    // ret = espSendCmd(FlashAdd, sizeof(FlashAdd), ESP_CMD_RETRY);
    // delay(100);
    // if(ret == false)
    //     return false;
}

/* This function is used to send commnad after writing FW binary to exit write process
 */
bool espComm::espExitFlash()
{
    bool ret = false;
    ret = espSendCmd(FlashLeave1, sizeof(FlashLeave1), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(FlashLeave2, sizeof(FlashLeave2), ESP_CMD_RETRY);
    delay(100);
    if(ret == false)
        return false;
}

/* All data sent and received from ESP32 needs to be parsed and all occurances of 0xC0 and 0xDB
 * shall be replaced with corresponding seq. as mentioend in below function.
 * This replacement is required to avoid conflicting 0xC0 character, which is packet start and End identifier
 * Used in all Commmand and data sent to and from ESP32.
 */
unsigned int espComm::update_C0_DB(unsigned char *destPkt,unsigned char *srcBuf, unsigned int size)
{
    unsigned int out_pos = 0;
    for (int i = 0; i < size; ++i)
    {
        unsigned char cur = srcBuf[i];
        if (cur == 0xC0)
        {
           // bl2DebugPrint("Found 0xC0");
            destPkt[out_pos++] = subst_C0[0];
            destPkt[out_pos++] = subst_C0[1];
        }
        else if (cur == 0xDB)
        {
           // bl2DebugPrint("Found 0xDB");
            destPkt[out_pos++] = subst_DB[0];
            destPkt[out_pos++] = subst_DB[1];
        }
        else
            destPkt[out_pos++] = cur;
    }
    return out_pos;
}

/* This function is used to clear any unread data from ESP32
 */
void espComm::espCleanup()
{
  while (ESP_SERIAL.available() > 0)
  {
    ESP_SERIAL.read();
  }
}

/* This function is used to send SYNC command to ESP32. Before writing any FW binary, ESP32 shall be 
 * sent with SYNC packets to establish comm. between STM and ESP
 */
bool espComm:: esp_SendSyncCmd(uint8_t rebootCount, uint8_t retrySync)
{
  //  bl2DebugPrint("Sending SYNC command...");
	int cnt = SYNC_RESP_LEN;
  //	bl2DebugPrint(sizeof(syncCommand));
    unsigned long timeout = ESP_SYNC_TIMEOUT; 
    unsigned long now = millis();
	int index = 0;
	uint8_t m_rebootCount = rebootCount;
	uint8_t m_retrySync = retrySync;
	byte expectedResponse[] = {0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20};
	byte responseCmd[32];
	while (m_retrySync >= 0 && m_rebootCount > 0)
	{ 
    ledManager.updateColors();
		if (m_retrySync > 0)
		{
			--m_retrySync;
			espFlashLog.print("Sync Command : ");
			for (int i = 0; i < sizeof(syncCommand); i++)
			{
				ESP_SERIAL.write(syncCommand[i]);
				espFlashLog.print(syncCommand[i], HEX);
				espFlashLog.print(" ");
			}
			espFlashLog.println();
			espFlashLog.print("Sync Response : ");
			delay(600);
			index = 0;
            now = millis();
            while (ESP_SERIAL.available() > 0 || ((millis()-now) > timeout))
            {
                if((millis()-now) > timeout)
                {
                    bl2DebugPrint("SYNC Packet Response Wait Timeout !");
                    delay(100);
                    return false;
                }
                byte x = ESP_SERIAL.read();
                if (x != 0xc0 && x != 0x08 && x != 0x04 && x != 0x00 && x != 0x55)
                {
                    responseCmd[index] = x;
                    espFlashLog.print(responseCmd[index], HEX);
                    espFlashLog.print(" ");
                    index++;
                }
                --cnt;
                if (cnt <= 0)
                {
                    //bl2DebugPrint("Found C0");
                    espFlashLog.println();
                    if (compareResponse(expectedResponse, responseCmd, sizeof(expectedResponse), sizeof(responseCmd)) == true)
                    {
                        bl2DebugPrint("SYNC Response Matched");
                        return true;
                    }
                    else
                    {
                        bl2DebugPrint("SYNC Response Mismatch");
                        espReboot();
                        m_retrySync = retrySync;
                        cnt = 112;
                        m_rebootCount--;
                        ESP_SERIAL.flush();
                        for (int i = 0; i < sizeof(responseCmd); i++)
                        {
                            responseCmd[i] = 0x00;
                        }
                    }
                }
            }
			delay(200);
			espFlashLog.println();
		}
		else
		{
			m_retrySync = retrySync;
		}
	}
	return false;
}

/* This function is used to send command packets to ESP32. 
 */
bool espComm::espSendCmd(byte command[], int size, int retry,uint8_t countC0)
{
	int cnt = countC0;
    unsigned long timeout = ESP_SEND_CMD_TIMEOUT; 
    unsigned long now = millis();
//	bl2DebugPrint(size);
	espFlashLog.print("Command  : ");
	for (int i = 0; i < size; i++)
	{
//    bl2DebugPrint(command[i],HEX);
//    bl2DebugPrint(" ");
		espFlashLog.print(command[i], HEX);
		espFlashLog.print(" ");
	}
	espFlashLog.println();
	espFlashLog.print("Response : ");
	while (retry > 0)
	{ 
    --retry;
		//espFlashLog.println();
		for (int i = 0; i < size; i++)
		{
			ESP_SERIAL.write(command[i]);
		}
        if(command[2] == 0xD0) //  for erase command
        {
            bl2DebugPrint("Erasing Flash");
            delay(3000);
        }
        else
		    delay(300);
        now = millis();
        while (ESP_SERIAL.available() > 0 || ((millis()-now) > timeout))
        {
            if((millis()-now) > timeout)
            {
                bl2DebugPrint("Send Command Response Wait Timeout !");
                delay(100);
                return false;
            }
			byte a = ESP_SERIAL.read();
			//DEBUG_SERIAL.write(a);
			espFlashLog.print(a, HEX);
			espFlashLog.print(" ");
			if (a == 0xc0)
			{
				--cnt;
				if (cnt <= 0)
				{
					//bl2DebugPrint("Found C0");
					espFlashLog.println();
					ESP_SERIAL.flush();
					return true;
				}
			}
		}
		espFlashLog.println();
		bl2DebugPrint("\n");
		delay(100);
	}
	return false;
}

/* This function is used to get MD5 hash of FW binary written in to ESP32. 
 */
String espComm:: espGetMD5Hash()
{
	int cnt = RESP_PKT_C0_COUNT;
    unsigned long timeout = ESP_GET_HASH_TIMEOUT; 
    unsigned long now = millis();
	char hash[64];
	char ReceivedMd5Hash[32];
    memset(ReceivedMd5Hash,'\0',sizeof(ReceivedMd5Hash));
	int index = 0;
//	ESP_SERIAL.flush();
    delay(2000);
	// bl2DebugPrint(sizeof(md5Hash));
	espFlashLog.print("MD5 Command  : ");
	for (int i = 0; i < sizeof(md5Hash); i++)
	{
		espFlashLog.print(md5Hash[i], HEX);
		espFlashLog.print(" ");
	}
	for (int i = 0; i < sizeof(md5Hash); i++)
	{
		ESP_SERIAL.write(md5Hash[i]);
	}
	espFlashLog.println();
	espFlashLog.print("MD5 Response : ");
    bl2DebugPrint("\nMD5 Response : ");
	delay(MD5_RESP_WAIT_DEL);   // Increase delay if required
    now = millis();
    while (ESP_SERIAL.available() > 0 || ((millis()-now) > timeout))
    {
        if((millis()-now) > timeout)
        {
            bl2DebugPrint("ESP Hash Response Wait Timeout !");
            delay(100);
            return String("");
        }
		byte a = ESP_SERIAL.read();
        // espFlashLog.print(a, HEX);
        // espFlashLog.print(" ");
        delayMicroseconds(10);
        hash[index] = a;
		index++;
		if (a == 0xc0)
		{
			--cnt;
            if (cnt <= 0)
		    {
                // ESP_SERIAL.flush();
                for (uint8_t i = START_OF_MD5_RESP; i < (index-MD5_RESP_TAIL_LEN); i++)
                {
                    char tempHash[2];
                    if(hash[i] == 0xDB && hash[i+1] == 0xDC){
                        hash[i] = 0xC0;
                        sprintf(tempHash, "%02x", hash[i]);
                        strncat(ReceivedMd5Hash, tempHash, 2);
                        i++;
                    }
                    else if(hash[i] == 0xDB && hash[i+1] == 0xDD){
                        hash[i] = 0xDB;
                        sprintf(tempHash, "%02x", hash[i]);
                        strncat(ReceivedMd5Hash, tempHash, 2);
                        i++;
                    }
                    else{
                        sprintf(tempHash, "%02x", hash[i]);
                        strncat(ReceivedMd5Hash, tempHash, 2);
                    }

                }
			    return (String)ReceivedMd5Hash;
		    }
		}
	}
}

/* This function is used to compare response packets received from ESP32
 */
bool espComm:: compareResponse(byte *a, byte *b, int len_a, int len_b)
{
	int n;
	// if their lengths are different, return false
	if (len_a != len_b)
		return false;
	// test each element to be the same. if not, return false
	for (n = 0; n < len_a; n++)
		if (a[n] != b[n])
			return false;
	//ok, if we have not returned yet, they are equal :)
	return true;
}

/* This function is used to reboot ESP32
 */
void espComm:: espReboot()
{
	bl2DebugPrint("Rebooting esp32");
	digitalWrite(ESP32_IO0, HIGH);
	delay(500);
	digitalWrite(ESP32_IO0, LOW);		 // IDE1.5_PORT_CHANGE
	digitalWrite(ESP32_ENABLE_PIN, LOW); // IDE1.5_PORT_CHANGE
	delay(100);
	digitalWrite(ESP32_ENABLE_PIN, HIGH); // IDE1.5_PORT_CHANGE
	delay(100);
	// ESP_SERIAL.flush();
	espCleanup();
	bl2DebugPrint("Download Mode");
}

/* This function is used to send FW binary Data packets to ESP32
 */
bool espComm::espSendDataPkt(unsigned char *command, uint32_t size, int retry, uint16_t seq)
{
    int cnt = DATA_PKT_RESP_LEN;
    unsigned long timeout = ESP_SEND_DATA_TIMOUT; 
    unsigned long now = millis();
    int index = 0;
    byte expectedResponse[] = {0x01, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    byte responseCmd[10];
   // bl2DebugPrint(size);
    // espFlashLog.print("Data Packet, Seq:");
    // espFlashLog.print(seq);
    // espFlashLog.print(" Size:");
    // espFlashLog.print(size);
    // espFlashLog.print("  Data: ");
    // for (int i = 0; i < size; i++)
    // {
    //     espFlashLog.print(command[i], HEX);
    //     espFlashLog.print(" ");
    // }
    // espFlashLog.println();
    // espFlashLog.print("Packet Response: ");
    while (retry > 0)
    {
        --retry;
        ESP_SERIAL.println();
        for (int i = 0; i < size; i++)
        {
            ESP_SERIAL.write(command[i]);
        }
        // ESP_SERIAL.write(0x0a);
        delay(100);
        now = millis();
        while (ESP_SERIAL.available() > 0 || ((millis()-now) > timeout))
        {
            if((millis()-now) > timeout)
            {
                bl2DebugPrint("Data Packet Response Wait Timeout !");
                delay(100);
                return false;
            }
            byte a = ESP_SERIAL.read();
            // bl2DebugPrint(a,HEX);
            //  bl2DebugPrint(" ");
            // espFlashLog.print(a, HEX);
            // espFlashLog.print(" ");
            if (a != 0xc0)
            {
                responseCmd[index] = a;
                // espFlashLog.print(responseCmd[index], HEX);
                // espFlashLog.print(" ");
                index++;
            }
            --cnt;
            if (cnt <= 0)
            {
             //   bl2DebugPrint("Found C0");
               // espFlashLog.println();
                if (compareResponse(expectedResponse, responseCmd, sizeof(expectedResponse), sizeof(responseCmd)) == true)
                {
                    //bl2DebugPrint("Response Matched");
                    return true;
                }
                else
                {
                    bl2DebugPrint("Response Mismatch, Flash write failed !!");
                    ESP_SERIAL.flush();
                    return false;
                }
            }
        }
      //  espFlashLog.println();
      //  bl2DebugPrint("");
        delay(100);
    }
    return false;
}