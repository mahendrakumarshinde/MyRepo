#include "IUFlash.h"


/* =============================================================================
    IUFSFlash - Flash with file system
============================================================================= */

char IUFSFlash::CONFIG_SUBDIR[IUFSFlash::CONFIG_SUBDIR_LEN] = "/iuconfig";
char IUFSFlash::CONFIG_EXTENSION[IUFSFlash::CONFIG_EXTENSION_LEN] = ".conf";

char IUFSFlash::FNAME_WIFI0[6] = "wifi0";
char IUFSFlash::FNAME_WIFI1[6] = "wifi1";
char IUFSFlash::FNAME_WIFI2[6] = "wifi2";
char IUFSFlash::FNAME_WIFI3[6] = "wifi3";
char IUFSFlash::FNAME_WIFI4[6] = "wifi4";
char IUFSFlash::FNAME_FEATURE[9] = "features";
char IUFSFlash::FNAME_COMPONENT[11] = "components";
char IUFSFlash::FNAME_DEVICE[7] = "device";
char IUFSFlash::FNAME_RAW_DATA_ENDPOINT[13] = "fft_endpoint";
char IUFSFlash::FNAME_MQTT_SERVER[12] = "mqtt_server";
char IUFSFlash::FNAME_MQTT_CREDS[11] = "mqtt_creds";


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
        bool mkdir(const char* path);
        m_begun = DOSFS.exists(CONFIG_SUBDIR);
        if (!m_begun && setupDebugMode)
        {
            debugPrint("Unable to find or create the config directory");
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
    if (!m_begun)
    {
        return 0;
    }
    char filepath[MAX_FULL_CONFIG_FPATH_LEN];
    getConfigFilename(configType, filepath);
    if (!DOSFS.exists(filepath))
    {
        return 0;
    }
    File file = DOSFS.open(filepath, "r");
    size_t readCharCount = file.readBytes(config, maxLength);
    file.close();
    return readCharCount;
}

/**
 * Store the config in flash by writing the config to the relevant file.
 *
 * Return the number of written chars. Returns 0 if the stored config type is
 * unknown.
 */
size_t IUFSFlash::writeConfig(storedConfig configType, char *config)
{
    if (!m_begun)
    {
        return 0;
    }
    char filepath[MAX_FULL_CONFIG_FPATH_LEN];
    getConfigFilename(configType, filepath);
    File file = DOSFS.open(filepath, "w");
    size_t configLength = strlen(config);
    size_t writtenCharCount = file.write((uint8_t*) config, configLength);
    file.close();
    return writtenCharCount;
}

/**
 * Delete the config.
 */
void IUFSFlash::deleteConfig(storedConfig configType)
{
    if (!m_begun)
    {
        return;
    }
    char filepath[MAX_FULL_CONFIG_FPATH_LEN];
    getConfigFilename(configType, filepath);
    if (DOSFS.exists(filepath))
    {
        DOSFS.remove(filepath);
    }
}

/**
 * Update the pointer with a Print subclass instance (writable only).
 */
bool IUFSFlash::getWritable(storedConfig configType, Print* printPtr)
{
    if (!m_begun)
    {
        printPtr = NULL;
        return false;
    }
    char filepath[MAX_FULL_CONFIG_FPATH_LEN];
    getConfigFilename(configType, filepath);
    *printPtr = DOSFS.open(filepath, "w");
    return (printPtr != NULL);
}


/**
 * Update the pointer with a Stream subclass instance (readable only).
 */
bool IUFSFlash::getReadable(storedConfig configType, Stream* streamPtr)
{
    if (!m_begun)
    {
        streamPtr = NULL;
        return false;
    }
    char filepath[MAX_FULL_CONFIG_FPATH_LEN];
    getConfigFilename(configType, filepath);
    if (!DOSFS.exists(filepath))
    {
        streamPtr = NULL;
        return false;
    }
    *streamPtr = DOSFS.open(filepath, "r");
    return (streamPtr != NULL);
}

/**
 * Find and return the file name of the requested config type.
 *
 * Dest char array pointer should at least be of length
 * MAX_FULL_CONFIG_FPATH_LEN.
 */
size_t IUFSFlash::getConfigFilename(storedConfig configType, char *dest)
{
    // Reinitialize dest (required for strcat)
    for (uint8_t i = 0; i < MAX_FULL_CONFIG_FPATH_LEN; ++i)
    {
        dest[i] = 0;
    }
    strcpy(dest, CONFIG_SUBDIR);
    strcat(dest, "/");
    switch (configType)
    {
        case CFG_WIFI0:
            strcat(dest, FNAME_WIFI0);
            break;
        case CFG_WIFI1:
            strcat(dest, FNAME_WIFI1);
            break;
        case CFG_WIFI2:
            strcat(dest, FNAME_WIFI2);
            break;
        case CFG_WIFI3:
            strcat(dest, FNAME_WIFI3);
            break;
        case CFG_WIFI4:
            strcat(dest, FNAME_WIFI4);
            break;
        case CFG_FEATURE:
            strcat(dest, FNAME_FEATURE);
            break;
        case CFG_COMPONENT:
            strcat(dest, FNAME_COMPONENT);
            break;
        case CFG_DEVICE:
            strcat(dest, FNAME_DEVICE);
            break;
        case CFG_RAW_DATA_ENDPOINT:
            strcat(dest, FNAME_RAW_DATA_ENDPOINT);
            break;
        case CFG_MQTT_SERVER:
            strcat(dest, FNAME_MQTT_SERVER);
            break;
        case CFG_MQTT_CREDS:
            strcat(dest, FNAME_MQTT_CREDS);
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unknown config type "), false);
                debugPrint(configType);
            }
            return 0;
    }
    strcat(dest, CONFIG_EXTENSION);
    return strlen(dest);
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
size_t IUSPIFlash::writeConfig(storedConfig configType, char *config)
{
    // TODO Implement
    return 0;
}

/**
 * Delete the config.
 */
void IUSPIFlash::deleteConfig(storedConfig configType)
{
    // TODO Implement
}

/**
 * Update the pointer with a Print subclass instance (writable only).
 */
bool IUSPIFlash::getWritable(storedConfig configType, Print* printPtr)
{
    // TODO Implement
    printPtr = NULL;
    return false;
}

/**
 * Update the pointer with a Stream subclass instance (writable and readable).
 */
bool IUSPIFlash::getReadable(storedConfig configType, Stream* streamPtr)
{
    // TODO Implement
    streamPtr = NULL;
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
    if(m_busy)
    {
        while(readStatus() & STATUS_WIP)
        {
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
            if (debugMode)
            {
                raiseException(F("Unknown sector / block type"));
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
            if (debugMode)
            {
                raiseException(F("Unknown sector / block type"));
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
    switch (blockType)
    {
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
            if (debugMode)
            {
                raiseException(F("Unknown sector / block type"));
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

