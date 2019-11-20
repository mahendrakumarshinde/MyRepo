#include "FS.h"
#include <Arduino.h>
#include "stm32l4_flash.h"
#include "stm32l4_wiring_private.h"


#include "iuMD5.h"

/* Serial - for USB-UART, Serial1 - for UART3, Serail4 - for UART5 */
#define DEBUG_SERIAL Serial4  

#define BL1_ADDRESS     (uint32_t)0x08000000    /* Start address of Bootloader 1 */
#define BL2_ADDRESS     (uint32_t)0x08010000    /* Start address of Bootloader 2 */
#define FFW_ADDRESS     (uint32_t)0x08036000    /* Start address of Facotry Firmware */
#define MFW_ADDRESS     (uint32_t)0x08060000    /* Start address of Main Firmware */
#define FLAG_ADDRESS    (uint32_t)0x080FF800    /* Start address of FLAG location*/


#define MFW_FLASH_FLAG 0
#define RETRY_FLAG 1
#define FACTORY_FW 2
#define MFW_VER 3
#define FW_VALIDATION 4
#define FW_ROLLBACK 5
#define STABLE_FW 6


#define ESP_FW_VER 7
#define ESP_FW_UPGRAD 8
#define ESP_RUNNING_VER 9
#define ESP_ROLLBACK 10


#define TEST_CHUNK_SIZE	256
#define STM_MFW_1 "iuTempFirmware/STM-MFW.bin"
#define STM_MFW_2 "iuRollbackFirmware/STM-MFW.bin"
#define STM_MFW_3 "iuBackupFirmware/STM-MFW.bin"
#define STM_MFW_1_SUM "iuTempFirmware/STM-MFW.md5"
#define STM_MFW_2_SUM "iuRollbackFirmware/STM-MFW.md5"
#define STM_MFW_3_SUM "iuBackupFirmware/STM-MFW.md5"
#define ESP_MFW_1 "ESP-MFW1.bin"
#define ESP_MFW_2 "ESP-MFW2.bin"
//#define TEST_READ_FILE STM_MFW_1


String calculated, received_md5;
iuMD5 myMD5inst;
MD5_CTX iuCtx;
unsigned char *hashVal=NULL;

uint8_t iu_all_flags[128];

void iu_read_all_flags(void);


void setup()
{
  delay(5000);
  pinMode(6,OUTPUT);
  digitalWrite(6,HIGH);
  
  //while (!DEBUG_SERIAL)
  //yield();
	DEBUG_SERIAL.begin(115200);
	DEBUG_SERIAL.print("Initializing External Flash Memory...");
	DOSFS.format();
  delay(1000);
	if (!DOSFS.begin()) {
		DEBUG_SERIAL.println("Memory failed, or not present");
    // don't do anything more:
    //while (1);
	}
	DEBUG_SERIAL.println("Memory initialized.");
}


void loop()
{
  // iu_all_flags[MFW_FLASH_FLAG] = 0;
  iu_read_all_flags();
  //iu_all_flags[MFW_FLASH_FLAG] = 1; /* Forcing the Flag, for testing */

  //DEBUG_SERIAL.println(switch_input1);
  File MD5_value;
  uint16_t retVal3;

  switch (iu_all_flags[MFW_FLASH_FLAG]) {
       
    case 0:  /* 0 -> Normal boot*/
            break;
    case 1:  /* 1 -> Flash STM Main Firmware */
            DEBUG_SERIAL.println("Upgrading STM Main Firmware..");
            retVal3 = Flash_Verify_STM_File(STM_MFW_1);
            //Flash_STM_File(STM_MFW_1);
            /*Verify_STM_FW();
            * If Error, iu_all_flags[MFW_FLASH_FLAG] = 2; else iu_all_flags[MFW_FLASH_FLAG] =3;
            */
            delay(2000);
            DEBUG_SERIAL.println("Setting MFW Flag");
            if(retVal3 == 0){
              update_flag(0,3); 
            } else if (retVal3 == 2) {
              update_flag(0,5); // File error
            } else {
              update_flag(0,2); // Verification failed error
            }
            delay(100);
            break;
    case 2:  /* 2 -> No action here */
            break;
    case 3:  /* 3 -> No Action here...*/
            break;
    case 4: /* 4-> Rollback STM MFW*/
            /* Swap STM-MFW1.bin and STM-MFW2.bin
            * need to check the file excist or not 
            */
            DEBUG_SERIAL.println("Roll back STM Main Firmware..");
            
            if((DOSFS.exists(STM_MFW_1)) && (DOSFS.exists(STM_MFW_2)) && (DOSFS.exists(STM_MFW_1_SUM)) && (DOSFS.exists(STM_MFW_2_SUM))) {
              DOSFS.rename(STM_MFW_1, "STM-TEMP.bin");
              delay(1000);
              DOSFS.rename(STM_MFW_2, STM_MFW_1);
              delay(1000);
              DOSFS.rename("STM-TEMP.bin", STM_MFW_2);
              delay(1000);
              DOSFS.rename(STM_MFW_1_SUM, "STM-TEMP.md5");
              delay(1000);
              DOSFS.rename(STM_MFW_2_SUM, STM_MFW_1_SUM);
              delay(1000);
              DOSFS.rename("STM-TEMP.md5", STM_MFW_2_SUM);
              delay(1000);
              
              
              DEBUG_SERIAL.println("Flashing STM Main Firmware..");
              retVal3 = Flash_Verify_STM_File(STM_MFW_1);
              //Flash_STM_File(STM_MFW_1);
              /*Verify STM FW;
              * If Error, iu_all_flags[MFW_FLASH_FLAG] = 2; else iu_all_flags[MFW_FLASH_FLAG] =3;
              */
              delay(2000);
              DEBUG_SERIAL.println("Setting MFW Flag");
              if(retVal3 == 0){
                update_flag(0,3); 
              } else if (retVal3 == 2) {
                update_flag(0,6); // File error
              } else {
                update_flag(0,2); // Verification failed error
              }
              delay(100);              
            } else {
              DEBUG_SERIAL.println("File(s) Missing");
            }
  
            break;
    case 5: // Rollback Main FW using Backup FW
            /* Swap STM-MFW1.bin and STM-MFW2.bin
            * need to check the file excist or not 
            */
            if((DOSFS.exists(STM_MFW_1)) && (DOSFS.exists(STM_MFW_3)) && (DOSFS.exists(STM_MFW_1_SUM)) && (DOSFS.exists(STM_MFW_3_SUM))) {
              DEBUG_SERIAL.println("Roll back STM Main Firmware..");
              DOSFS.rename(STM_MFW_3, "TEMP");
              delay(1000);
              DOSFS.rename(STM_MFW_1, STM_MFW_3);
              delay(1000);
              DOSFS.rename("TEMP", STM_MFW_1);
              delay(1000);
              DOSFS.rename(STM_MFW_3_SUM, "TEMP");
              delay(1000);
              DOSFS.rename(STM_MFW_1_SUM, STM_MFW_3_SUM);
              delay(1000);
              DOSFS.rename("TEMP", STM_MFW_1_SUM);
              delay(1000);
              //DOSFS.rename("STM-TEMP.bin", STM_MFW_2);
              //delay(1000);
              
              DEBUG_SERIAL.println("Flashing STM Main Firmware..");
              retVal3 = Flash_Verify_STM_File(STM_MFW_1);
              //Flash_STM_File(STM_MFW_1);
              /*Verify STM FW;
              * If Error, iu_all_flags[MFW_FLASH_FLAG] = 2; else iu_all_flags[MFW_FLASH_FLAG] =3;
              */
              delay(2000);
              DEBUG_SERIAL.println("Setting MFW Flag");
              if(retVal3 == 0){
                update_flag(0,3); 
              } else if (retVal3 == 2) {
                update_flag(0,6); // File error
              } else {
                update_flag(0,2); // Verification failed error
              }
            } else {
              DEBUG_SERIAL.println("File(s) Missing");
            }
            delay(100);            
    case 6: /* File read error, No action here */
            break;
            
      default:
            delay(1);
    }

    delay(1000);
    DEBUG_SERIAL.println("Rebooting IDE....");
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

 
uint16_t Flash_STM_File(char* TEST_READ_FILE) /* Write File to Internal Flash */
{
  DEBUG_SERIAL.println("dosFileSys: Writing to Memory");
  File dataFileFP;
  size_t dataFileSize, bytesRead;
  uint8_t dataBuffer[TEST_CHUNK_SIZE];

  if (iu_flash_open_file(&dataFileFP, TEST_READ_FILE, "r", &dataFileSize)
    != F_NO_ERROR) {
    DEBUG_SERIAL.println("Error Opening file.");
  }
  DEBUG_SERIAL.print("File Size is ="); DEBUG_SERIAL.println(dataFileSize);
  stm32l4_flash_unlock();
  stm32l4_flash_erase((uint32_t)MFW_ADDRESS,((dataFileSize/2048)+1)*2048);
  delay(1000);

   unsigned char received[dataFileSize];
   size_t charCount=0;
   
//  for (int i=1; i <= (dataFileSize/TEST_CHUNK_SIZE+1); i++) {
  for (int i=1; i <= (dataFileSize/TEST_CHUNK_SIZE)+1; i++) {
    if ((bytesRead=iu_flash_read_file_chunk(&dataFileFP, dataBuffer, TEST_CHUNK_SIZE, i)) > 0) {
      DEBUG_SERIAL.print("Chunk Seq="); DEBUG_SERIAL.println(i);
      DEBUG_SERIAL.print("Bytes Read="); DEBUG_SERIAL.println(bytesRead);
      //DEBUG_SERIAL.println((char*)dataBuffer);
      stm32l4_flash_program((uint32_t)(MFW_ADDRESS + ((i-1)*TEST_CHUNK_SIZE)), dataBuffer, bytesRead);
      delay(10);
    }
  }

  
  iu_flash_close_file(&dataFileFP);
  //update_flag(0,3);
  stm32l4_flash_lock();
  DEBUG_SERIAL.println("Flashing completed..."); 
  return 1;
  
}

uint16_t Flash_Verify_STM_File(char* INPUT_FILE) /* Verify, Write and Verify */
{
  String md5_val = MD5_sum_file(INPUT_FILE);
  DEBUG_SERIAL.print("MD5 Calculated value: ");
  DEBUG_SERIAL.println(md5_val);
  DEBUG_SERIAL.print ("Length = "); DEBUG_SERIAL.println(md5_val.length());
  calculated = md5_val;

  String md5_read = Read_MD5(STM_MFW_1_SUM);
  DEBUG_SERIAL.print("MD5 Read value: ");
  DEBUG_SERIAL.println(md5_read);
  DEBUG_SERIAL.print ("Length = "); DEBUG_SERIAL.println(md5_read.length());
  received_md5 = md5_read;
  
  if(received_md5 == calculated){
    DEBUG_SERIAL.println("STM MFW File ok..");
    uint16_t retval4 = Flash_STM_File(INPUT_FILE); // Flashing the code to Internal memory
    delay(1000);
    String verify_md5 = Flash_MD5_Sum(INPUT_FILE);
    if(verify_md5 == calculated) {
      DEBUG_SERIAL.println("STM MFW Flashed..!");
      return 0;
    } else {
      return 1;
    }           
  }else {
    DEBUG_SERIAL.println("Error in STM MFW File");
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
    //DEBUG_SERIAL.write(rv);
    DEBUG_SERIAL.println("Flash Erased...");
    stm32l4_flash_program((uint32_t)FLAG_ADDRESS, iu_all_flags, 128);
    delay(1000);
    stm32l4_flash_lock();
    DEBUG_SERIAL.println("Flag updated...");
    }

String MD5_sum_file(char* TEST_READ_FILE) /* Calculate MD5SUM for a file in External Flash */
{
  File dataFileFP;
  size_t dataFileSize, bytesRead;
  uint8_t dataBuffer[TEST_CHUNK_SIZE];
  bool retVal;

  if (iu_flash_open_file(&dataFileFP, TEST_READ_FILE, "r", &dataFileSize)
    != F_NO_ERROR) {
    DEBUG_SERIAL.println("Error Opening file.");
  }
  DEBUG_SERIAL.print("File Size is ="); DEBUG_SERIAL.println(dataFileSize);

  iuMD5 myMD5inst;
  MD5_CTX iuCtx;
  unsigned char *hashVal=NULL;
    
  for (int i=1; i <= (dataFileSize/TEST_CHUNK_SIZE)+1; i++) {
    if ((bytesRead=iu_flash_read_file_chunk(&dataFileFP, dataBuffer, TEST_CHUNK_SIZE, i)) > 0) {
      if (i == 1){
        myMD5inst.init_hash(&iuCtx, (char *)dataBuffer, bytesRead);
      }
      else{
        myMD5inst.make_chunk_hash(&iuCtx, (char *)dataBuffer, bytesRead);   
      }
    }
  }
  
  hashVal = myMD5inst.make_final_hash(&iuCtx);
  char *md5str = myMD5inst.make_digest(hashVal,16);
  String str1 = String(md5str); 
  iu_flash_close_file(&dataFileFP);
  return str1;        
}
   

String Read_MD5(char* TEST_FILE) /* Read MD5SUM from .md5 file */
{
  if(DOSFS.exists(TEST_FILE)) {
    File MD5_value = DOSFS.open(TEST_FILE,"r");
    while(MD5_value)
      {        
        //fileSize = MD5_value.size();
        //DEBUG_SERIAL.println("File Size:" + String(fileSize));
        String rd = MD5_value.readStringUntil('\0');
        DEBUG_SERIAL.print("Received MD5 value: ");DEBUG_SERIAL.println(rd);
        rd.remove(rd.length()-1);
        received_md5 = rd;
          MD5_value.close(); 
    }       
  }
  return received_md5;
}

String Flash_MD5_Sum(char* TEST_READ_FILE)  /* Calculate MD5SUm for Internal Flash */
{
  //DEBUG_SERIAL.println("dosFileSys: Writing to Memory");
  File dataFileFP;
  size_t dataFileSize, bytesRead;
  uint8_t dataBuffer[TEST_CHUNK_SIZE];

  if (iu_flash_open_file(&dataFileFP, TEST_READ_FILE, "r", &dataFileSize)
    != F_NO_ERROR) {
    DEBUG_SERIAL.println("Error Opening file.");
  }
  DEBUG_SERIAL.print("File Size is ="); DEBUG_SERIAL.println(dataFileSize);
  delay(1000);
  uint8_t *bytes = (uint8_t *)MFW_ADDRESS;
  myMD5inst.init_hash(&iuCtx, (char *)bytes, TEST_CHUNK_SIZE);
  myMD5inst.make_chunk_hash(&iuCtx, (char *)(bytes + TEST_CHUNK_SIZE), (dataFileSize-TEST_CHUNK_SIZE));
  DEBUG_SERIAL.println("MD5 checksum make final digest...");
  hashVal = myMD5inst.make_final_hash(&iuCtx);
  char *md5str = myMD5inst.make_digest(hashVal,16);
  String MD5_final = String(md5str);  
  return MD5_final;
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

/*--------------------------------*/
