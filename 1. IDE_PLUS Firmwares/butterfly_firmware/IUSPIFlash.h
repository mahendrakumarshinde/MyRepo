#ifndef IUSPIFLASH_H
#define IUSPIFLASH_H

#include <Arduino.h>
#include <SPI.h>

#include "Logger.h"


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
class IUSPIFlash
{
    public:
        /***** Preset values and default settings *****/
        static const uint8_t CSPIN = A1;
        /* Highest page number is:
         - 0xFFFF = 65535 for 16MB = 128Mbit flash
         - 0x0EFF =  4095 for 1MB = 8Mbit flash */
        static const uint16_t MAX_PAGE_NUMBER = 0x0EFF;
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
        /***** Constructors & desctructors *****/
        IUSPIFlash(SPIClass *spiPtr, uint8_t csPin, SPISettings settings);
        virtual ~IUSPIFlash() { }
        void begin();
        void hardReset();
        /***** Utilities *****/
        uint16_t getBlockIndex(pageBlockTypes blockType, uint16_t pageIndex);
        uint16_t getBlockFirstPage(pageBlockTypes blockType,
                                   uint16_t blockIndex);
        /***** Read / write functions *****/
        void readId(uint8_t *destination, uint8_t destinationCount);
        void eraseChip(bool wait);
        void erasePages(pageBlockTypes blockType, uint16_t pageIndex);
        void programPage(uint8_t *content, uint16_t pageIndex);
        void readPages(uint8_t *content, uint16_t pageIndex,
                       const uint16_t pageCount, bool highSpeed);

    protected:
        SPIClass *m_SPI;
        uint8_t m_csPin;
        SPISettings m_spiSettings;
        bool m_busy;
        /***** Internal SPI utility functions *****/
        void beginTransaction(bool waitIfBusy=true);
        void endTransaction(bool waitForCompletion=false);
        uint8_t readStatus();
        void waitForAvailability();
        uint32_t getAddressFromPage(uint16_t pageIndex);
        uint16_t getPageFromAddress(uint32_t addr);
};

#endif // IUSPIFLASH_H
