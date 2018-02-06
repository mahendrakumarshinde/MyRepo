#include "IUSPIFlash.h"


/* =============================================================================
    Constructors and destructors
============================================================================= */

uint8_t IUSPIFlash::ID_BYTES[IUSPIFlash::ID_BYTE_COUNT] = {0xEF, 0x40, 0x14};

IUSPIFlash::IUSPIFlash(uint8_t placeHolder) :
    m_busy(false)
{
    //ctor
}

/**
 * Set up the component and finalize the object initialization
 */
void IUSPIFlash::begin()
{
    pinMode(CSPIN, OUTPUT);
    digitalWrite(CSPIN, HIGH);
    SPI.begin();
}

/**
 * Component hard reset
 *
 * The spec says "the device will take approximately tRST=30 microseconds to
 * reset.
 */
void IUSPIFlash::hardReset()
{
    SPI.beginTransaction(SPISettings(50000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(CMD_RESET_DEVICE );
    endTransaction();
    delayMicroseconds(50);
    while(readStatus() & STATUS_WIP);
    {
        // Wait for the hard reset to finish
    }
}


/* =============================================================================
    Utility function
============================================================================= */

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


/* =============================================================================
    Read / write functions
============================================================================= */

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
    SPI.transfer(CMD_READ_ID);
    for(uint8_t i = 0; i < destinationCount; ++i)
    {
        destination[i] = SPI.transfer(0x00);
    }
    endTransaction();
}

/**
 * Erase the whole flash memory chip
 */
void IUSPIFlash::eraseChip(bool wait)
{
    beginTransaction();
    SPI.transfer(CMD_WRITE_ENABLE);
    digitalWrite(CSPIN, HIGH);
    digitalWrite(CSPIN, LOW);
    SPI.transfer(CMD_CHIP_ERASE);
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
    SPI.transfer(CMD_WRITE_ENABLE);
    digitalWrite(CSPIN, HIGH);
    digitalWrite(CSPIN, LOW);
    SPI.transfer(eraseCommand);
    // Send the 3 byte address
    SPI.transfer((address >> 16) & 0xff);
    SPI.transfer((address >> 8) & 0xff);
    SPI.transfer(address & 0xff);
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
    SPI.transfer(CMD_WRITE_ENABLE);
    endTransaction();
    // Program the page
    beginTransaction(false);
    SPI.transfer(CMD_PAGE_PROGRAM);
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
    for(uint16_t i = 0; i < 256; i++)
    {
        SPI.transfer(content[i]);
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
        SPI.transfer(CMD_READ_HIGH_SPEED);
    }
    else
    {
        SPI.transfer(CMD_READ_DATA);
    }
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
    if (highSpeed)
    {
        SPI.transfer(0);  // send dummy byte
    }
    uint32_t byteCount = (uint32_t) pageCount * 256;
    for(uint32_t i = 0; i < byteCount; ++i) {
        content[i] = SPI.transfer(0);
    }
    endTransaction();
}


/* =============================================================================
    Internal utility functions
============================================================================= */

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
    SPI.beginTransaction(SPISettings(50000000, MSBFIRST, SPI_MODE0));
    digitalWrite(CSPIN, LOW);
}

/**
 * Unselect the SPI slave (SS/CS pin)  and end the SPI transaction
 *
 * CS PIN is set to HIGH to unselect the flash memory chip.
 */
 void IUSPIFlash::endTransaction(bool waitForCompletion)
{
    digitalWrite(CSPIN, HIGH);
    SPI.endTransaction();
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
    SPI.transfer(CMD_READ_STATUS_REG);
    c = SPI.transfer(0x00);
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
