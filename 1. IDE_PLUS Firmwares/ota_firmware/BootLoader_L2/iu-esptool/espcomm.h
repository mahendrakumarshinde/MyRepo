/****************************************************************************
 ***
 ***    espcomm.h
 ***    - include file for espcomm.c
 ***
 **/

#ifndef ESPCOMM_H
#define ESPCOMM_H

#include <stdbool.h>
#include <stdint.h>
#include "string.h"

enum
{
    CMD0                        = 0x00,
    CMD1                        = 0x01,
    FLASH_DOWNLOAD_BEGIN        = 0x02,
    FLASH_DOWNLOAD_DATA         = 0x03,
    FLASH_DOWNLOAD_DONE         = 0x04,
    RAM_DOWNLOAD_BEGIN          = 0x05,
    RAM_DOWNLOAD_END            = 0x06,
    RAM_DOWNLOAD_DATA           = 0x07,
    SYNC_FRAME                  = 0x08,
    WRITE_REGISTER              = 0x09,
    READ_REGISTER               = 0x0A,
    SET_FLASH_PARAMS            = 0x0B,
    NO_COMMAND                  = 0xFF
};


typedef struct
{
    uint8_t     direction;
    uint8_t     command;
    uint16_t    size_f;
    union
    {
        uint32_t    checksum;
        uint32_t    response;
    };

    unsigned char *data;

} bootloader_packet;

#define BLOCKSIZE_FLASH         0x0400
#define BLOCKSIZE_RAM           0x1800

int espcomm_set_port(char *port);
//int espcomm_set_baudrate(const char *baudrate);
int espcomm_set_baudrate(uint32_t new_baudrate);
//int espcomm_set_address(const char *address);
int espcomm_set_address(uint32_t new_address);
int espcomm_set_board(const char* name);
int espcomm_set_chip(const char* name);

int espcomm_open(void);
void espcomm_close(void);

bool espcomm_upload_mem_to_RAM(uint8_t* src, size_t size_f, int address, int entry);
bool espcomm_upload_mem(uint8_t* src, size_t sizev, const char* source_name);
bool espcomm_upload_file(const char *name);
bool espcomm_upload_file_compressed(const char* name);
int espcomm_file_uploaded();
int espcomm_start_app(int reboot);
bool espcomm_reset();
bool espcomm_erase_region(const char* size_f);
bool espcomm_erase_flash();

#endif
