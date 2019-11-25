#include "espComm.h"
#include "Arduino.h"
#include "FS.h"

uint16_t espComm::flash_esp32_verify()
{
    bool ret = false;
 // DOSFS.begin();
    espFlashLog = DOSFS.open("esp32Response.log", "w");
    if (espFlashLog)
    {
        DEBUG_SERIAL.println("Opened ESP32 debug log file.");
        espFlashLog.println("ESP32 ROM command Logs :");
    }
    else
    {
        DEBUG_SERIAL.println("Failed to Open ESP32 debug log File !");
        return 2;
    }
    DEBUG_SERIAL.println("Setting ESP32 in Downloand mode...");
    pinMode(ESP32_ENABLE_PIN, OUTPUT); // IDE1.5_PORT_CHANGE
    pinMode(ESP32_IO0,OUTPUT); // IDE1.5_PORT_CHANGE
    digitalWrite(ESP32_IO0,LOW); // IDE1.5_PORT_CHANGE
    digitalWrite(ESP32_ENABLE_PIN, LOW); // IDE1.5_PORT_CHANGE
    delay(100);
    digitalWrite(ESP32_ENABLE_PIN, HIGH); // IDE1.5_PORT_CHANGE
    delay(100);
    espCleanup();
    DEBUG_SERIAL.println("Setting ESP32 Downloand mode...OK");
    for(int i=1;i<=20;++i)
    { 
        delay(100);
        DEBUG_SERIAL.print(i*5);
        DEBUG_SERIAL.print("%...");
    }
    DEBUG_SERIAL.println("\nSending SYNC command to ESP32...");
    ret = esp_SendSyncCmd(10,10);
    if(ret)  
    {
        DEBUG_SERIAL.println("Sending SYNC command to ESP32...OK");
        delay(1000);
        ret = espDetect();
        if(ret == false)
        {
            espFlashLog.close();
            return 2;
        }
        ret = espGetFeature();
        if(ret == false)
        {
            espFlashLog.close();
            return 2;
        }
        ret = espGetMacId();
        if(ret == false)
        {
            espFlashLog.close();
            return 2;
        }
        ret = espUploadStub();
        if(ret == false)
        {
            espFlashLog.close();
            return 2;
        }
        ret = espConfigureFlash();
        if(ret == false)
        {
            espFlashLog.close();
            return 2;
        }
        espFlashLog.close();
        delay(100);
        espFlashLog = DOSFS.open("esp32Response.log", "a");
        ret = espBinWrite("iuTempFirmware","wifi_ESP32.ino.bin");
        delay(100);
        if(ret == false)
        {
            espFlashLog.close();
            return 2;
        }
        espFlashLog = DOSFS.open("esp32Response.log", "a");
        delay(100);
        //String fwhash = espGetMD5Hash();
        //DEBUG_SERIAL.println(fwhash);
        ret = espExitFlash();
        if(ret == false)
        {
            espFlashLog.close();
            return 2;
        }
    }
    else
    {
        DEBUG_SERIAL.println("SYNC failed, aborting ESP32 FW download !!!");
    }  
    espFlashLog.close();
    // File log = DOSFS.open("esp32Response.log", "r");
    // while (log.available())
    // {
    //   DEBUG_SERIAL.write(log.read());
    // }
    // log.close();
    DOSFS.end();
}


bool espComm::espBinWrite(char *folderName,char *fileName)
{    
    uint32_t readIdx = 0;
    uint32_t fileSize = 0;
    uint16_t pktSeqNo = 0;
    bool ret = false;
    if(folderName == NULL || fileName == NULL)
    {
        DEBUG_SERIAL.println(F("fw bin file read param error !"));
        return false;
    }
    char filepath[40];
    snprintf(filepath, 40, "%s/%s", folderName, fileName);
    DEBUG_SERIAL.println("File path:" + String(filepath));
    File fwFile;    
    if(DOSFS.exists(filepath))
    {
        fwFile = DOSFS.open(filepath,"r");
        if(fwFile)
        {
            unsigned char fileBuf[MAX_FILE_RW_SIZE];
            fileSize = fwFile.size();
            DEBUG_SERIAL.print(F("File Size:"));
            DEBUG_SERIAL.println(fileSize);
            readIdx = 0;
            if(fileSize > 0)
            {
                FlashAdd[9] = (uint8_t)(fileSize & 0x000000FF);
                FlashAdd[10] = (uint8_t)((fileSize & 0x0000FF00) >> 8);
                FlashAdd[11] = (uint8_t)((fileSize & 0x00FF0000) >> 16);
                FlashAdd[12] = (uint8_t)((fileSize & 0xFF000000) >> 24);

                md5Hash[13] = (uint8_t)(fileSize & 0x000000FF);
                md5Hash[14] = (uint8_t)((fileSize & 0x0000FF00) >> 8);
                md5Hash[15] = (uint8_t)((fileSize & 0x00FF0000) >> 16);
                md5Hash[16] = (uint8_t)((fileSize & 0xFF000000) >> 24);
              // Set File size in Flash configure- Address set command 
                ret = espSendCmd(FlashAdd, sizeof(FlashAdd), 1);
                if(ret == false) {
                    fwFile.close();
                    return false;
                }
                delay(100);
                do
                {                    
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
                       // DEBUG_SERIAL.print(F("File Read Size:"));
                       // DEBUG_SERIAL.println(readIdx);
                        // for(int i = 0; i < FLASH_BLOCK_SIZE; i++)
                        //     DEBUG_SERIAL.print(readBuf[i]);  
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
                      //  DEBUG_SERIAL.print(F("File Read Size:"));
                       // DEBUG_SERIAL.println(readIdx);
                        // for(int i = 0; i < FLASH_BLOCK_SIZE; i++)
                        //     DEBUG_SERIAL.print(readBuf[i]);
                        fileSize = 0;
                        //DEBUG_SERIAL.println("Preparing packet for writing to ESP32..");
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
                DEBUG_SERIAL.print(F("File size error, size = 0"));
                DEBUG_SERIAL.println(filepath);
                fwFile.close();
                return false;
            }
        }
        else
        {
            DEBUG_SERIAL.print(F("Error opening FW download file:"));
            DEBUG_SERIAL.println(filepath);
            return false;
        }
    }
    else
    {
        DEBUG_SERIAL.print(F("FW download file doesn't exists !"));
        DEBUG_SERIAL.println(filepath);
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
    DEBUG_SERIAL.print("Packet CRC: 0x");
    DEBUG_SERIAL.println(pktCrc, HEX);
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
    
    DEBUG_SERIAL.println("Packet Seq. No: " + String(pktSeqNo));
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
  //  DEBUG_SERIAL.println("Modified packet size:" + String(pktSize1));
    writeIdx = writeIdx + pktSize1;
    PktBuf[writeIdx++] = 0xc0;
   // DEBUG_SERIAL.println("Total packet size:" + String(writeIdx));
    // if(pktSeqNo > 150) {
    //  for(int i = 0; i < writeIdx; i++) {
    //      DEBUG_SERIAL.print(PktBuf[i],HEX);
    //      DEBUG_SERIAL.print(" ");
    //  }
    // }
   ret = espSendDataPkt(PktBuf, writeIdx, 1,pktSeqNo);
   delay(100);
   if(ret == false)
    return false;
   else
       return true;
}

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
#if 0
bool espComm::espEraseFlash()
{
  espSendCmd(eraseFlash, sizeof(eraseFlash), 1);
  delay(100);
}
#endif
bool espComm::espDetect()
{
    bool ret = false;
    DEBUG_SERIAL.println("Sending ESP32 Detect command...");
    ret = espSendCmd(deviceType, sizeof(deviceType), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(chipId, sizeof(chipId), 2);  
    delay(100);
    if(ret == false)
        return false;
}

bool espComm::espGetFeature()
{
    bool ret = false;
    DEBUG_SERIAL.println("Sending ESP32 GetFeature command...");
    ret = espSendCmd(readReg1, sizeof(readReg1), 1);
    delay(100);
    if(ret == false)
        return false;

    ret = espSendCmd(readReg2, sizeof(readReg2), 1);
    delay(100);
    if(ret == false)
        return false;
}

bool espComm::espGetMacId()
{
    bool ret = false;
    DEBUG_SERIAL.println("Sending ESP32 Get MAC command...");
    ret = espSendCmd(MacIDreg1, sizeof(MacIDreg1), 1);
    delay(100);
    if(ret == false)
        return false;

    ret = espSendCmd(MacIDreg2, sizeof(MacIDreg2), 1);
    delay(100);
    if(ret == false)
        return false;
}

bool espComm::espUploadStub()
{
    bool ret = false;
    DEBUG_SERIAL.println("Sending ESP32 Upload Stub command...");
    ret = espSendCmd(uploadStub, sizeof(uploadStub), 1);
    delay(100);
    if(ret == false)
        return false;

    ret = espSendCmd(uploadStub0, sizeof(uploadStub0), 1);
    delay(100);
    if(ret == false)
        return false;

    ret = espSendCmd(uploadStub1, sizeof(uploadStub1), 1);
    delay(100);
    if(ret == false)
        return false;

    ret = espSendCmd(uploadStub2, sizeof(uploadStub2), 1);
    delay(100);
    if(ret == false)
        return false;

    ret = espSendCmd(uploadStub3, sizeof(uploadStub3), 1,4);
    delay(100);
    if(ret == false)
        return false;
}

bool espComm::espConfigureFlash()
{
    bool ret = false;
    DEBUG_SERIAL.println("Sending ESP32 Configure Flash command...");
    ret = espSendCmd(configFlash1, sizeof(configFlash1), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash2, sizeof(configFlash2), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash3, sizeof(configFlash3), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash4, sizeof(configFlash4), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash5, sizeof(configFlash5), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash6, sizeof(configFlash6), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash7, sizeof(configFlash7), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash8, sizeof(configFlash8), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash9, sizeof(configFlash9), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash10, sizeof(configFlash10), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash11, sizeof(configFlash11), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(configFlash12, sizeof(configFlash12), 1);
    delay(100);
    if(ret == false)
        return false;
    // ret = espSendCmd(FlashAdd, sizeof(FlashAdd), 1);
    // delay(100);
    // if(ret == false)
    //     return false;
}

bool espComm::espExitFlash()
{
    bool ret = false;
    ret = espSendCmd(FlashLeave1, sizeof(FlashLeave1), 1);
    delay(100);
    if(ret == false)
        return false;
    ret = espSendCmd(FlashLeave2, sizeof(FlashLeave2), 1);
    delay(100);
    if(ret == false)
        return false;
}

unsigned int espComm::update_C0_DB(unsigned char *destPkt,unsigned char *srcBuf, unsigned int size)
{
    unsigned int out_pos = 0;
    for (int i = 0; i < size; ++i)
    {
        unsigned char cur = srcBuf[i];
        if (cur == 0xC0)
        {
           // DEBUG_SERIAL.println("Found 0xC0");
            destPkt[out_pos++] = subst_C0[0];
            destPkt[out_pos++] = subst_C0[1];
        }
        else if (cur == 0xDB)
        {
           // DEBUG_SERIAL.println("Found 0xDB");
            destPkt[out_pos++] = subst_DB[0];
            destPkt[out_pos++] = subst_DB[1];
        }
        else
            destPkt[out_pos++] = cur;
    }
    return out_pos;
}


void espComm::espCleanup()
{
  while (Serial1.available() > 0)
  {
    Serial1.read();
  }
}

bool espComm:: esp_SendSyncCmd(uint8_t rebootCount, uint8_t retrySync)
{
    DEBUG_SERIAL.println("Sending SYNC command...");
	int cnt = 112;
	DEBUG_SERIAL.println(sizeof(syncCommand));
	int index = 0;
	uint8_t m_rebootCount = rebootCount;
	uint8_t m_retrySync = retrySync;
	byte expectedResponse[] = {0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20, 0x01, 0x07, 0x12, 0x20};
	byte responseCmd[32];
	while (m_retrySync >= 0 && m_rebootCount > 0)
	{
		if (m_retrySync > 0)
		{
			--m_retrySync;
			espFlashLog.print("Sync Command : ");
			for (int i = 0; i < sizeof(syncCommand); i++)
			{
				Serial1.write(syncCommand[i]);
				espFlashLog.print(syncCommand[i], HEX);
				espFlashLog.print(" ");
			}
			espFlashLog.println();
			espFlashLog.print("Sync Response : ");
			delay(600);
			index = 0;
            while (Serial1.available() > 0)
            {
                byte x = Serial1.read();
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
                    DEBUG_SERIAL.println("Found C0");
                    espFlashLog.println();
                    if (compareResponse(expectedResponse, responseCmd, sizeof(expectedResponse), sizeof(responseCmd)) == true)
                    {
                        DEBUG_SERIAL.println("SYNC Response Matched");
                        return true;
                    }
                    else
                    {
                        DEBUG_SERIAL.println("SYNC Response Mismatch");
                        espReboot();
                        m_retrySync = retrySync;
                        cnt = 112;
                        m_rebootCount--;
                        Serial1.flush();
                        for (int i = 0; i < sizeof(responseCmd); i++)
                        {
                            responseCmd[i] = 0x00;
                        }
                    }
                }
            }
			delay(400);
			espFlashLog.println();
		}
		else
		{
			m_retrySync = retrySync;
		}
	}
	return false;
}

/*
  Get the command response  from ESP32
*/
bool espComm::espSendCmd(byte command[], int size, int retry,uint8_t countC0)
{
	int cnt = countC0;
//	DEBUG_SERIAL.println(size);
	espFlashLog.print("Command  : ");
	for (int i = 0; i < size; i++)
	{
//    DEBUG_SERIAL.print(command[i],HEX);
//    DEBUG_SERIAL.print(" ");
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
			Serial1.write(command[i]);
		}
        if(command[2] == 0xD0) //  for erase command
        {
            DEBUG_SERIAL.println("Erasing Flash");
            delay(3000);
        }
        else
		    delay(300);
		while (Serial1.available() > 0)
		{
			byte a = Serial1.read();
			//DEBUG_SERIAL.write(a);
			espFlashLog.print(a, HEX);
			espFlashLog.print(" ");
			if (a == 0xc0)
			{
				--cnt;
				if (cnt <= 0)
				{
					//DEBUG_SERIAL.println("Found C0");
					espFlashLog.println();
					Serial1.flush();
					return true;
				}
			}
		}
		espFlashLog.println();
		DEBUG_SERIAL.println();
		delay(100);
	}
	return false;
}

String espComm:: espGetMD5Hash()
{
	int cnt = 28;
	char hash[16];
	char ReceivedMd5Hash[32];
	int index = 0;
//	Serial1.flush();
  delay(2000);
	DEBUG_SERIAL.println(sizeof(md5Hash));
	espFlashLog.print("MD5 Command  : ");
	for (int i = 0; i < sizeof(md5Hash); i++)
	{
		espFlashLog.print(md5Hash[i], HEX);
		espFlashLog.print(" ");
	}
	for (int i = 0; i < sizeof(md5Hash); i++)
	{
		Serial1.write(md5Hash[i]);
	}
	espFlashLog.println();
	espFlashLog.print("MD5 Response : ");
  DEBUG_SERIAL.println("\nMD5 Response : ");
	delay(1500);
	while (Serial1.available() > 0)
	{
		byte a = Serial1.read();
    // espFlashLog.print(a, HEX);
    // espFlashLog.print(" ");
		if (cnt < 20 && cnt > 2)
		{
			hash[index] = a;
			index++;
		}
		--cnt;
		if (cnt <= 0)
		{
			Serial1.flush();
			for (int i = 0; i < sizeof(hash); i++)
			{
				char tempHash[4];
				sprintf(tempHash, "%02x", hash[i]);
				strncat(ReceivedMd5Hash, tempHash, 2);
			}
			return (String)ReceivedMd5Hash;
		}
	}
}

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
void espComm:: espReboot()
{
	DEBUG_SERIAL.println("Rebooting esp32");
	digitalWrite(ESP32_IO0, HIGH);
	delay(500);
	digitalWrite(ESP32_IO0, LOW);		 // IDE1.5_PORT_CHANGE
	digitalWrite(ESP32_ENABLE_PIN, LOW); // IDE1.5_PORT_CHANGE
	delay(100);
	digitalWrite(ESP32_ENABLE_PIN, HIGH); // IDE1.5_PORT_CHANGE
	delay(100);
	// Serial1.flush();
	espCleanup();
	DEBUG_SERIAL.println("Download Mode");
}

bool espComm::espSendDataPkt(unsigned char *command, uint32_t size, int retry, uint16_t seq)
{
    int cnt = 12;
    int index = 0;
    byte expectedResponse[] = {0x01, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    byte responseCmd[10];
    DEBUG_SERIAL.println(size);
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
        Serial1.println();
        for (int i = 0; i < size; i++)
        {
            Serial1.write(command[i]);
        }
        // Serial1.write(0x0a);
        delay(100);
        while (Serial1.available() > 0)
        {
            byte a = Serial1.read();
            // DEBUG_SERIAL.print(a,HEX);
            //  DEBUG_SERIAL.print(" ");
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
             //   DEBUG_SERIAL.println("Found C0");
               // espFlashLog.println();
                if (compareResponse(expectedResponse, responseCmd, sizeof(expectedResponse), sizeof(responseCmd)) == true)
                {
                    DEBUG_SERIAL.println("Response Matched");
                    return true;
                }
                else
                {
                    DEBUG_SERIAL.println("Response Missmatch");
                    Serial1.flush();
                    return false;
                }
            }
        }
      //  espFlashLog.println();
        DEBUG_SERIAL.println(".");
        delay(100);
    }
    return false;
}