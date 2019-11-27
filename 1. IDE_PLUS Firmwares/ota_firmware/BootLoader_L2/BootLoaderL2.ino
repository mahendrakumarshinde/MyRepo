#include "FS.h"
#include <Arduino.h>
#include "stm32l4_flash.h"
#include "stm32l4_wiring_private.h"
#include "espComm.h"
#include "RGBLed.h"
#include "LedManager.h"

#include "iuMD5.h"
#include "bootloaderCodes.h"

APA102RGBLedStrip rgbLedStrip(16);
GPIORGBLed rgbLed(25, 26, 38);

LedManager ledManager(&rgbLed,&rgbLedStrip);


/* Serial - for USB-UART, Serial1 - for UART3, Serail4 - for UART5 */
#define DEBUG_SERIAL Serial4 

#define BL1_ADDRESS     (uint32_t)0x08000000    /* Start address of Bootloader 1 */
#define BL2_ADDRESS     (uint32_t)0x08010000    /* Start address of Bootloader 2 */
#define FFW_ADDRESS     (uint32_t)0x08036000    /* Start address of Facotry Firmware */
#define MFW_ADDRESS     (uint32_t)0x08060000    /* Start address of Main Firmware */
#define FLAG_ADDRESS    (uint32_t)0x080FF800    /* Start address of FLAG location*/


espComm esp32;

char calculatedMD5Sum[64], receivedMD5Sum[64], verifiedMD5Sum[64];
iuMD5 myMD5inst;
MD5_CTX iuCtx;
unsigned char *hashVal=NULL;

uint8_t iu_all_flags[128];

void iu_read_all_flags(void);


/*--------------------------------------------------------------------------------------------------------*/
void setup()
{
  delay(5000);
  pinMode(6,OUTPUT);
  digitalWrite(6,HIGH);
  rgbLed.setup();
  ledManager.setBaselineStatus(&STATUS_NO_STATUS);
  ledManager.showStatus(&STATUS_NO_STATUS);
  ledManager.stopColorOverride();

  Serial1.begin(115200);
	DEBUG_SERIAL.begin(115200);

	DEBUG_SERIAL.print("Initializing External Flash Memory...");
	DOSFS.format();
  delay(1000);
	if (!DOSFS.begin()) {
		DEBUG_SERIAL.println("Memory failed, or not present");
    // don't do anything more:
	}
	DEBUG_SERIAL.println("Memory initialized.");
  ledManager.stopColorOverride();
   
  ledManager.showStatus(&STATUS_OTA_UPGRADE);
  delay(2000);
  
}


void loop()
{
  iu_read_all_flags();
  //iu_all_flags[MFW_FLASH_FLAG] = 1; /* Forcing the Flag, for testing */

  File MD5_value;
  uint16_t retVal3;

  switch (iu_all_flags[MFW_FLASH_FLAG]) {
       
    case 0:  /* 0 -> Normal boot*/
            break;
    case 1:  /* 1 -> Flash STM Main Firmware */
            DEBUG_SERIAL.println("Upgrading STM Main Firmware..");
            if((DOSFS.exists(STM_MAIN_FIRMWARE)) && (DOSFS.exists(STM_MFW_1_SUM))) {
              retVal3 = esp32.flash_esp32_verify(ESP_MAIN_FIRMWARE,ESP_FIRMWARE_FILENAME);
              delay(2000);
              if(retVal3 == 0)
              {
                DEBUG_SERIAL.println("ESP32 FW Flashing Successfull.");
                retVal3 = Flash_Verify_STM_File(STM_MAIN_FIRMWARE,STM_MFW_1_SUM);
                delay(2000);
                if(retVal3 == 0)
                  DEBUG_SERIAL.println("STM32 FW Flashing Successfull."); 
                else
                  DEBUG_SERIAL.println("STM32 FW Flashing Failed !!");                
              }
              else
              {
                DEBUG_SERIAL.println("ESP32 FW Flashing Failed !!");
              }
              
              if(retVal3 == 0) 
              {
                update_flag(0,3); 
              } else if (retVal3 == 2) {
                DEBUG_SERIAL.println("Error : File Open Failed try again!!");
                update_flag(0,6); // File error
              } else {
                DEBUG_SERIAL.println("Error : Flashing Verification failed..");
                update_flag(0,2); // Verification failed error
              }
            } else {
              DEBUG_SERIAL.println("Error : File(s) Missing..");
              update_flag(0,7);
            } 
            delay(100);
            ledManager.overrideColor(RGB_BLACK);
            break;
    case 2:  /* 2 -> No action here */
             /* Using Factory Firmware */
             DEBUG_SERIAL.println("Upgrading STM with Factory Firmware..");
            break;
    case 3:  /* 3 -> No Action here...*/
            break;
    case 4: /* 4-> Rollback STM MFW*/
            /* Swap STM-MFW1.bin and STM-MFW2.bin
            * need to check the file excist or not 
            */
            DEBUG_SERIAL.println("Rollback STM Main Firmware..");
            
            if((DOSFS.exists(STM_ROLLBACK_FIRMWARE)) && (DOSFS.exists(STM_MFW_2_SUM))) {
              
              DEBUG_SERIAL.println("Flashing STM (RB) Firmware..");
              retVal3 = esp32.flash_esp32_verify(ESP_ROLLBACK_FIRMWARE,ESP_FIRMWARE_FILENAME);
              delay(2000);
              if(retVal3 == 0)
              {
                DEBUG_SERIAL.println("Rollback of ESP32 FW Flashing Successfull.");
                retVal3 = Flash_Verify_STM_File(STM_ROLLBACK_FIRMWARE,STM_MFW_2_SUM);
                delay(2000);
                if(retVal3 == 0)
                  DEBUG_SERIAL.println("Rollback of STM32 FW Flashing Successfull."); 
                else
                  DEBUG_SERIAL.println("Rollback of STM32 FW Flashing Failed !!");                
              }
              else
              {
                DEBUG_SERIAL.println("Rollback of ESP32 FW Flashing Failed !!");
              }
              //delay(2000);
              DEBUG_SERIAL.println("Setting MFW Flag");
              if(retVal3 == 0){
                update_flag(0,3); 
              } else if (retVal3 == 2) {
                DEBUG_SERIAL.println("Error : File Checksum mismatch! Download file(s) again!!");
                update_flag(0,6); // File error
              } else {
                DEBUG_SERIAL.println("Error : Flash Verification failed ..");
                update_flag(0,4); // Verification failed error
              }
            } else {
              update_flag(0,7);
              DEBUG_SERIAL.println("Error : File(s) Missing");
            }
            delay(100);  
            break;
    case 5: // Rollback Main FW using Backup FW
            /* Swap STM-MFW1.bin and STM-MFW2.bin
            * need to check the file excist or not 
            */
            if((DOSFS.exists(STM_FORCED_ROLLBACK_FIRMWARE)) && (DOSFS.exists(STM_MFW_3_SUM))) {
              DEBUG_SERIAL.println("Forced Rollback STM (Backup) Firmware..");
              
              DEBUG_SERIAL.println("Flashing STM (Backup) Firmware..");
              retVal3 = esp32.flash_esp32_verify(ESP_FORCED_ROLLBACK_FIRMWARE,ESP_FIRMWARE_FILENAME);
              delay(2000);
              if(retVal3 == 0)
              {
                DEBUG_SERIAL.println("Forced Rollback of ESP32 FW Flashing Successfull.");
                retVal3 = Flash_Verify_STM_File(STM_FORCED_ROLLBACK_FIRMWARE,STM_MFW_3_SUM);
                delay(2000);
                if(retVal3 == 0)
                  DEBUG_SERIAL.println("Forced Rollback of STM32 FW Flashing Successfull."); 
                else
                  DEBUG_SERIAL.println("Forced Rollback of STM32 FW Flashing Failed !!");                
              }
              else
              {
                DEBUG_SERIAL.println("Forced Rollback of ESP32 FW Flashing Failed !!");
              }
              delay(2000);
              DEBUG_SERIAL.println("Setting MFW Flag");
              if(retVal3 == 0){
                update_flag(0,2); 
              } else if (retVal3 == 2) {
                DEBUG_SERIAL.println("Error : File Checksum mismatch! Download file(s) again!!");
                update_flag(0,6); // File error
              } else {
                DEBUG_SERIAL.println("Error : Flash verification failed..");
                update_flag(0,5); // Verification failed error
              }
            } else {
              update_flag(0,7);
              DEBUG_SERIAL.println("Error : File(s) Missing");
            }
            delay(100);
            break;            
    case 6: /* File read error, No action here */
            break;
    case 7: /* File(s) Missing, No action here */
            break;            
    default:
            delay(1);
  }

  delay(1000);
  DEBUG_SERIAL.println("Rebooting IDE....");
  DOSFS.end();
  delay(1000);
  stm32l4_system_reset(); // Call reset function. 
  delay(1000);
  DEBUG_SERIAL.println("Reset failed...");
  
	while (1);
}

/*----------------------------------------------------------------------------------------------------*/

int iu_flash_open_file(File *fp, char * fileName, const char * fileMode, size_t * fileSize)
{
	*fp = DOSFS.open(fileName, fileMode);
	if (!*fp) {
		DEBUG_SERIAL.println("Error opening file...");
		return(F_ERR_NOTFOUND);
	}
	*fileSize = fp->size();
	return(F_NO_ERROR);
}

size_t iu_flash_read_file_chunk(File *fp, uint8_t *buffer, size_t chunkSize, int chunkSeq)
{
	size_t fileSize=0, bytesRead=0;
  size_t newPos=0, currPos;

	if (!fp) {
		return(-1);			//	As we need to return the size of the read
	}
	fileSize = fp->size();
	if (chunkSeq <= 0)
		return(-2);			// chunkSeq starts from 1...N

	if ((chunkSize*(chunkSeq-1)) > fileSize) {
		return(0);
	}
  newPos = chunkSize * (chunkSeq - 1);
  currPos = fp->position();
  //DEBUG_SERIAL.print("Current position is="); DEBUG_SERIAL.println(currPos);
	fp->seek(newPos-currPos,SeekSet);
  //DEBUG_SERIAL.print("Set file position to ="); DEBUG_SERIAL.println(newPos);
	bytesRead = fp->read(buffer, chunkSize);
	if (bytesRead < chunkSize) {
		//DEBUG_SERIAL.println("Full chunk not read from file.");
	}
	return(bytesRead);
}

size_t iu_flash_append_to_file(File *fp, uint8_t *buffer, uint8_t chunkSize, size_t buffSize)
{
	size_t bytesWritten=0;

	if (!fp) {
		return(-1);			//	As we need to return the size of data written
	}
	fp->seek(0, SeekEnd);
	bytesWritten = fp->write(buffer, buffSize);
	if (bytesWritten < chunkSize) {
		DEBUG_SERIAL.println("Full buffer not written to file.");
	}
	return(bytesWritten);
}

int iu_flash_close_file(File *fp)
{
	if (!fp) {
		return(F_ERR_NOTFOUND);
	}
	fp->close();
	return(F_NO_ERROR);
}
/*--------------------------------------------------------------------------------------------------------*/ 

/* Write File to Internal Flash */ 
uint16_t Flash_STM_File(char* readFileName) 
{
  DEBUG_SERIAL.println("dosFileSys: Writing to Memory");
  File dataFileFP;
  size_t dataFileSize, bytesRead;
  uint8_t dataBuffer[TEST_CHUNK_SIZE];
  bool falshFailed = false;
  
  if (iu_flash_open_file(&dataFileFP, readFileName, "r", &dataFileSize)
      != F_NO_ERROR) {
    DEBUG_SERIAL.println("Error Opening file.");
    return FILE_OPEN_FAILED;
    
  }
  DEBUG_SERIAL.print("File Size is ="); DEBUG_SERIAL.println(dataFileSize);
  if(!stm32l4_flash_unlock() ){
      DEBUG_SERIAL.println("FLASH STM32_FLASH_UNLOCK_FAILED !!!");
      //return STM32_FLASH_UNLOCK_FAILED; 
  }
  if(!stm32l4_flash_erase((uint32_t)MFW_ADDRESS,((dataFileSize/2048)+1)*2048) ){
    DEBUG_SERIAL.println("STM32_FLASH_ERASE_FAILED !!!");
    return STM32_FLASH_ERASE_FAILED;
  }
  delay(3000);

  //unsigned char received[dataFileSize];
  uint32_t sectorCnt = 0;
  if(dataFileSize > TEST_CHUNK_SIZE) {
    sectorCnt = (dataFileSize/TEST_CHUNK_SIZE);
    if(dataFileSize%TEST_CHUNK_SIZE)
      sectorCnt = sectorCnt + 1;
  }
  else if(dataFileSize == TEST_CHUNK_SIZE)
  {
    sectorCnt = 1;
  }     
  //for (int i=1; i <= (dataFileSize/TEST_CHUNK_SIZE)+1; i++) {
    for (int i=1; i <= sectorCnt; i++) {
    ledManager.updateColors();
    if ((bytesRead=iu_flash_read_file_chunk(&dataFileFP, dataBuffer, TEST_CHUNK_SIZE, i)) > 0) {
      DEBUG_SERIAL.print("Sector Write: "); 
      DEBUG_SERIAL.print(i);DEBUG_SERIAL.print("/");DEBUG_SERIAL.println(sectorCnt);
      //DEBUG_SERIAL.println((char*)dataBuffer);
      if(!stm32l4_flash_program((uint32_t)(MFW_ADDRESS + ((i-1)*TEST_CHUNK_SIZE)), dataBuffer, bytesRead) ){
          // Retry could be required ?
          return STM32_FLASH_WRITE_ERROR;
      }
      delay(10);
    }
  }

  iu_flash_close_file(&dataFileFP);
  stm32l4_flash_lock(); 
  DEBUG_SERIAL.println("Flashing completed..."); 
  return RETURN_SUCESS;
}


/* Verify, Write and Verify */
uint16_t Flash_Verify_STM_File(char* INPUT_BIN_FILE,char* INPUT_MD5_FILE) 
{ 
  uint8_t md5Status;

  md5Status = MD5_sum_file(INPUT_BIN_FILE, calculatedMD5Sum);
  if (md5Status > 0)
  {
    return md5Status;
  }
  
  DEBUG_SERIAL.print("MD5 Calculated value: ");
  DEBUG_SERIAL.println(calculatedMD5Sum);
  DEBUG_SERIAL.print ("Length = "); DEBUG_SERIAL.println(strlen(calculatedMD5Sum));

  Read_MD5(INPUT_MD5_FILE, receivedMD5Sum);
  DEBUG_SERIAL.print("MD5 Read value: ");
  DEBUG_SERIAL.println(receivedMD5Sum);
  DEBUG_SERIAL.print ("Length = "); DEBUG_SERIAL.println(strlen(receivedMD5Sum));

  if (strcmp(receivedMD5Sum, calculatedMD5Sum) == 0) {
    DEBUG_SERIAL.println("STM MFW File ok..");
    uint16_t retval4 = Flash_STM_File(INPUT_BIN_FILE); // Flashing the code to Internal memory
    delay(1000);
    if(RETURN_SUCESS != retval4) {
      //DO Retry
       return RETURN_FAILED;
    }

    Flash_MD5_Sum(INPUT_BIN_FILE, verifiedMD5Sum);
    if (strcmp(verifiedMD5Sum,calculatedMD5Sum) == 0) {
      DEBUG_SERIAL.println("STM Firmware flashed successfully..!");
      return RETURN_SUCESS;
    } else {
      DEBUG_SERIAL.println("Error : Internal memory checksum mismatch..");
      return 1;
    }           
  }else {
    DEBUG_SERIAL.println("Error in STM MFW / MD5 File(s)");
    return 2;
  }
  
}

  
void update_flag(uint8_t flag_addr , uint8_t flag_data)
  {
    DEBUG_SERIAL.println("Updating flag..."); 
 
    uint8_t flag_addr_temp = flag_addr * 8;
    iu_read_all_flags();
    iu_all_flags[flag_addr_temp] = flag_data;
    
    stm32l4_flash_unlock();
    stm32l4_flash_erase((uint32_t)FLAG_ADDRESS, 2048);
    delay(1000);
    // DEBUG_SERIAL.println("Flash Erased...");
    stm32l4_flash_program((uint32_t)FLAG_ADDRESS, iu_all_flags, 128);
    delay(1000);
    stm32l4_flash_lock();
    DEBUG_SERIAL.println("Flag updated...");
    }


/* Calculate MD5SUM for a file in External Flash */
uint8_t MD5_sum_file(char* readFileName, char *md5Result)
{
  File dataFileFP;
  size_t dataFileSize, bytesRead;
  uint8_t dataBuffer[TEST_CHUNK_SIZE];
  bool retVal;

  if (iu_flash_open_file(&dataFileFP, readFileName, "r", &dataFileSize)
      != F_NO_ERROR) {
    DEBUG_SERIAL.println("Error Opening file.");
    return  FILE_OPEN_FAILED;
  }
  DEBUG_SERIAL.print("File Size is ="); DEBUG_SERIAL.println(dataFileSize);

  iuMD5 myMD5inst;
  MD5_CTX iuCtx;
  unsigned char *hashVal=NULL;
    
  for (int i=1; i <= (dataFileSize/TEST_CHUNK_SIZE)+1; i++) {
    if ((bytesRead=iu_flash_read_file_chunk(&dataFileFP, dataBuffer, TEST_CHUNK_SIZE, i)) > 0) {
      if (i == 1){
        myMD5inst.init_hash(&iuCtx, (char *)dataBuffer, bytesRead);
      } else {
        myMD5inst.make_chunk_hash(&iuCtx, (char *)dataBuffer, bytesRead);   
      }
    }
  }
  
  hashVal = myMD5inst.make_final_hash(&iuCtx);
  char *md5str = myMD5inst.make_digest(hashVal,16);
  memset(md5Result, 0x00, 64);
  strcpy(md5Result, md5str);
  free(md5str);
  iu_flash_close_file(&dataFileFP);
  
  return RETURN_SUCESS;
}
   

/* Read MD5SUM from .md5 file */
void Read_MD5(char* TEST_FILE, char *md5Result)
{
  if (DOSFS.exists(TEST_FILE)) {
    File MD5_value = DOSFS.open(TEST_FILE,"r");

    if (MD5_value) {
      memset(md5Result, 0x00, 64);
      int readLen=MD5_value.readBytesUntil('\0', md5Result, 64);
      DEBUG_SERIAL.print("Received MD5 value: ");DEBUG_SERIAL.println(md5Result);
      byte lastChar = strlen(md5Result)-1;
      md5Result[lastChar] = '\0'; 
      MD5_value.close(); 
    }
  }
}

/* Calculate MD5SUm for Internal Flash */
uint8_t Flash_MD5_Sum(char* readFileName, char *md5Result)
{
//  DEBUG_SERIAL.println("dosFileSys: Writing to Memory");
  File dataFileFP;
  size_t dataFileSize, bytesRead;
  uint8_t dataBuffer[TEST_CHUNK_SIZE];

  if (iu_flash_open_file(&dataFileFP, readFileName, "r", &dataFileSize)
      != F_NO_ERROR) {
    DEBUG_SERIAL.println("Error Opening file.");
    return FILE_OPEN_FAILED;
  }
//  iu_flash_close_file(&dataFileFP);
  DEBUG_SERIAL.print("File Size is ="); DEBUG_SERIAL.println(dataFileSize);
  delay(1000);

  uint8_t *bytes = (uint8_t *)MFW_ADDRESS;

  myMD5inst.init_hash(&iuCtx, (char *)bytes, TEST_CHUNK_SIZE);
  myMD5inst.make_chunk_hash(&iuCtx, (char *)(bytes + TEST_CHUNK_SIZE), (dataFileSize-TEST_CHUNK_SIZE));
  //DEBUG_SERIAL.println("MD5 checksum make final digest...");
  hashVal = myMD5inst.make_final_hash(&iuCtx);

  char *md5str = myMD5inst.make_digest(hashVal,16);

  memset(md5Result, 0x00, 64);
  strcpy(md5Result, md5str);
  DEBUG_SERIAL.print("MD5 sum of Flash : "); DEBUG_SERIAL.println(md5str);
  iu_flash_close_file(&dataFileFP);
//  Need to free the md5str that was allocated by make_digest()
  free(md5str);
}

void iu_read_all_flags(void)
{
  for(int i = 0 ; i <128 ; i++) {
    iu_all_flags[i] = 0;
  }
  for (int i = 0 ; i < 128; i= i+8){
    iu_all_flags[i] = *(uint8_t*)(FLAG_ADDRESS + i);
    //DEBUG_SERIAL.println(iu_all_flags[i],HEX);
  }    
}

void swap_files(char* INPUT_FILE_1, char* INPUT_FILE_2)
{
  if((DOSFS.exists(INPUT_FILE_1)) && (DOSFS.exists(INPUT_FILE_2))) {
    DOSFS.rename(INPUT_FILE_1, "TEMP");
    delay(1000);
    DOSFS.rename(INPUT_FILE_2, INPUT_FILE_1);
    delay(1000);
    DOSFS.rename("TEMP", INPUT_FILE_2);
    delay(1000);
  } else {
    DEBUG_SERIAL.println("Error : File(s) Missing..");
  }
}

/*--------------------------------*/
