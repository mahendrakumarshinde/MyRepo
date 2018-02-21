#ifndef CMP_IUSPIFLASH_H
#define CMP_IUSPIFLASH_H

#include <ArduinoUnit.h>
#include "../IUFlash.h"


test(IUSPIFlash__initialization)
{
    iuFlash.setupHardware();
    assertEqual(iuFlash.getPowerMode(), PowerMode::ACTIVE);

    /***** Chip ID check *****/
    uint8_t flashID[IUSPIFlash::ID_BYTE_COUNT];
    iuFlash.readId(flashID, IUSPIFlash::ID_BYTE_COUNT);
    for (uint8_t i = 0; i < IUSPIFlash::ID_BYTE_COUNT; ++i)
    {
        assertEqual(flashID[i], IUSPIFlash::ID_BYTES[i]);
    }

    /***** Page sectors and blocks *****/
    uint16_t blockIndex;
    uint16_t pageIndex;
    blockIndex = iuFlash.getBlockIndex(IUSPIFlash::SECTOR, 300);
    assertEqual(blockIndex, 18);
    pageIndex = iuFlash.getBlockFirstPage(IUSPIFlash::SECTOR, blockIndex);
    assertEqual(pageIndex, 288);

    blockIndex = iuFlash.getBlockIndex(IUSPIFlash::BLOCK_32KB, 300);
    assertEqual(blockIndex, 2);
    pageIndex = iuFlash.getBlockFirstPage(IUSPIFlash::BLOCK_32KB, blockIndex);
    assertEqual(pageIndex, 256);

    blockIndex = iuFlash.getBlockIndex(IUSPIFlash::BLOCK_64KB, 300);
    assertEqual(blockIndex, 1);
    pageIndex = iuFlash.getBlockFirstPage(IUSPIFlash::BLOCK_64KB, blockIndex);
    assertEqual(pageIndex, 256);
}

test(IUSPIFlash__readWrite)
{
    // Write page
    uint8_t pageContent[256];
    for (uint16_t i = 0; i < 256; ++i)
    {
        pageContent[i] = (uint8_t) i;
    }
    iuFlash.programPage(pageContent, IUSPIFlash::MAX_PAGE_NUMBER);

    // Normal speed read
    for (uint16_t i = 0; i < 256; ++i)
    {
        pageContent[i] = 0;
    }
    iuFlash.readPages(pageContent, IUSPIFlash::MAX_PAGE_NUMBER, 1, false);
    assertEqual(pageContent[0], 0);
    assertEqual(pageContent[10], 10);
    assertEqual(pageContent[128], 128);
    assertEqual(pageContent[255], 255);

    // High speed read
    for (uint16_t i = 0; i < 256; ++i)
    {
        pageContent[i] = 0;
    }
    iuFlash.readPages(pageContent, IUSPIFlash::MAX_PAGE_NUMBER, 1, true);
    assertEqual(pageContent[0], 0);
    assertEqual(pageContent[10], 10);
    assertEqual(pageContent[128], 128);
    assertEqual(pageContent[255], 255);

    // Erase Page
    iuFlash.erasePages(IUSPIFlash::SECTOR, IUSPIFlash::MAX_PAGE_NUMBER);
    iuFlash.readPages(pageContent, IUSPIFlash::MAX_PAGE_NUMBER, 1, true);
    assertEqual(pageContent[0], 255);
    assertEqual(pageContent[10], 255);
    assertEqual(pageContent[128], 255);
    assertEqual(pageContent[255], 255);
}

#endif // CMP_IUSPIFLASH_H
