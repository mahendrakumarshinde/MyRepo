/*
 * boot_flag.c
 *
 *  Created on: 20-Oct-2019
 *      Author: arun
 */
#include "main.h"
#include "boot_flash.h"

void read_flag(uint16_t flag_addr)
{
	uint8_t  FlashValue = 0;
	FlashValue = *(uint64_t*)(FLAG_ADDRESS);


}
void write_flag(uint16_t flag_addr, uint8_t data)
{
  	Bootloader_Init();
    Bootloader_Erase();
    Bootloader_FlashBegin();
    Bootloader_FlashNext((uint8_t)0x12);
    Bootloader_FlashEnd();


}
