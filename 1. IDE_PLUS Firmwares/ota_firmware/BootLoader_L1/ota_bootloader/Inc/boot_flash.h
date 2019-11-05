#ifndef FLASH_H_
#define FLASH_H_

#include "stm32l4xx_hal.h"

/* Start and end addresses of the user application. */
#define FLASH_MFW_START_ADDRESS ((uint32_t)0x08060000u)
#define FLASH_FFW_START_ADDRESS ((uint32_t)0x08036000u)
#define FLASH_BL2_START_ADDRESS ((uint32_t)0x08010000u)
#define FLASH_APP_END_ADDRESS   ((uint32_t)FLASH_BANK1_END-0x10u) /**< Leave a little extra space at the end. */

//#define FLAG_ADDRESS     (uint32_t)0x08060000u    /* Start address of application space in flash */
#define FLAG_ADDRESS     (uint32_t)0x080FF800u    /* Start address of application space in flash */
//#define FLAG_END_ADDRESS     (uint32_t)0x08060FFFu    /* End address of application space (addr. of last byte) */
#define FLAG_END_ADDRESS     (uint32_t)0x080FFFFFu    /* End address of application space (addr. of last byte) */
#define FLASH_PAGE_NBPERBANK    256             /* Number of pages per bank in flash */
/* Status report for the functions. */
typedef enum {
  FLASH_OK              = 0x00u, /**< The action was successful. */
  FLASH_ERROR_SIZE      = 0x01u, /**< The binary is too big. */
  FLASH_ERROR_WRITE     = 0x02u, /**< Writing failed. */
  FLASH_ERROR_READBACK  = 0x04u, /**< Writing was successful, but the content of the memory is wrong. */
  FLASH_ERROR           = 0xFFu  /**< Generic error. */
} flash_status;
/* Bootloader Error Codes */
enum
{
    BL_OK = 0,
    BL_NO_APP,
    BL_SIZE_ERROR,
    BL_CHKS_ERROR,
    BL_ERASE_ERROR,
    BL_WRITE_ERROR,
    BL_OBP_ERROR
};

void flash_jump_to_factory_firmware(void);
void flash_jump_to_main_firmware(void);
void flash_jump_boot_loader_L2(void);
void    Bootloader_Init(void);
uint8_t Flag_Erase_All(void);

void    Bootloader_FlashBegin(void);
uint8_t Bootloader_FlashNext(uint8_t data);
void    Bootloader_FlashEnd(void);
uint8_t Write_Flag(uint8_t FlagAddr, uint32_t data);
uint32_t Read_Flag(uint8_t FlagAddr);

#endif /* FLASH_H_ */
