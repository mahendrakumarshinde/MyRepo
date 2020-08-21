#ifndef IUFLASH_H
#define IUFLASH_H

#include <Arduino.h>
#include <FS.h>
#include <SPI.h>
#include <ArduinoJson.h>

#include <IUDebugger.h>
#include "stm32l4_flash.h"
#define CONFIG_MQTT_FLASH_ADDRESS    (uint32_t)0x080FE800    /* Start address of MQTT Config location*/
#define CONFIG_HTTP_FLASH_ADDRESS    (uint32_t)0x080FE000    /* Start address of HTTP Config location*/
#define CONFIG_MODBUS_SLAVE_CONFIG_FLASH_ADDRESS    (uint32_t)0x80FD800    /* Start address of MODBUS slave Config location*/
#define CONFIG_WIFI_CONFIG_FLASH_ADDRESS    (uint32_t)0x80FD000    /* Start address of MODBUS slave Config location*/

class IUFlash
{
    public:
        /***** Preset values and default settings *****/
        enum storedConfig : uint8_t {CFG_WIFI0,
                                     CFG_WIFI1,
                                     CFG_WIFI2,
                                     CFG_WIFI3,
                                     CFG_WIFI4,
                                     CFG_FEATURE,
                                     CFG_COMPONENT,
                                     CFG_DEVICE,
                                     CFG_OP_STATE,
                                     CFG_RAW_DATA_ENDPOINT,
                                     CFG_MQTT_SERVER,
                                     CFG_MQTT_CREDS,
                                     CFG_FFT,
                                     CFG_HTTP,
                                     CFG_OTA,
                                     CFG_FORCE_OTA, // Forced OTA request,
                                     CFG_MODBUS_SLAVE,
                                     CFG_DIG,
                                     CFG_DESC,
                                     CFG_COUNT}; 
        /***** Core *****/
        IUFlash() {}
        virtual ~IUFlash() {}
        virtual void begin() = 0;
        virtual bool available() { return m_begun; }
        /***** Config read / write functions *****/
        virtual size_t readConfig(storedConfig configType, char *config,
                                  size_t maxLength) = 0;
        virtual size_t writeConfig(storedConfig configType, char *config,
                                   size_t len) = 0;
        virtual size_t writeConfig(storedConfig configType, char *config)
            { writeConfig(configType, config, strlen(config)); }
        virtual bool deleteConfig(storedConfig configType) = 0;
        /***** JSON Config load / save functions *****/
        virtual bool saveConfigJson(storedConfig configType,
                                    JsonVariant &config) = 0;
        virtual bool updateConfigJson(storedConfig configType, JsonVariant &config) = 0;
        virtual bool validateConfig(storedConfig configType, JsonObject &config, char *validationResultString, char* mac_id, double timestamp, char* messageId) = 0;

    protected:
        bool m_begun = false;
        bool m_otaDir = false;
        bool m_digDir = false;
};


class IUFSFlash : public IUFlash
{
    public:
        /***** Preset values and default settings *****/
        static const uint8_t CONFIG_SUBDIR_LEN = 24;
        static char CONFIG_SUBDIR[CONFIG_SUBDIR_LEN];
        static char IUFWBACKUP_SUBDIR[CONFIG_SUBDIR_LEN];
        static char IUFWTMPIMG_SUBDIR[CONFIG_SUBDIR_LEN];
        static char IUFWROLLBACK_SUBDIR[CONFIG_SUBDIR_LEN];
        static char RULE_SUBDIR[CONFIG_SUBDIR_LEN];
        static const uint8_t CONFIG_EXTENSION_LEN = 6;
        static char CONFIG_EXTENSION[CONFIG_EXTENSION_LEN];
        static char FNAME_WIFI0[6];
        static char FNAME_WIFI1[6];
        static char FNAME_WIFI2[6];
        static char FNAME_WIFI3[6];
        static char FNAME_WIFI4[6];
        static char FNAME_FEATURE[9];
        static char FNAME_COMPONENT[11];
        static char FNAME_DEVICE[7];
        static char FNAME_OP_STATE[8];
        static char FNAME_RAW_DATA_ENDPOINT[13];
        static char FNAME_MQTT_SERVER[12];
        static char FNAME_MQTT_CREDS[11];
        static char FNAME_FFT[4];
        static char FNAME_OTA[4];
        static char FNAME_FORCE_OTA[10];
        static char FNAME_MODBUS_SLAVE[12];        
        static const uint8_t MAX_FULL_CONFIG_FPATH_LEN = 28;
        static char FNAME_DIG[11];
        /***** Core *****/
        IUFSFlash() : IUFlash() {}
        ~IUFSFlash() {}
        void begin();
        /***** Config read / write functions *****/
        size_t readConfig(storedConfig configType, char *config,
                          size_t maxLength);
        size_t writeConfig(storedConfig configType, char *config, size_t len);
        virtual bool deleteConfig(storedConfig configType);
        /***** JSON Config load / save functions *****/
        template <size_t capacity>
        JsonObject& loadConfigJson(
            storedConfig configType, StaticJsonBuffer<capacity> &jsonBuffer);
        bool saveConfigJson(storedConfig configType, JsonVariant &config);
        bool updateConfigJson(storedConfig configType, JsonVariant &config);
        bool validateConfig(storedConfig configType, JsonObject &config, char *validationResultString, char* mac_id, double timestamp, char* messageId);
        /***** Utility *****/
        size_t getConfigFilename(storedConfig configType, char *dest,
                                 size_t len);
        File openConfigFile(storedConfig configType, const char* mode);

        void writeInternalFlash(uint8_t type, uint32_t address, uint16_t dataLength, const uint8_t* data);
        String readInternalFlash(uint32_t address);
        bool checkConfig(uint32_t address);
        void clearInternalFlash(uint32_t address);
};

template <size_t capacity>
JsonObject& IUFSFlash::loadConfigJson(
    storedConfig configType, StaticJsonBuffer<capacity> &jsonBuffer)
{
    File file = openConfigFile(configType, "r");
    JsonObject &root = jsonBuffer.parseObject(file);
    file.close();
    return root;
}



/**
 * 1MB Flash Memory, accessible through SPI
 *
 * Component:
 *  Name:
 *    Winbond  W25Q80BLUX1G flash memory
 *  Description:
 *    1MB SPI Flash Memory
 *    Flash id is on 3 bytes: 0xEF, 0x40, 0x14
 *    Memory is organised in:
 *    - pages: Each page contain 256B, ie 2048bits.
 *    - sectors: each sector is 4kB, which is 16 pages
 *    - blocks: 32kB or 64kB blocks (resp 8 or 16 sectors)
 */
class IUSPIFlash : public IUFlash
{
    public:
        /***** Preset values and default settings *****/
        static const uint8_t CSPIN = A1;
        /* Highest page number is:
         - 0xFFFF = 65535 for 16MB = 128Mbit flash
         - 0x0EFF =  4095 for 1MB = 8Mbit flash */
        static const uint16_t MAX_PAGE_NUMBER       = 0x0EFF;
        static const uint8_t STATUS_WIP             = 1;
        static const uint8_t STATUS_WEL             = 2;
        static const uint8_t CMD_WRITE_STATUS_REG   = 0x01;
        static const uint8_t CMD_PAGE_PROGRAM       = 0x02;
        static const uint8_t CMD_READ_DATA          = 0x03;
        static const uint8_t CMD_WRITE_DISABLE      = 0x04;
        static const uint8_t CMD_READ_STATUS_REG    = 0x05;
        static const uint8_t CMD_WRITE_ENABLE       = 0x06;
        static const uint8_t CMD_READ_HIGH_SPEED    = 0x0B;
        static const uint8_t CMD_SECTOR_ERASE       = 0x20;
        static const uint8_t CMD_BLOCK32K_ERASE     = 0x52;
        static const uint8_t CMD_RESET_DEVICE       = 0xF0;
        static const uint8_t CMD_READ_ID            = 0x9F;
        static const uint8_t CMD_RELEASE_POWER_DOWN = 0xAB;
        static const uint8_t CMD_POWER_DOWN         = 0xB9;
        static const uint8_t CMD_CHIP_ERASE         = 0xC7;
        static const uint8_t CMD_BLOCK64K_ERASE     = 0xD8;
        // ID bytes
        static const uint8_t ID_BYTE_COUNT = 3;
        static uint8_t ID_BYTES[ID_BYTE_COUNT];
        // Page group types
        enum pageBlockTypes : uint8_t {SECTOR,
                                       BLOCK_32KB,
                                       BLOCK_64KB};
        // Number of pages per sector or block
        static const uint16_t SECTOR_PAGE_COUNT     = 16;
        static const uint16_t BLOCK_32KB_PAGE_COUNT = 128;
        static const uint16_t BLOCK_64KB_PAGE_COUNT = 256;

        uint16_t configPageDirectory[storedConfig::CFG_COUNT] = {
            5,  // WIFI0,
            10,  // WIFI1,
            15,  // WIFI2,
            20,  // WIFI3,
            25,  // WIFI4,
            30,  // FEATURE,
            35,  // COMPONENT,
            40,  // DEVICE,
            45,  // RAW DATA ENDPOINT,
            50,  // MQTT SERVER,
            55,  // MQTT CREDENTIALS,
        };
        /***** Core *****/
        IUSPIFlash(SPIClass *spiPtr, uint8_t csPin, SPISettings settings);
        ~IUSPIFlash() {}
        void hardReset();
        void begin();
        /***** Config read / write functions *****/
        size_t readConfig(storedConfig configType, char *config,
                          size_t maxLength);
        size_t writeConfig(storedConfig configType, char *config, size_t len);
        bool deleteConfig(storedConfig configType);
        /***** JSON Config load / save functions *****/
        template <size_t capacity>
        JsonObject& loadConfigJson(
            storedConfig configType, StaticJsonBuffer<capacity> &jsonBuffer);
        bool saveConfigJson(storedConfig configType, JsonVariant &config);

    protected:
        SPIClass *m_SPI;
        uint8_t m_csPin;
        SPISettings m_spiSettings;
        bool m_busy;
        /***** SPI utilities *****/
        void beginTransaction(bool waitIfBusy=true);
        void endTransaction(bool waitForCompletion=false);
        uint8_t readStatus();
        void waitForAvailability();
        /***** Adress utilities *****/
        uint16_t getBlockIndex(pageBlockTypes blockType, uint16_t pageIndex);
        uint16_t getBlockFirstPage(pageBlockTypes blockType,
                                   uint16_t blockIndex);
        uint32_t getAddressFromPage(uint16_t pageIndex);
        uint16_t getPageFromAddress(uint32_t addr);
        /***** Read / write functions *****/
        void readId(uint8_t *destination, uint8_t destinationCount);
        void eraseChip(bool wait);
        void erasePages(pageBlockTypes blockType, uint16_t pageIndex);
        void programPage(uint8_t *content, uint16_t pageIndex);
        void readPages(uint8_t *content, uint16_t pageIndex,
                       const uint16_t pageCount, bool highSpeed);
};

template <size_t capacity>
JsonObject& IUSPIFlash::loadConfigJson(
    storedConfig configType, StaticJsonBuffer<capacity> &jsonBuffer)
{
    //TODO Implement
    return jsonBuffer.parseObject("");
}


#endif // IUFLASH_H
