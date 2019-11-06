/**
   AWS S3 OTA Update
   Date: 14th June 2017
   Author: Arvind Ravulavaru <https://github.com/arvindr21>
   Purpose: Perform an OTA update from a bin located in Amazon S3 (HTTP Only)

   Upload:
   Step 1 : Download the sample bin file from the examples folder
   Step 2 : Upload it to your Amazon S3 account, in a bucket of your choice
   Step 3 : Once uploaded, inside S3, select the bin file >> More (button on top of the file list) >> Make Public
   Step 4 : You S3 URL => http://bucket-name.s3.ap-south-1.amazonaws.com/sketch-name.ino.bin
   Step 5 : Build the above URL and fire it either in your browser or curl it `curl -I -v http://bucket-name.ap-south-1.amazonaws.com/sketch-name.ino.bin` to validate the same
   Step 6:  Plug in your SSID, Password, S3 Host and Bin file below

   Build & upload
   Step 1 : Menu > Sketch > Export Compiled Library. The bin file will be saved in the sketch folder (Menu > Sketch > Show Sketch folder)
   Step 2 : Upload bin to S3 and continue the above process

   // Check the bottom of this sketch for sample serial monitor log, during and after successful OTA Update
*/

#include <WiFi.h>
#include <Update.h>
#include "FS.h"
#include "SPIFFS.h"

#include "esp_https_ota.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "FS.h"
#include "SPIFFS.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

WiFiClient client;

// Variables to validate
// response from S3
long contentLength = 0;
bool isValidContentType = false;

// Your SSID and PSWD that the chip needs
// to connect to
const char* SSID = "TECH-Team";
const char* PSWD = "Tech@2019";

//const char* SSID = "JioFi2_Omkar";
//const char* PSWD = "";


//const char* SSID = "dsmmob";
//const char* PSWD = "8884230693";


// S3 Bucket Config
String host = "iu-firmware.s3.ap-south-1.amazonaws.com"; // Host => bucket-name.s3.region.amazonaws.com
int port = 80; // Non https. For HTTPS 443. As of today, HTTPS doesn't work.
String stm_fw1 = "/butterfly_firmware.ino.dfu"; // bin file name with a slash in front.
String stm_fw = "/butterfly_firmware.ino.iap"; // bin file name with a slash in front.
String wifi_fw = "/wifi_station_ESP8285.ino.bin"; // bin file name with a slash in front.
String wifi_fw1 = "/WiFiClient.ino.bin"; // bin file name with a slash in front.
// Utility to extract header value from headers
String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

// OTA Logic 
bool execOTA(String bin)
{
  Serial.println("Connecting to: " + String(host));
  // Connect to S3
  if (client.connect(host.c_str(), port)) 
  {
    // Connection Succeed.
    // Fecthing the bin
    Serial.println("Fetching Bin: " + String(bin));
    // Get the contents of the bin file
    client.print(String("GET ") + bin + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Cache-Control: no-cache\r\n" +
                 "Connection: close\r\n\r\n");

    // Check what is being sent
        Serial.print(String("GET ") + bin + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Cache-Control: no-cache\r\n" +
                     "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 10000) {
        Serial.println("Client Timeout !");
        client.stop();
        return false;
      }
    }
    // Once the response is available,
    // check stuff

    /*
       Response Structure
        HTTP/1.1 200 OK
        x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
        x-amz-request-id: 2D56B47560B764EC
        Date: Wed, 14 Jun 2017 03:33:59 GMT
        Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
        ETag: "d2afebbaaebc38cd669ce36727152af9"
        Accept-Ranges: bytes
        Content-Type: application/octet-stream
        Content-Length: 357280
        Server: AmazonS3
                                   
        {{BIN FILE CONTENTS}}

    */
    while (client.available())
    {
      // read line till /n
      String line = client.readStringUntil('\n');
      // remove space, to check if the line is end of headers
      line.trim();

      // if the the line is empty,
      // this is end of headers
      // break the while and feed the
      // remaining `client` to the
      // Update.writeStream();
      if (!line.length()) {
        //headers ended
        break; // and get the OTA started
      }

      Serial.print("HTTP Response:");
      Serial.println(line);

      // Check if the HTTP Response is 200
      // else break and Exit Update
      if (line.startsWith("HTTP/1.1")) {
        if (line.indexOf("200") < 0) {
          Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
          break;
        }
      }

      // extract headers here
      // Start with content length
      if (line.startsWith("Content-Length: ")) {
        contentLength = atol((getHeaderValue(line, "Content-Length: ")).c_str());
        Serial.println("Got " + String(contentLength) + " bytes from server");
      }

      // Next, the content type
      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        Serial.println("Got " + contentType + " payload.");
//        if (contentType == "application/octet-stream") {
        if (contentType == "application/octet-stream" || contentType == "binary/octet-stream") {
          isValidContentType = true;
        }
      }
    }
  } 
  else 
  {
    Serial.println("Connection to " + String(host) + " failed. Please check your setup");
    return false; 
  }

  // Check what is the contentLength and if content type is `application/octet-stream`
  Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

  // check contentLength and content type
  if (contentLength && isValidContentType) 
  {
    // Check if there is enough to OTA Update
    bool canBegin = Update.begin(contentLength);
    // If yes, begin
    if (canBegin) 
    {
  //    Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      // No activity would appear on the Serial monitor
      // So be patient. This may take 2 - 5mins to complete
#if 1    
      if(WriteBinFile(SPIFFS,contentLength,bin) == false)
      {
        Serial.println("SPIFFS Write File Failed !!");
        client.flush();
        return false;
      }
#endif
#if 0
      size_t written = Update.writeStream_OTA(client);
      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
        // retry??
        // execOTA();
      }

      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
  //        ESP.restart();
        } else {
          Serial.println("Update not finished? Something went wrong!");
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }
#endif
    }
    else
    {
      // not enough space to begin OTA
      // Understand the partitions and
      // space availability
      Serial.println("Not enough space to begin OTA");
      client.flush();
      return false;
    }
  }
  else
  {
    Serial.println("There was no content in the response");
    client.flush();
    return false;
  }
  return true;
}

void setup() {
  bool STM_Rx_Sts = false;
  bool ESP_Rx_Sts = false;
  //Begin Serial
  Serial.begin(115200);
  delay(10);

  //disable brownout detector - ESP32_PORT_TRUE Temp. change to prevent reset due to supply drop
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  const esp_partition_t *SPIFFS_partition = NULL;

  if(!SPIFFS.begin(true)){
      Serial.println("SPIFFS Mount Failed");
      return;
  }

 // SPIFFS.format();


  Serial.println("Connecting to " + String(SSID));

  // Connect to provided SSID and PSWD
  WiFi.begin(SSID, PSWD);

  // Wait for connection to establish
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); // Keep the serial monitor lit!
    delay(500);
  }
  // Connection Succeed
  Serial.println("");
  Serial.println("Connected to " + String(SSID));
  Serial.println("***********************************************");

 // GetPartitionDetail();

  if(SPIFFS.remove(stm_fw.c_str()))
    Serial.println("Removed " + String(stm_fw));

  if(SPIFFS.remove(stm_fw1.c_str()))
    Serial.println("Removed " + String(stm_fw1));

  if(SPIFFS.remove(wifi_fw.c_str()))
    Serial.println("Removed " + String(wifi_fw));

  if(SPIFFS.remove(wifi_fw1.c_str()))
    Serial.println("Removed " + String(wifi_fw1));

 // SPIFFS.format();

 // SPIFFS_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,NULL);
 // esp_partition_erase_range(SPIFFS_partition,SPIFFS_partition->address,SPIFFS_partition->size);

  // Execute OTA Update
  STM_Rx_Sts = execOTA(stm_fw);
  if(STM_Rx_Sts == true)
  {
    Serial.println("STM FW Received, stored on SPIFFS.");
  // ReadBinFile(SPIFFS,stm_fw);
  }
#if 1
  if(STM_Rx_Sts == true) 
  {
  // delay(100);
    Update.abort(); // Temp. Added to reset execOTA - Update class members to receive next ESP FW
    ESP_Rx_Sts = execOTA(wifi_fw);
    if(ESP_Rx_Sts == true) 
    {
  // ReadBinFile(SPIFFS,wifi_fw);
#if 1
      Serial.println("\n*********************************");
      Serial.println("Updating ESP32 FW - OTA");
      Serial.println("*********************************");
      size_t written = Update.writeStream_SPIFFS(wifi_fw,contentLength);
      if (written == contentLength) 
      {
        Serial.println("Written : " + String(written) + " successfully");
      } else 
      {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
        // retry??
        // execOTA();
      }
      if (Update.end1()) 
      {
          Serial.println("OTA done!");
          Serial.println("Update successfully completed. Rebooting.");
          SPIFFS.end();
          ESP.restart();
      } else 
      {
          Serial.println("Update not finished? Something went wrong!");
      }
#endif
    }
  }
#endif
  SPIFFS.end();
}

void loop() {
  // chill
  delay(100);
}

void GetPartitionDetail()
{
  const esp_partition_t *update_partition = NULL;
  const esp_partition_t *SPIFFS_partition = NULL;

  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  Serial.print("Boot Partition Add:0x");
  Serial.println(configured->address,HEX);
  Serial.print("Boot Type: ");
  Serial.print(configured->type,DEC);
  Serial.print(" Boot Subtype:");
  Serial.print(configured->subtype,DEC);
  Serial.print(" Boot Size:0x");
  Serial.println(configured->size,HEX);

  Serial.print("Running Partition Add:0x");
  Serial.println(running->address,HEX);
  Serial.print("Running Type:");
  Serial.print(running->type,DEC);
  Serial.print(" Running Subtype:");
  Serial.print(running->subtype,DEC);
  Serial.print(" Running Size:0x");
  Serial.println(running->size,HEX);

  update_partition = esp_ota_get_next_update_partition(NULL);
  Serial.print("Update Partition Add:0x");
  Serial.println(update_partition->address,HEX);
  Serial.print("Update Type: ");
  Serial.print(update_partition->type,DEC);
  Serial.print(" Update Subtype:");
  Serial.print(update_partition->subtype,DEC);
  Serial.print(" Update Size:0x");
  Serial.println(update_partition->size,HEX);

  SPIFFS_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,NULL);
  Serial.print("SPIFFS Partition Add:");
  Serial.println(SPIFFS_partition->address,HEX);
  Serial.print("SPIFFS Type: ");
  Serial.print(SPIFFS_partition->type,DEC);
  Serial.print("  SPIFFS Subtype:");
  Serial.print(SPIFFS_partition->subtype,DEC);
  Serial.print("  SPIFFS Size:0x");
  Serial.println(SPIFFS_partition->size,HEX);
}

bool WriteBinFile(fs::FS &fs,long FileSize,String path)
{
    Serial.println("FW file write:" + String(path));
    static uint8_t buf[512];
    int totalblock = FileSize/512;
    size_t count = 0;
    int loopCnt = 0;
    size_t ReadBlock = 512;
    long len = FileSize;
    unsigned long time1 = 0;
    unsigned long time2 = 0;   

    if(FileSize%512 != 0)
      totalblock =  totalblock +1;

    Serial.println("Total Blocks to write:" + String(totalblock));

    File file = fs.open(path.c_str(), FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return false;
    }
    Serial.println("Writing bin data:");    
    while(1)
    {
      delay(50);
  //    time1 = millis();
      count = client.readBytes(buf,ReadBlock);
  //    time2 = millis();
      if(count != 0) 
      {
        loopCnt++;
        Serial.print("Writing Block: ");
        Serial.print(loopCnt);
        Serial.print(" / ");
        Serial.println(totalblock);
 //       Serial.print(" Read Time: ");
  //      Serial.print(time2-time1);
  //     Serial.print("Data[");
  //     Serial.print(loopCnt++);
  //     Serial.print("] Size:");
  //     Serial.println(count);
  //     for(int j = 0; j < count; j++)
    //      Serial.print(" " + String(buf[j]));
  //      time1 = millis();
        file.write(buf, count);
   //     time2 = millis();
   //     Serial.print(" File Write Time: ");
   //     Serial.println(time2-time1);
        if(count < 512) {
          if(loopCnt == totalblock)
          {
            Serial.println("Last Packet ");
            break; // on last packet          
          }
          else
          {
            Serial.println("Failed Write !!");
            return false;
          }          
        }
        if(len < 512)
          ReadBlock = len;
        else
          len = len - count;
      }
      else
      {
        Serial.println("Bin file read failed !");
        file.close();
        return false;
      }
    }
    file.close();
    return true;
}


void ReadBinFile(fs::FS &fs, String fwpath)
{
    Serial.print("FW file read:" + String(fwpath));
    static uint8_t buf[512];
    int totalblock = 0;
    size_t count = 0;
    size_t ReadBlock = 512;
    int loopCnt = 0;
    long len = 0;
    File file = fs.open(fwpath.c_str(), FILE_READ);
    if(!file){
        Serial.println("- failed to open file for reading");
        return;
    }

    if(file && !file.isDirectory()){
        len = file.size();
        totalblock = len/512;
        if(len % 512 != 0)
          totalblock = totalblock + 1;
        size_t flen = len;
        Serial.print(" Reading:" );
        Serial.println(len,DEC);
        while(len){
            Serial.println("Reading Block: " + String(loopCnt) + "/" + String(totalblock));
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
 //           Serial.print(" Block:");
 //           Serial.println(toRead,DEC);
            loopCnt++;
            file.read(buf, toRead);
     //       for(int j = 0; j < toRead; j++)
      //        Serial.print(" " + String(buf[j]));
            len -= toRead;
        }
        Serial.println("");
        file.close();
    } else {
        Serial.println("Not File valid file type");
    }
}

#if 0
void testFileIO(fs::FS &fs, const char * path){
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing" );
    uint32_t start = millis();
    for(i=0; i<2048; i++){
        if ((i & 0x001F) == 0x001F){
          Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if(file && !file.isDirectory()){
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading" );
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F){
              Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    } else {
        Serial.println("- failed to open file for reading");
    }
}
#endif
/*
 * Serial Monitor log for this sketch
 * 
 * If the OTA succeeded, it would load the preference sketch, with a small modification. i.e.
 * Print `OTA Update succeeded!! This is an example sketch : Preferences > StartCounter`
 * And then keeps on restarting every 10 seconds, updating the preferences
 * 
 * 
      rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
      configsip: 0, SPIWP:0x00
      clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
      mode:DIO, clock div:1
      load:0x3fff0008,len:8
      load:0x3fff0010,len:160
      load:0x40078000,len:10632
      load:0x40080000,len:252
      entry 0x40080034
      Connecting to SSID
      ......
      Connected to SSID
      Connecting to: bucket-name.s3.ap-south-1.amazonaws.com
      Fetching Bin: /StartCounter.ino.bin
      Got application/octet-stream payload.
      Got 357280 bytes from server
      contentLength : 357280, isValidContentType : 1
      Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!
      Written : 357280 successfully
      OTA done!
      Update successfully completed. Rebooting.
      ets Jun  8 2016 00:22:57
      
      rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
      configsip: 0, SPIWP:0x00
      clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
      mode:DIO, clock div:1
      load:0x3fff0008,len:8
      load:0x3fff0010,len:160
      load:0x40078000,len:10632
      load:0x40080000,len:252
      entry 0x40080034
      
      OTA Update succeeded!! This is an example sketch : Preferences > StartCounter
      Current counter value: 1
      Restarting in 10 seconds...
      E (102534) wifi: esp_wifi_stop 802 wifi is not init
      ets Jun  8 2016 00:22:57
      
      rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
      configsip: 0, SPIWP:0x00
      clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
      mode:DIO, clock div:1
      load:0x3fff0008,len:8
      load:0x3fff0010,len:160
      load:0x40078000,len:10632
      load:0x40080000,len:252
      entry 0x40080034
      
      OTA Update succeeded!! This is an example sketch : Preferences > StartCounter
      Current counter value: 2
      Restarting in 10 seconds...

      ....
 * 
 */
