#ifndef IUESPFLASH_H
#define IUESPFLASH_H

#include <MacAddress.h>
//#include "Utilities.h"
#include "BoardDefinition.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <Arduino.h>
#include <IUDebugger.h>
/**
 * A class to perform the file IO operations 
 * The class is typically instanciated with an empty payload, that is then
 * 
 *
 * */
class IUESPFlash
{
    public:
        /***** Preset values and default settings *****/
        enum storedConfig : uint8_t {CFG_MQTT_CLIENT0,
                                     CFG_MQTT_CLIENT1,
                                     CFG_MQTT_KEY0,
                                     CFG_MQTT_KEY1,
                                     CFG_HTTPS_ROOTCA0,
                                     CFG_HTTPS_ROOTCA1,
                                     CFG_EAP_CLIENT0,
                                     CFG_EAP_CLIENT1,
                                     CFG_EAP_KEY0,
                                     CFG_EAP_KEY1,
                                     CFG_STATIC_CERT_ENDPOINT,
                                     CFG_CERT_UPGRADE_CONFIG,
                                     CFG_DIAGNOSTIC_ENDPOINT,
                                     CFG_COUNT};
        
        /***** Preset values and default settings *****/
        static const uint8_t CONFIG_SUBDIR_LEN = 20;
        static char CONFIG_SUBDIR[CONFIG_SUBDIR_LEN];
        static char IUCERTBACKUP_SUBDIR[CONFIG_SUBDIR_LEN];
        static const uint8_t CONFIG_EXTENSION_LEN = 6;
        static char CONFIG_EXTENSION[CONFIG_EXTENSION_LEN];
        static char CERT_EXTENSION[CONFIG_EXTENSION_LEN];
        static char KEY_EXTENSION[CONFIG_EXTENSION_LEN];
        
        static char FNAME_MQTTS_CLIENT0_KEY[12];
        static char FNAME_MQTTS_CLIENT0_CRT[12];
        static char FNAME_MQTTS_CLIENT1_KEY[12];      // as a backup
        static char FNAME_MQTTS_CLIENT1_CRT[12];      // as a backup
        static char FNAME_HTTPS_ROOTCA0_CRT[12];
        static char FNAME_HTTPS_ROOTCA1_CRT[12];      // as a backup
        
        static char FNAME_UPGRADE_CONFIG[12];           // store the cert download json message
        static char FNAME_STATIC_URL[12];      // store the common url
        static char FNAME_DIAGNOSTIC_ENDPOINT[12];

        static char FNAME_EAP_CLIENT0_KEY[12];
        static char FNAME_EAP_CLIENT0_CRT[12];
        static char FNAME_EAP_CLIENT1_KEY[12];
        static char FNAME_EAP_CLIENT1_CRT[12];
        
        static const uint8_t MAX_FULL_CONFIG_FPATH_LEN = 50;
        char* certificateConfigurationEndpoint;
        /***** Core *****/
        IUESPFlash();
        virtual ~IUESPFlash() {}
        /***** Endpoint *****/
        virtual void begin();
        virtual bool available() { return m_begun; }

        File openFile(storedConfig configType,const char* mode);
        bool isFilePresent(storedConfig configType);   
        size_t readFile(storedConfig configType,char *config,
                             size_t maxLength);
        bool writeFile(storedConfig configType,char *config, 
                             size_t len);

        bool removeFile(storedConfig configType);
        bool renameFile(storedConfig pathFrom, storedConfig pathTo);
        size_t listAllAvailableFiles(const char* folderPath);
        size_t getFileSize(storedConfig configType);
         /***** JSON Config load / save functions *****/
        template <size_t capacity>
        JsonObject& loadConfigJson(
            storedConfig configType, StaticJsonBuffer<capacity> &jsonBuffer);
        virtual bool saveConfigJson(storedConfig configType, JsonVariant &config);
        //virtual bool updateConfigJson(storedConfig configType, JsonVariant &config);
        virtual bool updateCommonHttpEndpoint(storedConfig configType);

        /***** Utility *****/
        size_t getConfigFilename(storedConfig configType, char *dest,
                                 size_t len);
        //File openConfigFile(storedConfig configType, const char* mode);



    protected:
        
        bool m_begun = false;
        //HTTPClient http_cert;

};

template <size_t capacity>
JsonObject& IUESPFlash::loadConfigJson(
    storedConfig configType, StaticJsonBuffer<capacity> &jsonBuffer)
{
    File file = openFile(configType, "r");
    JsonObject &root = jsonBuffer.parseObject(file);
    file.close();
    return root;
}

#endif // IUESPFLASH_H
