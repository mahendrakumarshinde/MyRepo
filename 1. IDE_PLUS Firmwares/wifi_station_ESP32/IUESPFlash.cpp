#include "IUESPFlash.h"


/* =============================================================================
    IUESPFlash - Flash with file system
============================================================================= */

char IUESPFlash::CONFIG_SUBDIR[IUESPFlash::CONFIG_SUBDIR_LEN] = "/iuConfig/certs";

// char IUESPFlash::IUFWBACKUP_SUBDIR[IUESPFlash::CONFIG_SUBDIR_LEN] = "/iuBackupFirmware";
// char IUESPFlash::IUFWTMPIMG_SUBDIR[IUESPFlash::CONFIG_SUBDIR_LEN]  = "/iuTempFirmware";
// char IUESPFlash::IUFWROLLBACK_SUBDIR[IUESPFlash::CONFIG_SUBDIR_LEN] = "/iuRollbackFirmware";

char IUESPFlash::CONFIG_EXTENSION[IUESPFlash::CONFIG_EXTENSION_LEN] = ".conf";
char IUESPFlash::CERT_EXTENSION[IUESPFlash::CONFIG_EXTENSION_LEN] = ".crt";
char IUESPFlash::KEY_EXTENSION[IUESPFlash::CONFIG_EXTENSION_LEN] = ".key";


char IUESPFlash::FNAME_MQTTS_CLIENT0_CRT[12] = "client0";
char IUESPFlash::FNAME_MQTTS_CLIENT0_KEY[12] = "client0";
char IUESPFlash::FNAME_MQTTS_CLIENT1_CRT[12] = "client1";
char IUESPFlash::FNAME_MQTTS_CLIENT1_KEY[12] = "client1";

char IUESPFlash::FNAME_HTTPS_ROOTCA0_CRT[12] = "rootCA0";
char IUESPFlash::FNAME_HTTPS_ROOTCA1_CRT[12] = "rootCA1";

char IUESPFlash::FNAME_HTTPS_OEM_ROOTCA0_CRT[12] = "OEMrootCA0";
char IUESPFlash::FNAME_HTTPS_OEM_ROOTCA1_CRT[12] = "OEMrootCA1";

char IUESPFlash::FNAME_EAP_CLIENT0_CRT[12] = "eapClient0";
char IUESPFlash::FNAME_EAP_CLIENT0_KEY[12] = "eapClient0";

char IUESPFlash::FNAME_EAP_CLIENT1_CRT[12] = "eapClient1";
char IUESPFlash::FNAME_EAP_CLIENT1_KEY[12] = "eapClient1";


char IUESPFlash::FNAME_STATIC_URL[12] = "staticUrl";
char IUESPFlash::FNAME_UPGRADE_CONFIG[12] = "getCert";
char IUESPFlash::FNAME_DIAGNOSTIC_ENDPOINT[12] = "diagnosURL";
char IUESPFlash::FNAME_WIFI_CONFIG[12] = "wifi0";



// Core 
IUESPFlash::IUESPFlash(){
  
}

void IUESPFlash::begin(){
    if (!SPIFFS.begin() )
    {   
        if (debugMode)
        {   
            debugPrint("Failed to start SPIFFS (SPIFFS.begin returned false)");
        }
        m_begun = false;
        if(SPIFFS.begin(true)){    // Mount the File system if not mounted default
            m_begun = true;
          Serial.println("Forcefully mounted filesystem");
        }else
        {
            Serial.println("SPIFFS Not able to Mount Forcefully");
        }
    }else
    { 
        m_begun = true;
        Serial.println("begin() - CONFIG_SUBDIR exists");
    }
    Serial.print("m_begun : ");Serial.println(m_begun);
    // EEPROM begin
    eepromBegin(MEMORY_SIZE); 
    
}

File IUESPFlash::openFile(storedConfig configType ,const char* mode){
    if (!available())
    {   //Serial.println("OpenFile !available()");
        return File();
    }
    char filepath[MAX_FULL_CONFIG_FPATH_LEN];
    size_t fpathLen = getConfigFilename(configType, filepath,
                                        MAX_FULL_CONFIG_FPATH_LEN);
    if (fpathLen == 0 || filepath == NULL)
    {
        if (debugMode)
        {   
            debugPrint("Couldn't find the file name for config ", false);
            debugPrint(configType);
        }
        return File();
    }
    return SPIFFS.open(filepath, mode);    

}

/**
 * @brief  This method returns the file is present or not in the give filepath
 * 
 * @param filepath 
 * @return true  - success
 * @return false  - Failed
 */
bool IUESPFlash::isFilePresent(storedConfig configType){

    if (!available())
    {
        return File();
    }
    char filepath[MAX_FULL_CONFIG_FPATH_LEN];
    size_t fpathLen = getConfigFilename(configType, filepath,
                                        MAX_FULL_CONFIG_FPATH_LEN);
    if (fpathLen == 0 || filepath == NULL)
    {   
        //Serial.println("File not Preset ...");
        if (debugMode)
        {
            debugPrint("Couldn't find the file name ", false);
            debugPrint(configType);
        }
        return File();
    }
    
    return SPIFFS.exists(filepath);
}
/***** Config read / write functions *****/

/**
 * Read the relevant file to retrieve the stored config.
 *
 * Return the number of read chars. Returns 0 if the stored config type is
 * unknown or if the file is not found.
 */
size_t IUESPFlash::readFile(storedConfig configType,char *config,
                             size_t maxLength)
{
    File file = openFile(configType, "r");
    size_t readCharCount = 0;
    if (!file) {
        strcpy(config, "");
    }
    else {
        readCharCount = file.readBytes(config, maxLength);
    }
    file.close();
    //Serial.print("Read Count :");Serial.println(readCharCount);
    return readCharCount;
}

bool IUESPFlash::writeFile(storedConfig configType,char *config, size_t len){

    File file = openFile(configType, "w");
    size_t writtenCharCount = 0;
    if (file)
    {
        writtenCharCount = file.write((uint8_t*) config, len);
    }
    file.close();
    return writtenCharCount;
}

/**
 * @brief delete the confiFile 
 * 
 * @param configType 
 * @return true -success
 * @return false -Failed
 */
bool IUESPFlash::removeFile(storedConfig configType)
{
    if (!available()) {
        return false;
    }
    char filepath[MAX_FULL_CONFIG_FPATH_LEN];
    getConfigFilename(configType, filepath, MAX_FULL_CONFIG_FPATH_LEN);
    if (SPIFFS.exists(filepath)) {
        return SPIFFS.remove(filepath);
    } else {
        return false;
    }

}
/**
 * @brief  function used to rename the files
 * 
 * @param pathFrom 
 * @param pathTo 
 * @return true 
 * @return false 
 */
bool IUESPFlash::renameFile(storedConfig pathFrom, storedConfig pathTo){

    if (!available()) {
        return false;
    }
    char fromfilepath[MAX_FULL_CONFIG_FPATH_LEN];
    char tofilepath[MAX_FULL_CONFIG_FPATH_LEN];
    
    getConfigFilename(pathFrom, fromfilepath, MAX_FULL_CONFIG_FPATH_LEN);
    getConfigFilename(pathTo, tofilepath, MAX_FULL_CONFIG_FPATH_LEN);
    
    if (SPIFFS.exists(fromfilepath)) {
        return SPIFFS.rename(fromfilepath,tofilepath);
    } else {
        return true;
    }

}

/**
 * @brief List all the available files from the given directory
 * 
 * @param folderPath 
 * @return size_t 
 */
size_t IUESPFlash::listAllAvailableFiles(const char* folderPath){
    
  File root = SPIFFS.open(folderPath);
  File file = root.openNextFile();
 
  while(file){
      Serial.print("FILE: ");
      Serial.print(file.name());Serial.print("\t\t");
      Serial.println(file.size());
      file = root.openNextFile();
  }
}

size_t IUESPFlash::getFileSize(storedConfig configType){
    
    if (!available()) {
        return false;
    }
    char filepath[MAX_FULL_CONFIG_FPATH_LEN];
    getConfigFilename(configType, filepath, MAX_FULL_CONFIG_FPATH_LEN);
    File file =  SPIFFS.open(filepath);
    
    if (file) {
        return file.size();
    } else {
        return 0;
    }
}

bool IUESPFlash:: updateCommonHttpEndpoint(storedConfig configType){

    if (!available())
    {
        return false;
    }

    if (isFilePresent(configType))
    {
        StaticJsonBuffer<512> jsonBuffer;
        JsonVariant config = JsonVariant(loadConfigJson(configType,jsonBuffer));
        bool success = config.success();
        Serial.print("Using COMMON URL from File :");
        config.prettyPrintTo(Serial);
        const char* url = config["certUrl"]["url"];
        strcpy(certificateConfigurationEndpoint,url);

        return true;
    }else
    {
        // Serial.println("Common HTTP Endpoint file is not present, use default");
        char* json = "{\"certUrl\":{\"url\":\"https:\/\/fc4e0c8a-bf30-4da2-8b31-05b85c445721.mock.pstmn.io\/getCertConfig\"},\"messageId\":\"cEgxwaPKJRCRloSNYW0xk3GFp\"}";
        
        strcpy(certificateConfigurationEndpoint,json);
        return false;
    }
    
}
/****Utility ****/

/**
 * Find and return the file name of the requested config type.
 *
 * Dest char array pointer should at least be of length
 * MAX_FULL_CONFIG_FPATH_LEN.
 */
size_t IUESPFlash::getConfigFilename(storedConfig configType, char *dest,
                                    size_t len)
{
    char *fname = NULL;
    char FILE_EXTENSION[6];
    switch (configType)
    {
        case CFG_MQTT_CLIENT0:
            fname = FNAME_MQTTS_CLIENT0_CRT;
            strcpy(FILE_EXTENSION,CERT_EXTENSION);
            break;
        case CFG_MQTT_CLIENT1:
            fname = FNAME_MQTTS_CLIENT1_CRT;
            strcpy(FILE_EXTENSION,CERT_EXTENSION);
            break;
        case CFG_MQTT_KEY0:
            fname = FNAME_MQTTS_CLIENT0_KEY;
            strcpy(FILE_EXTENSION,KEY_EXTENSION);
            break;
        case CFG_MQTT_KEY1:
            fname = FNAME_MQTTS_CLIENT1_KEY;
            strcpy(FILE_EXTENSION,KEY_EXTENSION);
            break;
        case CFG_HTTPS_ROOTCA0:
            fname = FNAME_HTTPS_ROOTCA0_CRT;
            strcpy(FILE_EXTENSION,CERT_EXTENSION);
            break;
        case CFG_HTTPS_ROOTCA1:
            fname = FNAME_HTTPS_ROOTCA1_CRT;
            strcpy(FILE_EXTENSION,CERT_EXTENSION);
            break;
        case CFG_HTTPS_OEM_ROOTCA0:
            fname = FNAME_HTTPS_OEM_ROOTCA0_CRT;
            strcpy(FILE_EXTENSION,CERT_EXTENSION);
            break;
        case CFG_HTTPS_OEM_ROOTCA1:
            fname = FNAME_HTTPS_OEM_ROOTCA1_CRT;
            strcpy(FILE_EXTENSION,CERT_EXTENSION);
            break;
        case CFG_EAP_CLIENT0 :
            fname = FNAME_EAP_CLIENT0_CRT;
            strcpy(FILE_EXTENSION,CERT_EXTENSION);
            break;
        case CFG_EAP_CLIENT1:
            fname = FNAME_EAP_CLIENT1_CRT;
            strcpy(FILE_EXTENSION,CERT_EXTENSION);
            break;
        case CFG_EAP_KEY0:
            fname = FNAME_EAP_CLIENT0_KEY;
            strcpy(FILE_EXTENSION,KEY_EXTENSION);
            break;
        case CFG_EAP_KEY1:
            fname = FNAME_EAP_CLIENT1_KEY;
            strcpy(FILE_EXTENSION,KEY_EXTENSION);
            break;
        case CFG_STATIC_CERT_ENDPOINT:
            fname = FNAME_STATIC_URL;
            strcpy(FILE_EXTENSION,CONFIG_EXTENSION);
            break;
        case CFG_CERT_UPGRADE_CONFIG:
            fname = FNAME_UPGRADE_CONFIG;
            strcpy(FILE_EXTENSION,CONFIG_EXTENSION);
            break;
        case CFG_DIAGNOSTIC_ENDPOINT:
            fname = FNAME_DIAGNOSTIC_ENDPOINT;
            strcpy(FILE_EXTENSION,CONFIG_EXTENSION);
            break;
        case CFG_WIFI:
            fname = FNAME_WIFI_CONFIG;
            strcpy(FILE_EXTENSION,CONFIG_EXTENSION);
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unknown config type "), false);
                debugPrint(configType);
            }
            return 0;
    }
    if (fname == NULL)
    {
        return 0;
    }
    
    return snprintf(dest, len, "%s/%s%s", CONFIG_SUBDIR, fname,
                    FILE_EXTENSION);
}

// File IUESPFlash::openConfigFile(storedConfig configType,
//                                        const char* mode)
// {
//     if (!available())
//     {
//         return File();
//     }
//     char filepath[MAX_FULL_CONFIG_FPATH_LEN];
//     size_t fpathLen = getConfigFilename(configType, filepath,
//                                         MAX_FULL_CONFIG_FPATH_LEN);
//     if (fpathLen == 0 || filepath == NULL)
//     {
//         if (debugMode)
//         {
//             debugPrint("Couldn't find the file name for config ", false);
//             debugPrint(configType);
//         }
//         return File();
//     }
//     return SPIFFS.open(filepath, mode);
    
// }

/***** JSON Config load / save functions *****/

bool IUESPFlash::saveConfigJson(storedConfig configType, JsonVariant &config)
{
    if (!available()) {
        return false;
    }
    File file = openFile(configType, "w");
    if (config.printTo(file) == 0)
    {
        return false;
    }
    file.close();
    if (debugMode)
    {
        debugPrint("Successfully saved config type #", false);
        debugPrint((uint8_t) configType);
    }
    return true;
}

