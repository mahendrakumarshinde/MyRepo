#include "IUFlash.h"
#include "FFTConfiguration.h"

/* =============================================================================
    IUFSFlash - Flash with file system
============================================================================= */

char IUFSFlash::CONFIG_SUBDIR[IUFSFlash::CONFIG_SUBDIR_LEN] = "/iuconfig";
char IUFSFlash::IUFWBACKUP_SUBDIR[IUFSFlash::CONFIG_SUBDIR_LEN] = "/iuBackupFirmware";
char IUFSFlash::IUFWTMPIMG_SUBDIR[IUFSFlash::CONFIG_SUBDIR_LEN]  = "/iuTempFirmware";
char IUFSFlash::IUFWROLLBACK_SUBDIR[IUFSFlash::CONFIG_SUBDIR_LEN] = "/iuRollbackFirmware";
char IUFSFlash::CONFIG_EXTENSION[IUFSFlash::CONFIG_EXTENSION_LEN] = ".conf";

char IUFSFlash::FNAME_WIFI0[6] = "wifi0";
char IUFSFlash::FNAME_WIFI1[6] = "wifi1";
char IUFSFlash::FNAME_WIFI2[6] = "wifi2";
char IUFSFlash::FNAME_WIFI3[6] = "wifi3";
char IUFSFlash::FNAME_WIFI4[6] = "wifi4";
char IUFSFlash::FNAME_FEATURE[9] = "features";
char IUFSFlash::FNAME_COMPONENT[11] = "components";
char IUFSFlash::FNAME_DEVICE[7] = "device";
char IUFSFlash::FNAME_OP_STATE[8] = "opState";
char IUFSFlash::FNAME_RAW_DATA_ENDPOINT[13] = "fft_endpoint";
char IUFSFlash::FNAME_MQTT_SERVER[12] = "mqtt_server";
char IUFSFlash::FNAME_MQTT_CREDS[11] = "mqtt_creds";
char IUFSFlash::FNAME_FFT[4] = "fft";
char IUFSFlash::FNAME_OTA[4] = "ota";
char IUFSFlash::FNAME_FORCE_OTA[10] = "force_ota";
/***** Core *****/

void IUFSFlash::begin()
{
    if (!DOSFS.begin())
    {
        if (setupDebugMode)
        {
            debugPrint("Failed to start DOSFS (DOSFS.begin returned false)");
        }
    }
    m_begun = DOSFS.exists(CONFIG_SUBDIR);
    if (!m_begun)
    {
        DOSFS.mkdir(CONFIG_SUBDIR);
        m_begun = DOSFS.exists(CONFIG_SUBDIR);
        if (!m_begun && setupDebugMode)
        {
            debugPrint("Unable to find or create the config directory");
        }
    }
    m_otaDir = DOSFS.exists(IUFWBACKUP_SUBDIR);
    if (!m_otaDir)
    {
        DOSFS.mkdir(IUFWBACKUP_SUBDIR);
        m_otaDir = DOSFS.exists(IUFWBACKUP_SUBDIR);
        if (!m_otaDir && setupDebugMode)
        {
            debugPrint("Unable to find/create the ota_mainbkup directory");
        }
    }
    m_otaDir = DOSFS.exists(IUFWTMPIMG_SUBDIR);
    if (!m_otaDir)
    {
        DOSFS.mkdir(IUFWTMPIMG_SUBDIR);
        m_otaDir = DOSFS.exists(IUFWTMPIMG_SUBDIR);
        if (!m_otaDir && setupDebugMode)
        {
            debugPrint("Unable to find or create the ota_mainimage directory");
        }
    }
    m_otaDir = DOSFS.exists(IUFWROLLBACK_SUBDIR);
    if (!m_otaDir)
    {
        DOSFS.mkdir(IUFWROLLBACK_SUBDIR);
        m_otaDir = DOSFS.exists(IUFWROLLBACK_SUBDIR);
        if (!m_otaDir && setupDebugMode)
        {
            debugPrint("Unable to find or create the ota_rollback directory");
        }
    }
}


/***** Config read / write functions *****/

/**
 * Read the relevant file to retrieve the stored config.
 *
 * Return the number of read chars. Returns 0 if the stored config type is
 * unknown or if the file is not found.
 */
size_t IUFSFlash::readConfig(storedConfig configType, char *config,
                             size_t maxLength)
{
    File file = openConfigFile(configType, "r");
    size_t readCharCount = 0;
    if (!file) {
        strcpy(config, "");
    }
    else {
        readCharCount = file.readBytes(config, maxLength);
    }
    file.close();
    return readCharCount;
}

/**
 * Store the config in flash by writing the config to the relevant file.
 *
 * Return the number of written chars. Returns 0 if the stored config type is
 * unknown.
 */
size_t IUFSFlash::writeConfig(storedConfig configType, char *config, size_t len)
{
    File file = openConfigFile(configType, "w");
    size_t writtenCharCount = 0;
    if (file)
    {
        writtenCharCount = file.write((uint8_t*) config, len);
    }
    file.close();
    return writtenCharCount;
}

/**
 * Delete the config.
 */
bool IUFSFlash::deleteConfig(storedConfig configType)
{
    if (!available()) {
        return false;
    }
    char filepath[MAX_FULL_CONFIG_FPATH_LEN];
    getConfigFilename(configType, filepath, MAX_FULL_CONFIG_FPATH_LEN);
    if (DOSFS.exists(filepath)) {
        return DOSFS.remove(filepath);
    } else {
        return true;
    }
}

/***** JSON Config load / save functions *****/

bool IUFSFlash::saveConfigJson(storedConfig configType, JsonVariant &config)
{
    if (!available()) {
        return false;
    }
    File file = openConfigFile(configType, "w");
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
/*
    Following method only updates the fields received in the config message.
    It will not overwrite the previous json string, only add/update the fields
    received in the config message.
 */
bool IUFSFlash::updateConfigJson(storedConfig configType, JsonVariant &config)
{
    if (!available()) {
        return false;
    }
    StaticJsonBuffer<300> storedConfigJsonBuffer;
    const int storedConfigMaxLength = 300;
    char storedConfigJsonString[storedConfigMaxLength];
    int charCount;
    strcpy(storedConfigJsonString, "");
    charCount = readConfig(configType, storedConfigJsonString, storedConfigMaxLength);    
    if (charCount == 0) { strcpy(storedConfigJsonString, "{}"); }
    //storedConfigJsonString[charCount] = '\0'; //DOES NOT WORK, json is created as {}, however no fields can be added in this json
    JsonObject& storedConfigJson = storedConfigJsonBuffer.parse(storedConfigJsonString);
    for(auto kv:(JsonObject&)config) {
        storedConfigJson[kv.key] = kv.value;
    }
    File file = openConfigFile(configType, "w");
    if (storedConfigJson.printTo(file) == 0)
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

bool IUFSFlash::validateConfig(storedConfig configType, JsonObject &config, char* validationResultString, char* mac_id, double timestamp, char* messageId)
{
    // Perform validation checks on the config json
    // Return the errors in a json object
    bool validConfig = true;
    StaticJsonBuffer<400> validationResultBuffer;
    JsonObject& validationResult = validationResultBuffer.createObject();
    JsonArray& errorMessages = validationResult.createNestedArray("errorMessages");

    switch(configType) {
        case CFG_DEVICE: {
            // Configuration is valid if there is atleast one correct key           

            // Indicate the type of validation
            validationResult["messageType"] = "device-config-ack";

            // Save the updated params
            JsonArray& validParams = validationResult.createNestedArray("validParams");

            // Validation for "DSP" parameter
            if(config.containsKey("DSP")) {
                int newDataSendPeriod = config["DSP"];
                if(newDataSendPeriod < 200) { // DSP cannot be less than 200 milliseconds
                    errorMessages.add("DSP < 200 ms");
                    config.remove("DSP");
                } else {
                    validParams.add("DSP");
                }
            }

            // Validation for "RAW parameter"
            if(config.containsKey("RAW")) {
                int newRawDataPublishPeriod = config["RAW"];
                if(newRawDataPublishPeriod < 30) { // RAW cannot be less than 10 seconds
                    errorMessages.add("RAW < 30 seconds");
                    config.remove("RAW");
                } else {
                    validParams.add("RAW");
                }
            }

            // Device configuration message : {"main":{"GRP":["MOTSTD"],"RAW":1800,"POW":0,"TSL":60,"TOFF":10,"TCY":20,"DSP":512}}
            // TODO: Validation should be added for these keys (GRP, POW, TSL, TOFF, TCY etc.) as well, right now these keys
            // are assumed to be correct and are being added without any validation checks. After adding validation checks, 
            // if the parameter is incorrect it should be removed from "config". The following logic is added as a temporary workaround, 
            // it ensures that the parameters which do not have validation checks will be considered to be valid.
            /* WORKAROUND FOR PARAMS WITHOUT VALIDATION CHECKS*/
            const int numberOfParametersWithValidationChecks = 2;
            const int parameterNameLength = 4; 
            // The following list contains the parameters which have validation checks
            char parametersWithValidationChecks[2][4] = {"DSP", "RAW"};
            // All parameters in "config" excluding parametersWithValidationChecks are to be added to the list of valid params
            for (auto kv:config) {
                bool parameterWithValidationCheck = false;
                for (int i=0; i<numberOfParametersWithValidationChecks; ++i) {
                    if (strcmp(kv.key, parametersWithValidationChecks[i]) == 0) {
                        parameterWithValidationCheck = true;
                        break;
                    }
                }
                if(!parameterWithValidationCheck) {
                    validParams.add(kv.key);
                }
            }   
            /* THIS WORKAROUND CAN BE REMOVED AFTER VALIDATION CHECKS ARE ADDED FOR ALL PARAMETERS */         

            if (validParams.size() == 0) {
                validConfig = false;
            }
            break;
        }
        case CFG_FFT: {
            // Configuration is valid only if both samplingRate and blockSize are present
            // and both are within their acceptable range of values
            validConfig = true;

            // Indicate the type of validation
            validationResult["messageType"] = "fft-config-ack";

            // If the received config matches the current config, report an error
            bool sameBlockSize = false;
            bool sameSamplingRate = false;

            //Validation for samplingRate field
            if(config.containsKey("samplingRate")) {
                uint16_t samplingRate = config["samplingRate"];
                // Validation for samplingRate
                bool validSamplingRate = true;
                for(int i=0; i<FFTConfiguration::samplingRateConfigurations - 1; ++i) {
                    if( samplingRate < FFTConfiguration::samplingRates[0] || 
                        samplingRate > FFTConfiguration::samplingRates[FFTConfiguration::samplingRateConfigurations - 1] || 
                        (FFTConfiguration::samplingRates[i] < samplingRate &&
                         samplingRate < FFTConfiguration::samplingRates[i+1]) ) {
                           validSamplingRate = false;
                       }
                }
                if (!validSamplingRate) {
                        validConfig = false;
                        errorMessages.add("Invalid samplingRate");
                } else if (samplingRate == 416) {  // Temporary workaround
                    validConfig = false;
                    errorMessages.add("Sampling rate not supported");
                } else if (FFTConfiguration::currentSamplingRate == samplingRate) {
                    sameSamplingRate = true;
                }
            } else {
                validConfig = false;
                errorMessages.add("Key missing: samplingRate"); 
            }
         
            // Validation for blockSize field
            if(config.containsKey("blockSize")) {
                uint16_t blockSize = config["blockSize"];
                // Validation for samplingRate
                bool validBlockSize = true;
                for(int i=0; i<FFTConfiguration::blockSizeConfigurations; i++) {
                    if( blockSize < FFTConfiguration::blockSizes[0] ||
                        blockSize > FFTConfiguration::blockSizes[FFTConfiguration::blockSizeConfigurations - 1] ||
                        (FFTConfiguration::blockSizes[i] < blockSize &&
                        blockSize < FFTConfiguration::blockSizes[i+1]) ) {
                            validBlockSize = false;
                            break;
                        }
                }
                if (!validBlockSize) {
                        validConfig = false;
                        errorMessages.add("Invalid blockSize");
                } else if (FFTConfiguration::currentBlockSize == blockSize) {
                    sameBlockSize = true;
                }
            } else {
                validConfig = false;
                errorMessages.add("Key missing: blockSize"); 
            }

            // If the received config matches the current config, report an error
            if(sameBlockSize && sameSamplingRate) {
                validConfig = false;
                errorMessages.add("Same configuration received");
            }
            break;
        }
    }

    // Construct the validationResult
    validationResult["messageId"] = messageId;
    validationResult["validConfig"] = validConfig;
    validationResult["mac"] = mac_id;
    validationResult["timestamp"] = timestamp;
    validationResult.printTo(validationResultString, 300);
    return validConfig;
}

/***** Utility *****/

/**
 * Find and return the file name of the requested config type.
 *
 * Dest char array pointer should at least be of length
 * MAX_FULL_CONFIG_FPATH_LEN.
 */
size_t IUFSFlash::getConfigFilename(storedConfig configType, char *dest,
                                    size_t len)
{
    char *fname = NULL;
    switch (configType)
    {
        case CFG_WIFI0:
            fname = FNAME_WIFI0;
            break;
        case CFG_WIFI1:
            fname = FNAME_WIFI1;
            break;
        case CFG_WIFI2:
            fname = FNAME_WIFI2;
            break;
        case CFG_WIFI3:
            fname = FNAME_WIFI3;
            break;
        case CFG_WIFI4:
            fname = FNAME_WIFI4;
            break;
        case CFG_FEATURE:
            fname = FNAME_FEATURE;
            break;
        case CFG_COMPONENT:
            fname = FNAME_COMPONENT;
            break;
        case CFG_DEVICE:
            fname = FNAME_DEVICE;
            break;
        case CFG_OP_STATE:
            fname = FNAME_OP_STATE;
            break;
        case CFG_RAW_DATA_ENDPOINT:
            fname = FNAME_RAW_DATA_ENDPOINT;
            break;
        case CFG_MQTT_SERVER:
            fname = FNAME_MQTT_SERVER;
            break;
        case CFG_MQTT_CREDS:
            fname = FNAME_MQTT_CREDS;
            break;
        case CFG_FFT:
            fname = FNAME_FFT;
            break;
        case CFG_OTA:
            fname = FNAME_OTA;
            break;
        case CFG_FORCE_OTA:
            fname = FNAME_FORCE_OTA;
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
                    CONFIG_EXTENSION);
}

File IUFSFlash::openConfigFile(storedConfig configType,
                                       const char* mode)
{
    if (!available())
    {
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
    return DOSFS.open(filepath, mode);
}


/* =============================================================================
    IUSPIFlash - Flash accessible via SPI
============================================================================= */

/***** Core *****/

uint8_t IUSPIFlash::ID_BYTES[IUSPIFlash::ID_BYTE_COUNT] = {0xEF, 0x40, 0x14};

IUSPIFlash::IUSPIFlash(SPIClass *spiPtr, uint8_t csPin, SPISettings settings) :
    IUFlash(),
    m_csPin(csPin),
    m_spiSettings(settings),
    m_busy(false)
{
    m_SPI = spiPtr;
}

/**
 * Component hard reset
 *
 * The spec says "the device will take approximately tRST=30 microseconds to
 * reset.
 */
void IUSPIFlash::hardReset()
{
    beginTransaction();
    m_SPI->write(CMD_RESET_DEVICE);
    endTransaction();
    delayMicroseconds(50);
    while(readStatus() & STATUS_WIP);
    {
        // Wait for the hard reset to finish
    }
}

/**
 *
 */
void IUSPIFlash::begin()
{
    pinMode(m_csPin, OUTPUT);
    digitalWrite(m_csPin, HIGH);
    m_SPI->begin();
    m_begun = true;
}


/***** Config read / write functions *****/

/**
 *
 */
size_t IUSPIFlash::readConfig(storedConfig configType, char *config,
                              size_t maxLength)
{
    // TODO Implement
    return 0;
}

/**
 *
 */
size_t IUSPIFlash::writeConfig(storedConfig configType, char *config,
                               size_t len)
{
    // TODO Implement
    return 0;
}

/**
 * Delete the config.
 */
bool IUSPIFlash::deleteConfig(storedConfig configType)
{
    // TODO Implement
    return false;
}


/***** JSON Config load / save functions *****/

bool IUSPIFlash::saveConfigJson(storedConfig configType, JsonVariant &config)
{
    // TODO Implement
    return false;
}


/***** SPI utilities *****/

/**
 * Begin the SPI transaction and select the flash memory chip (with SS/CS pin)
 *
 * CS pin is set to LOW to select the flash memory chip.
 */
void IUSPIFlash::beginTransaction(bool waitIfBusy)
{
    if (waitIfBusy)
    {
        waitForAvailability();
    }
    m_SPI->beginTransaction(m_spiSettings);
    digitalWrite(m_csPin, LOW);
}

/**
 * Unselect the SPI slave (SS/CS pin)  and end the SPI transaction
 *
 * CS PIN is set to HIGH to unselect the flash memory chip.
 */
 void IUSPIFlash::endTransaction(bool waitForCompletion)
{
    digitalWrite(m_csPin, HIGH);
    m_SPI->endTransaction();
    if (waitForCompletion)
    {
        waitForAvailability();
    }
}

/**
 * Read and return the flash status (0 if available, else busy)
 */
uint8_t IUSPIFlash::readStatus()
{
    uint8_t c;
    beginTransaction(false);  // !!! Don't wait if busy !!!
    m_SPI->transfer(CMD_READ_STATUS_REG);
    c = m_SPI->transfer(0x00);
    endTransaction();
    return(c);
}

/**
 * Function returns only when the SPI Flash is available (holds the process)
 */
void IUSPIFlash::waitForAvailability()
{
    if(m_busy) {
        while(readStatus() & STATUS_WIP) {
            // wait
        }
        m_busy = false;
    }
}


/***** Address Utilities *****/

/**
 * Return the index of the sector / block containing the given page
 */
uint16_t IUSPIFlash::getBlockIndex(pageBlockTypes blockType,
                                   uint16_t pageIndex)
{
    switch (blockType)
    {
        case SECTOR:
            return pageIndex / SECTOR_PAGE_COUNT;
        case BLOCK_32KB:
            return pageIndex / BLOCK_32KB_PAGE_COUNT;
        case BLOCK_64KB:
            return pageIndex / BLOCK_64KB_PAGE_COUNT;
        default:
            if (debugMode) {
                debugPrint(F("Unknown sector / block type"));
            }
            return -1;
    }
}

uint16_t IUSPIFlash::getBlockFirstPage(pageBlockTypes blockType,
                                       uint16_t blockIndex)
{
    switch (blockType)
    {
        case SECTOR:
            return blockIndex * SECTOR_PAGE_COUNT;
        case BLOCK_32KB:
            return blockIndex * BLOCK_32KB_PAGE_COUNT;
        case BLOCK_64KB:
            return blockIndex * BLOCK_64KB_PAGE_COUNT;
        default:
            if (debugMode) {
                debugPrint(F("Unknown sector / block type"));
            }
            return -1;
    }
}

/**
 * Convert a page number to a 24-bit address
 *
 * @param pageIndex The page number
 */
uint32_t IUSPIFlash::getAddressFromPage(uint16_t pageIndex)
{
  return ((uint32_t) pageIndex) << 8;
}

/**
 * Convert a 24-bit address to a page number
 *
 * @param addr The address
 */
uint16_t IUSPIFlash::getPageFromAddress(uint32_t addr)
{
  return (uint16_t) (addr >> 8);
}


/***** Read / write functions *****/

/**
 * Read the flash memory chip ID
 *
 * @param destination A destination unsigned char array, to receive the read ID
 * @param destinationCount The length of destination (= the expected length of
 *  the ID), usually 3.
 */
void IUSPIFlash::readId(uint8_t *destination, uint8_t destinationCount)
{
    beginTransaction();
    m_SPI->transfer(CMD_READ_ID);
    for(uint8_t i = 0; i < destinationCount; ++i)
    {
        destination[i] = m_SPI->transfer(0x00);
    }
    endTransaction();
}

/**
 * Erase the whole flash memory chip
 */
void IUSPIFlash::eraseChip(bool wait)
{
    beginTransaction();
    m_SPI->transfer(CMD_WRITE_ENABLE);
    digitalWrite(CSPIN, HIGH);
    digitalWrite(CSPIN, LOW);
    m_SPI->transfer(CMD_CHIP_ERASE);
    endTransaction();
    m_busy = true;
    if (wait)
    {
        waitForAvailability();
    }
}

/**
 * Erase all the pages in the sector / block containing given page number
 *
 * Tse Typ=0.6sec Max=3sec, measured at measured 549.024ms for SECTOR.
 * The smallest unit of memory which can be erased is the 4kB sector
 * (which is 16 pages).
 *
 * @param blockType The type of page block to erase (pages can only be erased by
 *  sector or block)
 * @param pageIndex The page number
 */
void IUSPIFlash::erasePages(IUSPIFlash::pageBlockTypes blockType,
                            uint16_t pageIndex)
{
    uint8_t eraseCommand;
    switch (blockType) {
        case SECTOR:
            eraseCommand = CMD_SECTOR_ERASE;
            break;
        case BLOCK_32KB:
            eraseCommand = CMD_BLOCK32K_ERASE;
            break;
        case BLOCK_64KB:
            eraseCommand = CMD_BLOCK64K_ERASE;
            break;
        default:
            if (debugMode) {
                debugPrint(F("Unknown sector / block type"));
            }
            return;
    }
    uint32_t address = getAddressFromPage(pageIndex);
    beginTransaction();
    m_SPI->transfer(CMD_WRITE_ENABLE);
    digitalWrite(CSPIN, HIGH);
    digitalWrite(CSPIN, LOW);
    m_SPI->transfer(eraseCommand);
    // Send the 3 byte address
    m_SPI->transfer((address >> 16) & 0xff);
    m_SPI->transfer((address >> 8) & 0xff);
    m_SPI->transfer(address & 0xff);
    endTransaction();
    m_busy = true;
}

/**
 * Program (write) the given content in a page
 *
 * NB: To program a page, it must have be erased beforehand.
 * A page contains 256 bytes, ie 256 chars or uint8_t.
 *
 * @param content A pointer to a byte array of length 256
 * @param pageIndex The page number
 */
void IUSPIFlash::programPage(uint8_t *content, uint16_t pageIndex)
{
    uint32_t address = getAddressFromPage(pageIndex);
    // Enable write
    beginTransaction();
    m_SPI->transfer(CMD_WRITE_ENABLE);
    endTransaction();
    // Program the page
    beginTransaction(false);
    m_SPI->transfer(CMD_PAGE_PROGRAM);
    m_SPI->transfer((address >> 16) & 0xFF);
    m_SPI->transfer((address >> 8) & 0xFF);
    m_SPI->transfer(address & 0xFF);
    for(uint16_t i = 0; i < 256; i++)
    {
        m_SPI->transfer(content[i]);
    }
    endTransaction();
    m_busy = true;
}

/**
 * Read one or several successive page(s) into given content byte array
 *
 * A page contains 256 bytes, ie 256 chars or uint8_t.
 *
 * @param content A pointer to a byte array, to receive the
 *  content of the read pages. Its length should be pageCount * 256.
 * @param pageIndex The page number
 * @param pageCount The number of successive pages to read
 * @param highSpeed If true, function will use the high speed read flash
 *  command, else will use the default speed read flash command.
 */
void IUSPIFlash::readPages(uint8_t *content, uint16_t pageIndex,
                           const uint16_t pageCount, bool highSpeed)
{
    uint32_t address = getAddressFromPage(pageIndex);
    beginTransaction();
    if (highSpeed)
    {
        m_SPI->transfer(CMD_READ_HIGH_SPEED);
    }
    else
    {
        m_SPI->transfer(CMD_READ_DATA);
    }
    m_SPI->transfer((address >> 16) & 0xFF);
    m_SPI->transfer((address >> 8) & 0xFF);
    m_SPI->transfer(address & 0xFF);
    if (highSpeed)
    {
        m_SPI->transfer(0);  // send dummy byte
    }
    uint32_t byteCount = (uint32_t) pageCount * 256;
    for(uint32_t i = 0; i < byteCount; ++i) {
        content[i] = m_SPI->transfer(0);
    }
    endTransaction();
}

