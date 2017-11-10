#ifndef TEST_COMPONENT_H_INCLUDED
#define TEST_COMPONENT_H_INCLUDED


#include <ArduinoUnit.h>
#include "../IUSPIFlash.h"


test(IUSPIFlash__initialization)
{
    iuSPIFlash.setupHardware();

    assertEqual(iuSPIFlash.getPowerMode(), PowerMode::ACTIVE);

    uint8_t destination[20];

    iuSPIFlash.readId(destination, 20);
    Serial.print("ID bytes: ");
    Serial.println(destination);

    uint16_t blockIndex;
    uint16_t pageIndex;
    blockIndex = iuSPIFlash.getBlockIndex(iuSPIFlash::SECTOR, 300);
    Serial.println("SECTOR");
    Serial.println(blockIndex);
    pageIndex = iuSPIFlash.getBlockFirstPage(iuSPIFlash::SECTOR, blockIndex);
    Serial.println(pageIndex);

    blockIndex = iuSPIFlash.getBlockIndex(iuSPIFlash::BLOCK_32K, 300);
    Serial.println("BLOCK_32K");
    Serial.println(blockIndex);
    pageIndex = iuSPIFlash.getBlockFirstPage(iuSPIFlash::BLOCK_32K, blockIndex);
    Serial.println(pageIndex);

    blockIndex = iuSPIFlash.getBlockIndex(iuSPIFlash::BLOCK_64K, 300);
    Serial.println("BLOCK_64K");
    Serial.println(blockIndex);
    pageIndex = iuSPIFlash.getBlockFirstPage(iuSPIFlash::BLOCK_64K, blockIndex);
    Serial.println(pageIndex);

    // Write page
    uint8_t pageContent[256];
    for (uint16_t i = 0; i < 256; ++i)
    {
        pageContent[i] = (uint8_t) i;
    }
    iuSPIFlash.programPage(pageContent, iuSPIFlash::MAX_PAGE_NUMBER);

    // Normal speed read
    for (uint16_t i = 0; i < 256; ++i)
    {
        pageContent[i] = 0;
    }
    iuSPIFlash.readPages(pageContent, iuSPIFlash::MAX_PAGE_NUMBER, 1, false);
    Serial.println("Normal speed read");
    Serial.println(pageContent[0]);
    Serial.println(pageContent[10]);
    Serial.println(pageContent[128]);
    Serial.println(pageContent[255]);

    // High speed read
    for (uint16_t i = 0; i < 256; ++i)
    {
        pageContent[i] = 0;
    }
    iuSPIFlash.readPages(pageContent, iuSPIFlash::MAX_PAGE_NUMBER, 1, true);
    Serial.println("Normal speed read");
    Serial.println(pageContent[0]);
    Serial.println(pageContent[10]);
    Serial.println(pageContent[128]);
    Serial.println(pageContent[255]);

    // Erase Page
    iuSPIFlash.erasePages(iuSPIFlash::SECTOR, iuSPIFlash::MAX_PAGE_NUMBER);
    iuSPIFlash.readPages(pageContent, iuSPIFlash::MAX_PAGE_NUMBER, 1, true);
    Serial.println("Erase Page");
    Serial.println(pageContent[0]);
    Serial.println(pageContent[10]);
    Serial.println(pageContent[128]);
    Serial.println(pageContent[255]);
}

#endif // TEST_COMPONENT_H_INCLUDED
