/*
 * boot_flash.c
 *
 *  Created on: Oct 18, 2019
 *      Author: arun
 */
#include "boot_flash.h"
#include "stm32l4xx.h"

/* Function pointer for jumping to user application. */
typedef void (*fnc_ptr)(void);
typedef void (*pFunction)(void);

static uint32_t flash_ptr = FLAG_ADDRESS;

void flash_jump_to_factory_firmware(void)
{
  /* Function pointer to the address of the user application. */
	uart_transmit_str((uint8_t*)"inside fuct.... \n\r");
  fnc_ptr jump_to_app;
  jump_to_app = (fnc_ptr)(*(volatile uint32_t*) (FLASH_FFW_START_ADDRESS+4u));
  HAL_RCC_DeInit();
  HAL_DeInit();
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  SCB->VTOR = FLASH_FFW_START_ADDRESS;


 // SCB->VTOR = SRAM_BASE | 0x00; /* Vector Table Relocation in Internal SRAM */

 // SCB->VTOR = FLASH_FFW_START_ADDRESS | 0x00; /* Vector Table Relocation in Internal FLASH */


  /* Change the main stack pointer. */
  __set_MSP(*(volatile uint32_t*)FLASH_FFW_START_ADDRESS);
  jump_to_app();
}
void flash_jump_to_main_firmware(void)
{
  /* Function pointer to the address of the user application. */
  fnc_ptr jump_to_app;
  jump_to_app = (fnc_ptr)(*(volatile uint32_t*) (FLASH_MFW_START_ADDRESS+4u));
  HAL_RCC_DeInit();
  HAL_DeInit();
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  SCB->VTOR = FLASH_MFW_START_ADDRESS;


 // SCB->VTOR = SRAM_BASE | 0x00; /* Vector Table Relocation in Internal SRAM */

  //SCB->VTOR = FLASH_MFW_START_ADDRESS | 0x00; /* Vector Table Relocation in Internal FLASH */


  /* Change the main stack pointer. */
  __set_MSP(*(volatile uint32_t*)FLASH_MFW_START_ADDRESS);
  jump_to_app();
}
void flash_jump_boot_loader_L2(void)
{
  /* Function pointer to the address of the user application. */
	uart_transmit_str((uint8_t*)"inside fuct.... \n\r");
  fnc_ptr jump_to_app;
  jump_to_app = (fnc_ptr)(*(volatile uint32_t*) (FLASH_BL2_START_ADDRESS+4u));
  HAL_RCC_DeInit();
  HAL_DeInit();
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  SCB->VTOR = FLASH_BL2_START_ADDRESS;


 // SCB->VTOR = SRAM_BASE | 0x00; /* Vector Table Relocation in Internal SRAM */

 // SCB->VTOR = FLASH_FFW_START_ADDRESS | 0x00; /* Vector Table Relocation in Internal FLASH */


  /* Change the main stack pointer. */
  __set_MSP(*(volatile uint32_t*)FLASH_BL2_START_ADDRESS);
  jump_to_app();
}
void Bootloader_Init(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_FLASH_CLK_ENABLE();

    /* Clear flash flags */
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    HAL_FLASH_Lock();
}

uint8_t Flag_Erase_All(void)
{
    uint32_t NbrOfPages = 0;
    uint32_t PageError  = 0;
    FLASH_EraseInitTypeDef  pEraseInit;
    HAL_StatusTypeDef       status = HAL_OK;

    HAL_FLASH_Unlock();

    /* Get the number of pages to erase */
    NbrOfPages = (FLASH_BASE + FLASH_SIZE - FLAG_ADDRESS) / FLASH_PAGE_SIZE;//stm32l496rg number of pages = 512
   // NbrOfPages= 511;

    if(NbrOfPages > FLASH_PAGE_NBPERBANK)
    {
        pEraseInit.Banks = FLASH_BANK_1;
        pEraseInit.NbPages = NbrOfPages % FLASH_PAGE_NBPERBANK;
        pEraseInit.Page = FLASH_PAGE_NBPERBANK - pEraseInit.NbPages;
        pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
        status = HAL_FLASHEx_Erase(&pEraseInit, &PageError);
        NbrOfPages = FLASH_PAGE_NBPERBANK;
    }

    if(status == HAL_OK)
    {
        pEraseInit.Banks = FLASH_BANK_2;
        pEraseInit.NbPages = 4;
        pEraseInit.Page = FLASH_PAGE_NBPERBANK - pEraseInit.NbPages;
        pEraseInit.Page = 508;
        pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
        status = HAL_FLASHEx_Erase(&pEraseInit, &PageError);
        //status = HAL_FLASHEx_Erase(&pEraseInit, PageError);
    }

    HAL_FLASH_Lock();

    return (status == HAL_OK) ? BL_OK : BL_ERASE_ERROR;
}

void Bootloader_FlashBegin(void)
{
    /* Reset flash destination address */
	flash_ptr = FLAG_ADDRESS;

    /* Unlock flash */
    HAL_FLASH_Unlock();
}
uint8_t Bootloader_FlashNext(uint8_t data)
{
    if( !(flash_ptr <= (FLASH_BASE + FLASH_SIZE - 8)) || (flash_ptr < FLAG_ADDRESS) )
    {
        HAL_FLASH_Lock();
        return BL_WRITE_ERROR;
    }

    if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_ptr, data) == HAL_OK)
    {
        /* Check the written value */
        if(*(uint8_t*)flash_ptr != data)
        {
            /* Flash content doesn't match source content */
            HAL_FLASH_Lock();
            return BL_WRITE_ERROR;
        }
        /* Increment Flash destination address */
        flash_ptr += 8;
    }
    else
    {
        /* Error occurred while writing data into Flash */
        HAL_FLASH_Lock();
        return BL_WRITE_ERROR;
    }

    return BL_OK;
}
uint8_t Write_Flag(uint8_t FlagAddr, uint32_t data)
{
	uint32_t FlagAddr_temp = (FlagAddr*8)+FLAG_ADDRESS;

	if( !(FlagAddr_temp <= (FLASH_BASE + FLASH_SIZE - 8)) || (FlagAddr_temp < FLAG_ADDRESS) )
    {
        HAL_FLASH_Lock();
        return BL_WRITE_ERROR;
    }

    if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, FlagAddr_temp, data) == HAL_OK)
    {
        /* Check the written value */
        if(*(uint32_t*)FlagAddr_temp != data)
        {
            /* Flash content doesn't match source content */
            HAL_FLASH_Lock();
            return BL_WRITE_ERROR;
        }
        /* Increment Flash destination address */
        //flash_ptr += 8;
    }
    else
    {
        /* Error occurred while writing data into Flash */
        HAL_FLASH_Lock();
        return BL_WRITE_ERROR;
    }

    return BL_OK;
}
void Bootloader_FlashEnd(void)
{
    /* Lock flash */
    HAL_FLASH_Lock();
}
uint32_t Read_Flag(uint8_t FlagAddr)
{
	uint32_t FlashValue = 0;

	uint32_t FlagAddr_temp = (FlagAddr*8)+FLAG_ADDRESS;
	FlashValue = *(uint32_t*)(FlagAddr_temp);
	return FlashValue;

}

