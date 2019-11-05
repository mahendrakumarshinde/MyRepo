#include "FS.h"
#include <Arduino.h>
#include "stm32l4_flash.h"
#include "stm32l4_wiring_private.h"

#ifndef CHECK_SUM
#define CHECK_SUM 1
#endif

#ifdef CHECK_SUM
#include "MD5.h"
MD5 hashMD5;
#endif

#define DEBUG_SERIAL Serial1

#define FFW_ADDRESS     (uint32_t)0x08036000    /* Start address of Facotry Firmware */
#define MFW_ADDRESS     (uint32_t)0x08060000    /* Start address of Main Firmware */
#define FLAG_ADDRESS    (uint32_t)0x080FF800    /* Start address of FLAG location*/
//#define FLAG_ADDRESS    (uint32_t)0x08060000    /* Start address of FLAG location*/


#define MFW_FLASH_FLAG 0
#define RETRY_FLAG 1
#define FACTORY_FW 2
#define MFW_VER 3
#define FW_VALIDATION 4
#define FW_ROLLBACK 5
#define STABLE_FW 6

//#define MFW_FLAG  iu_all_flags[0]

#define ESP_FW_VER 7
#define ESP_FW_UPGRAD 8
#define ESP_RUNNING_VER 9
#define ESP_ROLLBACK 10



#define TEST_CHUNK_SIZE	256
#define STM_MFW_1 "STM-MFW1.bin"
#define STM_MFW_2 "STM-MFW2.bin"
#define ESP_MFW_1 "ESP-MFW1.bin"
#define ESP_MFW_2 "ESP-MFW2.bin"
//#define TEST_READ_FILE STM_MFW_1


uint8_t iu_all_flags[128];

void iu_read_all_flags(void);




void setup()
{
  delay(5000);
pinMode(6,OUTPUT);
digitalWrite(6,HIGH);
  
//	while (!Serial)
//		yield();
	DEBUG_SERIAL.begin(115200);
 
	DEBUG_SERIAL.print("Initializing External Flash Memory...");
	//DOSFS.format();
	if (!DOSFS.begin()) {
		DEBUG_SERIAL.println("Memory failed, or not present");
// don't do anything more:
//		while (1);
	}
	DEBUG_SERIAL.println("Memory initialized.");

}

void loop()
{
 // iu_all_flags[MFW_FLASH_FLAG] = 0;
  iu_read_all_flags();
//  iu_all_flags[MFW_FLASH_FLAG] = 1; /* Forcing the Flag, for testing */

     
//   DEBUG_SERIAL.println(switch_input1);


    switch (iu_all_flags[MFW_FLASH_FLAG])
    {
    case 0:  /* 0 -> Normal boot*/
           break;
    case 1:  /* 1 -> Flash STM Main Firmware */
          Flash_STM_File(STM_MFW_1);
          /*Verify_STM_FW();
           * If Error, iu_all_flags[MFW_FLASH_FLAG] = 2; else iu_all_flags[MFW_FLASH_FLAG] =3;
           */
           delay(2000);
           DEBUG_SERIAL.println("Setting MFW Flag");
           update_flag(0,3);
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
           DOSFS.rename("STM-MFW1.bin", "STM-TEMP.bin");
           delay(1000);
           DOSFS.rename("STM-MFW2.bin", "STM-MFW1.bin");
           delay(1000);
           DOSFS.rename("STM-TEMP.bin", "STM-MFW2.bin");
           delay(1000);
           DOSFS.remove("STM-TEMP.bin");
           delay(1000);
           Flash_STM_File(STM_MFW_1);
           /*Verify_STM_FW();
           * If Error, iu_all_flags[MFW_FLASH_FLAG] = 2; else iu_all_flags[MFW_FLASH_FLAG] =3;
           */
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
  DEBUG_SERIAL.print("Current position is="); DEBUG_SERIAL.println(currPos);
	fp->seek(newPos-currPos,SeekSet);
  DEBUG_SERIAL.print("Set file position to ="); DEBUG_SERIAL.println(newPos);
	bytesRead = fp->read(buffer, chunkSize);
	if (bytesRead < chunkSize) {
		DEBUG_SERIAL.println("Full chunk not read from file.");
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
//uint32_t read_flag(uint8_t FlagAddr)
//{
//  uint32_t FlashValue = 0;
//  uint32_t FlagAddr_temp = (FlagAddr*8)+FLAG_ADDRESS;
//  FlashValue = *(uint32_t*)(FlagAddr_temp);
//  return FlashValue;
//  }



  
/*--------------------------------------------------------------------------------------------------------*/  
uint16_t Flash_STM_File(char* TEST_READ_FILE)
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
#ifdef CHECK_SUM      
      for(int j=0;j<TEST_CHUNK_SIZE;j++){
      received[dataFileSize]= dataBuffer[j];
      //Serial.print(received);
      }
#endif      
      
    }
  }

#ifdef CHECK_SUM
  char *md5str = hashMD5.md5(received);;
  Serial.print("Expected MD5 code: ");
  Serial.println(md5str);
#endif
  
  iu_flash_close_file(&dataFileFP);
  //update_flag(0,3);
  stm32l4_flash_lock();
  DEBUG_SERIAL.println("Flashing completed..."); 
  return 1;
  
  }



uint16_t Verify_STM_File(char* TEST_READ_FILE)
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
//  stm32l4_flash_unlock();
//  stm32l4_flash_erase((uint32_t)MFW_ADDRESS,((dataFileSize/2048)+1)*2048);
  delay(1000);

   unsigned char received[dataFileSize];
   unsigned char read_data[dataFileSize];
   size_t charCount=0;
   
//  for (int i=1; i <= (dataFileSize/TEST_CHUNK_SIZE+1); i++) {
  for (int i=1; i <= (dataFileSize/TEST_CHUNK_SIZE)+1; i++) {
    if ((bytesRead=iu_flash_read_file_chunk(&dataFileFP, dataBuffer, TEST_CHUNK_SIZE, i)) > 0) {
      DEBUG_SERIAL.print("Chunk Seq="); DEBUG_SERIAL.println(i);
      DEBUG_SERIAL.print("Bytes Read="); DEBUG_SERIAL.println(bytesRead);
      //DEBUG_SERIAL.println((char*)dataBuffer);
//      stm32l4_flash_program((uint32_t)(MFW_ADDRESS + ((i-1)*TEST_CHUNK_SIZE)), dataBuffer, bytesRead);
      delay(10);
#ifdef CHECK_SUM      
      for(int j=0;j<TEST_CHUNK_SIZE;j++){
      received[dataFileSize]= dataBuffer[j];
      read_data[j] = *(uint8_t*)(MFW_ADDRESS + j);
      //Serial.print(received);
      }
#endif      
      
    }
  }

#ifdef CHECK_SUM
  char *md5str = hashMD5.md5(received);;
  Serial.print("Expected MD5 code: ");
  Serial.println(md5str);
#endif
  
  iu_flash_close_file(&dataFileFP);
  //update_flag(0,3);
  stm32l4_flash_lock();
  DEBUG_SERIAL.println("Flashing Verification completed..."); 
  return 1;
  
  }


  
  void update_flag(uint8_t flag_addr , uint8_t flag_data)
  {
    DEBUG_SERIAL.println("Updating flag..."); 
 /*
        uint8_t FlashData[128];
        for (int i = 0 ; i < 128; i++)
        FlashData[i] = i;
*/
   uint8_t flag_addr_temp = flag_addr * 8;
//    iu_read_all_flags();
//    iu_all_flags[flag_addr_temp] = flag_data;
    
//    iu_all_flags[flag_addr] = read_flag(flag_data);

/*
    DEBUG_SERIAL.println(flag_addr);
    DEBUG_SERIAL.println("");
    DEBUG_SERIAL.println(flag_data);
    DEBUG_SERIAL.println("");
    DEBUG_SERIAL.println(flag_addr_temp);
    DEBUG_SERIAL.println("");
    DEBUG_SERIAL.println(FLAG_ADDRESS,HEX);
    DEBUG_SERIAL.println("");
*/    
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



void iu_read_all_flags(void)
{
  for(int i = 0 ; i <128 ; i++)
  {
    iu_all_flags[i] = 0;
   }
   for (int i = 0 ; i < 128; i= i+8){
        iu_all_flags[i] = *(uint8_t*)(FLAG_ADDRESS + i);
        DEBUG_SERIAL.println(iu_all_flags[i],HEX);
    }    
}

#ifdef CHECK_SUM
#undef CHECK_SUM
#endif
/*--------------------------------*/
