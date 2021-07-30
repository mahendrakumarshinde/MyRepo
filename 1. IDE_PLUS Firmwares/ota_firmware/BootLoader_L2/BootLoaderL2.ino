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


#define BL1_ADDRESS                   (uint32_t)0x08000000    /* Start address of Bootloader 1 */
#define BL2_ADDRESS                   (uint32_t)0x08010000    /* Start address of Bootloader 2 */
#define FFW_ADDRESS                   (uint32_t)0x08036000    /* Start address of Facotry Firmware */
#define MFW_ADDRESS                   (uint32_t)0x08060000    /* Start address of Main Firmware */
#define FLAG_ADDRESS                  (uint32_t)0x080FF800    /* Start address of FLAG location*/
#define CONFIG_MQTT_FLASH_ADDRESS     (uint32_t)0x080FE800    /* Start address of MQTT Config location*/
#define CONFIG_HTTP_FLASH_ADDRESS     (uint32_t)0x080FE000    /* Start address of HTTP Config location*/

#define CONFIG_MODBUS_SLAVE_CONFIG_FLASH_ADDRESS    (uint32_t)0x80FD800    /* Start address of MODBUS slave Config location*/
#define CONFIG_WIFI_CONFIG_FLASH_ADDRESS            (uint32_t)0x80FD000    /* Start address of MODBUS slave Config location*/
#define CONFIG_DEV_INFO_ADDRESS                     (uint32_t)0x080FC800   /* Start address Device Information location */
// #ifdef DEVIDFIX_TESTSTUB
// #define CONFIG_DEV_ID_ADDRESS                       (uint32_t)0x080FC000
// #endif

//#define CONFIG_DEV_ID_ADDRESS         (uint32_t)0x080FC000    /* Start address BMD MAC location */
//#define CONFIG_DEV_INFO_ADDRESS       (uint32_t)0x080FC800    /* Start address Device Information location */

espComm esp32;

char calculatedMD5Sum[64], receivedMD5Sum[64], verifiedMD5Sum[64];

char stmcalcMD5Sum[64];
char espcalcMD5Sum[64];

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

  ESP_SERIAL.begin(115200);
#if DEBUG_ENABLE == 1
	DEBUG_SERIAL.begin(115200);
#endif

	bl2DebugPrint("=================================");
	bl2DebugPrint("     IU Bootloader-2             ");
	  bl2DebugPrint("Bootloader Version :",false);
	bl2DebugPrint(BOOTLOADER2_VERSION,false);
  bl2DebugPrint("        ");
	bl2DebugPrint("=================================");

  delay(1000);
	if (!DOSFS.begin()) {
		bl2DebugPrint("Memory access failed, or not present");
    // don't do anything more:
	}
//	bl2DebugPrint("Memory initialized.");
  ledManager.stopColorOverride();
   
  ledManager.showStatus(&STATUS_OTA_UPGRADE);
  delay(100);
  iu_read_all_flags();
  bl2DebugPrint("Flag Main_FLAG_0 : ",false);bl2DebugPrint(iu_all_flags[MFW_FLASH_FLAG*8]);
  bl2DebugPrint("Flag RTRY_FLAG_1 : ",false);bl2DebugPrint(iu_all_flags[RETRY_FLAG*8]);
  bl2DebugPrint("Flag VALIDTION_2 : ",false);bl2DebugPrint(iu_all_flags[RETRY_VALIDATION*8]);
  bl2DebugPrint("Flag OTAPEND_STS_3 : ",false);bl2DebugPrint(iu_all_flags[MFW_VER*8]);
  bl2DebugPrint("Flag SELF_UPGRDE_4 : ",false);bl2DebugPrint(iu_all_flags[SELF_UPGRD_STATUS_MSG_LOC*8]);
  delay(100);
}


void loop()
{
  iu_read_all_flags();
  //iu_all_flags[MFW_FLASH_FLAG] = 1; /* Forcing the Flag, for testing */

  File MD5_value;
  uint16_t retVal3;

  switch (iu_all_flags[MFW_FLASH_FLAG]) {
       
    case OTA_FW_SUCCESS:  /* 0 -> Normal boot*/
            break;
    case OTA_FW_DOWNLOAD_SUCCESS:  /* 1 -> Flash STM Main Firmware */
            bl2DebugPrint("Upgrading vEdge Main,WiFi Firmware..");
            if((DOSFS.exists(STM_MAIN_FIRMWARE)) && (DOSFS.exists(STM_MFW_1_SUM)) &&
               (DOSFS.exists(ESP_MAIN_FIRMWARE1)) && (DOSFS.exists(ESP_MFW_1_SUM))) {

              if(checkMainFWMD5Hash(STM_MAIN_FIRMWARE,STM_MFW_1_SUM) == false)
              {                
                bl2DebugPrint("Error : Main FW File check failed !!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
                break;
              }
              if(checkWifiFWMD5Hash(ESP_MAIN_FIRMWARE1,ESP_MFW_1_SUM) == false)
              {
                bl2DebugPrint("Error : WiFi FW File check failed !!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
                break;
              }

              retVal3 = esp32.flash_esp32_verify(ESP_MAIN_FIRMWARE,ESP_FIRMWARE_FILENAME);
              delay(2000);
              if(retVal3 == RETURN_SUCESS)
              {
                bl2DebugPrint("WiFi FW Flashing Successful.");
                retVal3 = Flash_Verify_STM_File(STM_MAIN_FIRMWARE,STM_MFW_1_SUM);
                delay(2000);
                if(retVal3 == RETURN_SUCESS)
                  bl2DebugPrint("Main FW Flashing Successful."); 
                else
                  bl2DebugPrint("Main FW Flashing Failed !!");                
              }
              else
              {
                bl2DebugPrint("WiFi FW Flashing Failed !!");
              }
              
              if(retVal3 == RETURN_SUCESS) 
              {
                update_flag(MFW_FLASH_FLAG,OTA_FW_UPGRADE_SUCCESS); 
              } else if (retVal3 == 2) {
                bl2DebugPrint("Error : File Open Failed try again!!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
              } else {
                bl2DebugPrint("Error : Flashing Verification failed..");
                update_flag(MFW_FLASH_FLAG,OTA_FW_UPGRADE_FAILED); // Verification failed error
              }
            } else {
              bl2DebugPrint("Error : File(s) Missing..");
              update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_SYS_ERROR);
            } 
            delay(100);
            ledManager.overrideColor(RGB_BLACK);
            break;
    case OTA_FW_UPGRADE_FAILED:  /* 2 -> No action here */
             /* Using Factory Firmware */
            bl2DebugPrint("Upgrading vEdge with Factory Firmware..");
            break;
    case OTA_FW_UPGRADE_SUCCESS:  /* 3 -> No Action here...*/
            break;
    case OTA_FW_INTERNAL_ROLLBACK: /* 4-> Rollback STM MFW*/
            /* Swap STM-MFW1.bin and STM-MFW2.bin
            * need to check the file excist or not 
            */
            bl2DebugPrint("Internal Rollback vEdge Main,WiFi Firmware..");
            
            if((DOSFS.exists(STM_ROLLBACK_FIRMWARE)) && (DOSFS.exists(STM_MFW_2_SUM)) &&
               (DOSFS.exists(ESP_ROLLBACK_FIRMWARE1)) && (DOSFS.exists(ESP_MFW_2_SUM)) &&
               (DOSFS.exists(ESP_RLBK_BOOT_APP_BIN)) && (DOSFS.exists(ESP_RLBK_BOOT_APP_SUM)) &&
               (DOSFS.exists(ESP_RLBK_BOOT_LODR_BIN)) && (DOSFS.exists(ESP_RLBK_BOOT_LODR_SUM)) &&
               (DOSFS.exists(ESP_RLBK_PART_BIN)) && (DOSFS.exists(ESP_RLBK_PART_SUM)) ) {

              if(checkMainFWMD5Hash(STM_ROLLBACK_FIRMWARE,STM_MFW_2_SUM) == false)
              {                
                bl2DebugPrint("Error : Main FW File check failed !!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
                break;
              }
              if(checkWifiFWMD5Hash(ESP_ROLLBACK_FIRMWARE1,ESP_MFW_2_SUM) == false)
              {
                bl2DebugPrint("Error : WiFi FW File check failed !!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
                break;
              }
              if(checkWifiFWMD5Hash(ESP_RLBK_BOOT_APP_BIN,ESP_RLBK_BOOT_APP_SUM) == false)
              {
                bl2DebugPrint("Error : WiFi Boot FW File check failed !!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
                break;
              }
              if(checkWifiFWMD5Hash(ESP_RLBK_BOOT_LODR_BIN,ESP_RLBK_BOOT_LODR_SUM) == false)
              {
                bl2DebugPrint("Error : WiFi Boot loader FW File check failed !!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
                break;
              }
              if(checkWifiFWMD5Hash(ESP_RLBK_PART_BIN,ESP_RLBK_PART_SUM) == false)
              {
                bl2DebugPrint("Error : WiFi partition FW File check failed !!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
                break;
              }
              retVal3 = esp32.flash_esp32_verify(ESP_ROLLBACK_FIRMWARE,ESP_FIRMWARE_FILENAME);
              delay(2000);
              if(retVal3 == RETURN_SUCESS)
              {
                bl2DebugPrint("Rollback of WiFi FW Flashing Successful.");
                retVal3 = Flash_Verify_STM_File(STM_ROLLBACK_FIRMWARE,STM_MFW_2_SUM);
                delay(2000);
                if(retVal3 == RETURN_SUCESS)
                  bl2DebugPrint("Rollback of Main FW Flashing Successful."); 
                else
                  bl2DebugPrint("Rollback of Main FW Flashing Failed !!");                
              }
              else
              {
                bl2DebugPrint("Rollback of WiFi FW Flashing Failed !!");
              }
              //delay(2000);
//              bl2DebugPrint("Setting MFW Flag");
              if(retVal3 == RETURN_SUCESS){
                update_flag(MFW_FLASH_FLAG,OTA_FW_SUCCESS);
                // while reading, flag loc x 8 is performed, because all flags are stored at 8 byte interval in memory
                if(iu_all_flags[SELF_UPGRD_STATUS_MSG_LOC*8] == OTA_FW_DOWNLOAD_SUCCESS) {
                  bl2DebugPrint("Self upgrade success - Internal Rollback ");  
                  update_flag(SELF_UPGRD_STATUS_MSG_LOC,SELF_FW_UPGRADE_SUCCESS); //SELF_FW_UPGRADE_SUCCESS
                }
              } else if (retVal3 == 2) {
                bl2DebugPrint("Error : File Checksum mismatch! Download file(s) again!!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
              } else {
                bl2DebugPrint("Error : Flash Verification failed ..");
                update_flag(MFW_FLASH_FLAG,OTA_FW_INTERNAL_ROLLBACK); // Verification failed error
              }
            } else {
              update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_SYS_ERROR);
              bl2DebugPrint("Error : File(s) Missing");
            }
            delay(100);  
            break;
    case OTA_FW_FORCED_ROLLBACK: // Rollback Main FW using Backup FW
            /* Swap STM-MFW1.bin and STM-MFW2.bin
            * need to check the file excist or not 
            */
            bl2DebugPrint("Forced Rollback vEdge (Backup) Firmware..");

            if((DOSFS.exists(STM_FORCED_ROLLBACK_FIRMWARE)) && (DOSFS.exists(STM_MFW_3_SUM)) &&
               (DOSFS.exists(ESP_FORCED_ROLLBACK_FIRMWARE1)) && (DOSFS.exists(ESP_MFW_3_SUM)) &&
               (DOSFS.exists(ESP_BKUP_BOOT_APP_BIN)) && (DOSFS.exists(ESP_BKUP_BOOT_APP_SUM)) &&
               (DOSFS.exists(ESP_BKUP_BOOT_LODR_BIN)) && (DOSFS.exists(ESP_BKUP_BOOT_LODR_SUM)) &&
               (DOSFS.exists(ESP_BKUP_PART_BIN)) && (DOSFS.exists(ESP_BKUP_PART_SUM))) {

              if(checkMainFWMD5Hash(STM_FORCED_ROLLBACK_FIRMWARE,STM_MFW_3_SUM) == false)
              {                
                bl2DebugPrint("Error : Main FW File check failed !!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
                break;
              }
              if(checkWifiFWMD5Hash(ESP_FORCED_ROLLBACK_FIRMWARE1,ESP_MFW_3_SUM) == false)
              {
                bl2DebugPrint("Error : WiFi FW File check failed !!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
                break;
              }
              if(checkWifiFWMD5Hash(ESP_BKUP_BOOT_APP_BIN,ESP_BKUP_BOOT_APP_SUM) == false)
              {
                bl2DebugPrint("Error : WiFi Boot FW File check failed !!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
                break;
              }
              if(checkWifiFWMD5Hash(ESP_BKUP_BOOT_LODR_BIN,ESP_BKUP_BOOT_LODR_SUM) == false)
              {
                bl2DebugPrint("Error : WiFi Boot loader FW File check failed !!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
                break;
              }
              if(checkWifiFWMD5Hash(ESP_BKUP_PART_BIN,ESP_BKUP_PART_SUM) == false)
              {
                bl2DebugPrint("Error : WiFi partition FW File check failed !!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
                break;
              }
              retVal3 = esp32.flash_esp32_verify(ESP_FORCED_ROLLBACK_FIRMWARE,ESP_FIRMWARE_FILENAME);
              delay(2000);
              if(retVal3 == RETURN_SUCESS)
              {
                bl2DebugPrint("Forced Rollback of WiFi FW Flashing Successfull.");
                retVal3 = Flash_Verify_STM_File(STM_FORCED_ROLLBACK_FIRMWARE,STM_MFW_3_SUM);
                delay(2000);
                if(retVal3 == RETURN_SUCESS)
                  bl2DebugPrint("Forced Rollback of Main FW Flashing Successfull."); 
                else
                  bl2DebugPrint("Forced Rollback of Main FW Flashing Failed !!");                
              }
              else
              {
                bl2DebugPrint("Forced Rollback of WiFi FW Flashing Failed !!");
              }
              delay(2000);
//              bl2DebugPrint("Setting MFW Flag");
              if(retVal3 == RETURN_SUCESS){
                update_flag(MFW_FLASH_FLAG,OTA_FW_SUCCESS);
                // while reading, flag loc x 8 is performed, because all flags are stored at 8 byte interval in memory
                if(iu_all_flags[SELF_UPGRD_STATUS_MSG_LOC*8] == OTA_FW_DOWNLOAD_SUCCESS) {
                  bl2DebugPrint("Self upgrade success - backupfolder ");  
                  update_flag(SELF_UPGRD_STATUS_MSG_LOC,SELF_FW_UPGRADE_SUCCESS); //SELF_FW_UPGRADE_SUCCESS
                }
              } else if (retVal3 == 2) {
                bl2DebugPrint("Error : File Checksum mismatch! Download file(s) again!!");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_CHKSUM_ERROR); // File error
              } else {
                bl2DebugPrint("Error : Flash verification failed..");
                update_flag(MFW_FLASH_FLAG,OTA_FW_FORCED_ROLLBACK); // Verification failed error
              }
            } else {
              update_flag(MFW_FLASH_FLAG,OTA_FW_FILE_SYS_ERROR);
              bl2DebugPrint("Error : File(s) Missing");
            }
            delay(100);
            break;            
    case OTA_FW_FILE_CHKSUM_ERROR: /* File read error, No action here */
            break;
    case OTA_FW_FILE_SYS_ERROR: /* File(s) Missing, No action here */
            break;            
    default:
            delay(1);
  }

  delay(1000);
  bl2DebugPrint("Rebooting Device....");
  DOSFS.end();
  delay(1000);
  stm32l4_system_reset(); // Call reset function. 
  delay(1000);
  bl2DebugPrint("Reset failed...");
  
	while (1);
}

/* This function is used to check MD5 of WiFi FW binary stored in external flash and 
 * MD5 hash received during OTA req. (stored in .MD5 file along with FW binary file) 
 */
bool checkWifiFWMD5Hash(char *ESP_FW_FILE,char *ESP_MD5_FILE)
{
    uint8_t md5Status;

    md5Status = MD5_sum_file(ESP_FW_FILE, calculatedMD5Sum);
    if (md5Status > 0)
    {
      bl2DebugPrint("WiFi FW MD5 Check Fail: ");
      return false;
    }    
    bl2DebugPrint("WiFi FW MD5 Calculated value: ",false);
    bl2DebugPrint(calculatedMD5Sum);
    //bl2DebugPrint ("Length = "); bl2DebugPrint(strlen(calculatedMD5Sum));

    Read_MD5(ESP_MD5_FILE, receivedMD5Sum);
    bl2DebugPrint("WiFi FW MD5 Read value: ",false);
    bl2DebugPrint(receivedMD5Sum);
    //bl2DebugPrint ("Length = "); bl2DebugPrint(strlen(receivedMD5Sum));
    if (strcmp(receivedMD5Sum, calculatedMD5Sum) == 0) 
    {
      bl2DebugPrint("WiFi FW File MD5 Hash Ok");
      strncpy(espcalcMD5Sum,calculatedMD5Sum,64);
      return true;
    }
    else
    {
      bl2DebugPrint("WiFi FW File MD5 Hash Failed !");
      return false;
    }
}

/* This function is used to check MD5 of Main FW binary stored in external flash and 
 * MD5 hash received during OTA req. (stored in .MD5 file along with FW binary file) 
 */
bool checkMainFWMD5Hash(char *STM_FW_FILE,char *STM_MD5_FILE)
{
    uint8_t md5Status;

    md5Status = MD5_sum_file(STM_FW_FILE, calculatedMD5Sum);
    if (md5Status > 0)
    {
      return false;
    }
    
    bl2DebugPrint("Main FW MD5 Calculated value: ",false);
    bl2DebugPrint(calculatedMD5Sum);
    //bl2DebugPrint ("Length = "); bl2DebugPrint(strlen(calculatedMD5Sum));

    Read_MD5(STM_MD5_FILE, receivedMD5Sum);
    bl2DebugPrint("Main FW MD5 Read value: ",false);
    bl2DebugPrint(receivedMD5Sum);
    //bl2DebugPrint ("Length = "); bl2DebugPrint(strlen(receivedMD5Sum));
    if (strcmp(receivedMD5Sum, calculatedMD5Sum) == 0) 
    {
      bl2DebugPrint("Main FW File MD5 Hash Ok");
      strncpy(stmcalcMD5Sum,calculatedMD5Sum,64);
      return true;
    }
    else
    {
      bl2DebugPrint("Main FW File MD5 Hash Failed !");
      return false;
    }
}

/*----------------------------------------------------------------------------------------------------*/

/* This function is used to open file from external flash memory
 *  
 */
int iu_flash_open_file(File *fp, char * fileName, const char * fileMode, size_t * fileSize)
{
	*fp = DOSFS.open(fileName, fileMode);
	if (!*fp) {
		bl2DebugPrint("Error opening file...");
		return(F_ERR_NOTFOUND);
	}
	*fileSize = fp->size();
	return(F_NO_ERROR);
}

/* This function is used to read file from external flash memory in chunks
 *  
 */
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
  //bl2DebugPrint("Current position is="); bl2DebugPrint(currPos);
	fp->seek(newPos-currPos,SeekSet);
  //bl2DebugPrint("Set file position to ="); bl2DebugPrint(newPos);
	bytesRead = fp->read(buffer, chunkSize);
	if (bytesRead < chunkSize) {
		//bl2DebugPrint("Full chunk not read from file.");
	}
	return(bytesRead);
}

/* This function is used to append file stored on external flash memory in chunks
 *  
 */
size_t iu_flash_append_to_file(File *fp, uint8_t *buffer, uint8_t chunkSize, size_t buffSize)
{
	size_t bytesWritten=0;

	if (!fp) {
		return(-1);			//	As we need to return the size of data written
	}
	fp->seek(0, SeekEnd);
	bytesWritten = fp->write(buffer, buffSize);
	if (bytesWritten < chunkSize) {
		bl2DebugPrint("Full buffer not written to file.");
	}
	return(bytesWritten);
}

/* This function is used to close file stored on external flash memory
 *  
 */
int iu_flash_close_file(File *fp)
{
	if (!fp) {
		return(F_ERR_NOTFOUND);
	}
	fp->close();
	return(F_NO_ERROR);
}
/*--------------------------------------------------------------------------------------------------------*/ 

/* This function is used to  write Main FW binary into internal STM flash memory
 *  
 */
/* Write File to Internal Flash */ 
uint16_t Flash_STM_File(char* readFileName) 
{
  //bl2DebugPrint("dosFileSys: Writing to Memory");
  File dataFileFP;
  size_t dataFileSize, bytesRead;
  uint8_t dataBuffer[TEST_CHUNK_SIZE];
  bool falshFailed = false;
  
  if (iu_flash_open_file(&dataFileFP, readFileName, "r", &dataFileSize)
      != F_NO_ERROR) {
    bl2DebugPrint("Error Opening file.");
    return FILE_OPEN_FAILED;
    
  }
  bl2DebugPrint("File Size is =",false); bl2DebugPrint(dataFileSize);
  if(!stm32l4_flash_unlock() ){
    //  bl2DebugPrint("FLASH STM32_FLASH_UNLOCK_FAILED !!!");
      //return STM32_FLASH_UNLOCK_FAILED; 
  }
  if(!stm32l4_flash_erase((uint32_t)MFW_ADDRESS,((dataFileSize/2048)+1)*2048) ){
    bl2DebugPrint("STM32_FLASH_ERASE_FAILED !!!");
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
      bl2DebugPrint("Main FW Sector Write: ",false); 
      bl2DebugPrint(i,false);bl2DebugPrint("/",false);bl2DebugPrint(sectorCnt);
      //bl2DebugPrint((char*)dataBuffer);
      if(!stm32l4_flash_program((uint32_t)(MFW_ADDRESS + ((i-1)*TEST_CHUNK_SIZE)), dataBuffer, bytesRead) ){
          // Retry could be required ?
          return STM32_FLASH_WRITE_ERROR;
      }
      delay(10);
    }
  }

  iu_flash_close_file(&dataFileFP);
  stm32l4_flash_lock(); 
  bl2DebugPrint("Flashing completed..."); 
  return RETURN_SUCESS;
}


/* This function is used to  Verify checksum, and write Main FW binary 
 * into internal STM flash memory
 */
uint16_t Flash_Verify_STM_File(char* INPUT_BIN_FILE,char* INPUT_MD5_FILE) 
{
  bl2DebugPrint("Main FW File ok..");
  uint16_t retval4 = Flash_STM_File(INPUT_BIN_FILE); // Flashing the code to Internal memory
  delay(1000);
  if(RETURN_SUCESS != retval4) {
    //DO Retry
      return RETURN_FAILED;
  }

  Flash_MD5_Sum(INPUT_BIN_FILE, verifiedMD5Sum);
  if (strcmp(verifiedMD5Sum,stmcalcMD5Sum) == 0) {
    //bl2DebugPrint("STM Firmware flashed successfully..!");
    return RETURN_SUCESS;
  } else {
    bl2DebugPrint("Error : Internal memory checksum mismatch..");
    return 1;
  }  
}


/* This function is used to update status flag values stored in STM internal flash @FLAG_ADDRESS
 */
void update_flag(uint8_t flag_addr , uint8_t flag_data)
{
    bl2DebugPrint("Updating flag:",false);
    bl2DebugPrint(flag_addr,false);
    bl2DebugPrint(" Val:",false);
    bl2DebugPrint(flag_data); 
 
    uint8_t flag_addr_temp = flag_addr * 8;
    iu_read_all_flags();
    iu_all_flags[flag_addr_temp] = flag_data;
    
    stm32l4_flash_unlock();
    stm32l4_flash_erase((uint32_t)FLAG_ADDRESS, 2048);
    delay(1000);
    // bl2DebugPrint("Flash Erased...");
    stm32l4_flash_program((uint32_t)FLAG_ADDRESS, iu_all_flags, 128);
    delay(1000);
    stm32l4_flash_lock();
    bl2DebugPrint("Flag updated...");
}

/* This function is used to Calculate MD5SUM for a file in External Flash
 */
uint8_t MD5_sum_file(char* readFileName, char *md5Result)
{
  File dataFileFP;
  size_t dataFileSize, bytesRead;
  uint8_t dataBuffer[TEST_CHUNK_SIZE];
  bool retVal;

  if (iu_flash_open_file(&dataFileFP, readFileName, "r", &dataFileSize)
      != F_NO_ERROR) {
    bl2DebugPrint("Error Opening file.");
    return  FILE_OPEN_FAILED;
  }
  bl2DebugPrint("File Size is =",false); bl2DebugPrint(dataFileSize);

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
   
/* This function is used to Read MD5SUM from .md5 file
 */
void Read_MD5(char* TEST_FILE, char *md5Result)
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

/* This function is used to Calculate MD5SUm for Internal Flash
 */
uint8_t Flash_MD5_Sum(char* readFileName, char *md5Result)
{
//  bl2DebugPrint("dosFileSys: Writing to Memory");
  File dataFileFP;
  size_t dataFileSize, bytesRead;
  uint8_t dataBuffer[TEST_CHUNK_SIZE];

  if (iu_flash_open_file(&dataFileFP, readFileName, "r", &dataFileSize)
      != F_NO_ERROR) {
    bl2DebugPrint("Error Opening file.");
    return FILE_OPEN_FAILED;
  }
//  iu_flash_close_file(&dataFileFP);
  bl2DebugPrint("File Size is =",false); bl2DebugPrint(dataFileSize);
  delay(1000);

  uint8_t *bytes = (uint8_t *)MFW_ADDRESS;

  myMD5inst.init_hash(&iuCtx, (char *)bytes, TEST_CHUNK_SIZE);
  myMD5inst.make_chunk_hash(&iuCtx, (char *)(bytes + TEST_CHUNK_SIZE), (dataFileSize-TEST_CHUNK_SIZE));
  //bl2DebugPrint("MD5 checksum make final digest...");
  hashVal = myMD5inst.make_final_hash(&iuCtx);

  char *md5str = myMD5inst.make_digest(hashVal,16);

  memset(md5Result, 0x00, 64);
  strcpy(md5Result, md5str);
  bl2DebugPrint("MD5 sum of Flash : ",false); bl2DebugPrint(md5str);
  iu_flash_close_file(&dataFileFP);
//  Need to free the md5str that was allocated by make_digest()
  free(md5str);
}

/* This function is used to read status flag values stored in STM internal flash @FLAG_ADDRESS
 */
void iu_read_all_flags(void)
{
  for(int i = 0 ; i <128 ; i++) {
    iu_all_flags[i] = 0;
  }
  for (int i = 0 ; i < 128; i= i+8){
    iu_all_flags[i] = *(uint8_t*)(FLAG_ADDRESS + i);
    //bl2DebugPrint(iu_all_flags[i],HEX);
  }    
}

#if 0
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
    bl2DebugPrint("Error : File(s) Missing..");
  }
}
#endif
/*--------------------------------*/
