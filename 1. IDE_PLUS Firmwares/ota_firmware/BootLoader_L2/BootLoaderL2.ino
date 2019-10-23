#include "FS.h"
#include <Arduino.h>
#include "stm32l4_flash.h"

#define FFW_ADDRESS     (uint32_t)0x08010000    /* Start address of Facotry Firmware */
#define MFW_ADDRESS     (uint32_t)0x08060000    /* Start address of Main Firmware */
#define FLAG_ADDRESS    (uint32_t)0x080FF800    /* Start address of FLAG location*/


#define MFW_FLASH 0
#define FACTORY_FW 1
#define MFW_VER 2
#define FW_VALIDATION 3
#define FW_ROLLBACK 4
#define STABLE_FW 5

#define ESP_FW_VER 6
#define ESP_FW_UPGRAD 7
#define ESP_RUNNING_VER 8
#define ESP_ROLLBACK 9



#define TEST_CHUNK_SIZE	256
#define STM_MFW_1 "STM-MFW1.bin"
#define STM_MFW_2 "STM-MFW2.bin"
#define ESP_MFW_1 "ESP-MFW1.bin"
#define ESP_MFW_2 "ESP-MFW2.bin"
//#define TEST_READ_FILE STM_MFW_1


uint32_t all_flags[32];

void setup()
{
  delay(5000);

  
//	while (!Serial)
//		yield();
	Serial.begin(115200);
	Serial.print("Initializing External Flash Memory...");
  Serial1.begin(115200);
  Serial1.print("Initializing External Flash Memory...");
	DOSFS.format();
	if (!DOSFS.begin()) {
		Serial.println("Memory failed, or not present");
// don't do anything more:
//		while (1);
	}
	Serial.println("Memory initialized.");

}

void loop()
{
  read_all_flags();
  all_flags[MFW_FLASH] = 1;
  if(all_flags[MFW_FLASH] == 1)
  {
    Flash_STM_File(STM_MFW_1);
    }
    

	while (1);
}

int iu_flash_open_file(File *fp, char * fileName, const char * fileMode, size_t * fileSize)
{
	*fp = DOSFS.open(fileName, fileMode);
	if (!*fp) {
		Serial.println("Error opening file...");
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
  Serial.print("Current position is="); Serial.println(currPos);
	fp->seek(newPos-currPos,SeekSet);
  Serial.print("Set file position to ="); Serial.println(newPos);
	bytesRead = fp->read(buffer, chunkSize);
	if (bytesRead < chunkSize) {
		Serial.println("Full chunk not read from file.");
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
		Serial.println("Full buffer not written to file.");
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
uint32_t read_flag(uint8_t FlagAddr)
{
  uint32_t FlashValue = 0;
  uint32_t FlagAddr_temp = (FlagAddr*8)+FLAG_ADDRESS;
  FlashValue = *(uint32_t*)(FlagAddr_temp);
  return FlashValue;
  }
void read_all_flags()
{
  
  for(int i = 0 ; i <32 ; i++)
  {
    all_flags[i] = read_flag(i);
    }
  }
uint16_t Flash_STM_File(char* TEST_READ_FILE)
{
    Serial.println("dosFileSys: Writing to Memory");
  File dataFileFP;
  size_t dataFileSize, bytesRead;
  uint8_t dataBuffer[TEST_CHUNK_SIZE];

  if (iu_flash_open_file(&dataFileFP, TEST_READ_FILE, "r", &dataFileSize)
    != F_NO_ERROR) {
    Serial.println("Error Opening file.");
  }
  Serial.print("File Size is ="); Serial.println(dataFileSize);
  stm32l4_flash_erase((uint32_t)MFW_ADDRESS,((dataFileSize/2048)+1)*2048);
  delay(1000);
//  for (int i=1; i <= (dataFileSize/TEST_CHUNK_SIZE+1); i++) {
  for (int i=1; i <= (dataFileSize/TEST_CHUNK_SIZE)+1; i++) {
    if ((bytesRead=iu_flash_read_file_chunk(&dataFileFP, dataBuffer, TEST_CHUNK_SIZE, i)) > 0) {
      Serial.print("Chunk Seq="); Serial.println(i);
      Serial.print("Bytes Read="); Serial.println(bytesRead);
      //Serial.println((char*)dataBuffer);
      stm32l4_flash_program((uint32_t)(MFW_ADDRESS + ((i-1)*TEST_CHUNK_SIZE)), dataBuffer, bytesRead);
      delay(10);
    }
  }
  iu_flash_close_file(&dataFileFP);
  Serial.println("Flashing completed...");
  return 1;
  
  }

/*--------------------------------*/



/*void dis_setup()
{
    uint64_t  FlashValue = 0;
    volatile uint32_t *FlashPtr;
    Serial.begin(115200);
    delay(5000);
    Serial.println("Flash Test begin...");
    uint8_t FlashData[2048];
    for (int i = 0 ; i < 1024; i++)
        FlashData[i] = 0xAA;
    for (int i = 1024 ; i < 2048; i++)
        FlashData[i] = 0x55;
//    stm32l4_flash_erase((uint32_t)APP_ADDRESS,2048);
    delay(1000);
//    stm32l4_flash_program((uint32_t)(APP_ADDRESS),&FlashData[0],2048);
    delay(1000);
    for (int i = 0 ; i < 2048; i++)
        FlashData[i] = 0x00;
    delay(1000);

    Serial.print("Flash Data:");
    uint32_t FlashValue1 = 0;
    for (int i = 0 ; i < 512; i++)
    {
  //      FlashValue1 = *(volatile uint32_t*)(APP_ADDRESS + (i*4));
        Serial.print(" 0x");
        Serial.println(FlashValue1,HEX);
    }
}*/
