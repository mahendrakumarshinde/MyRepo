  /****************************************************************************
 ***
 ***    espcomm.c
 ***    - routines to access the bootloader in the ESP
 ***
 **/
#include "FS.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
//#include <errno.h>

//#include "infohelper.h"
#include "iu-esptool.h"
#include "espcomm.h"
#include "serialport.h"
#include "espcomm_boards.h"
//#include "delay.h"
#include "HardwareSerial.h"
#include <Arduino.h>
#include "stm32l4_flash.h"
#include "stm32l4_wiring_private.h"




extern void infohelper_output(int loglevel, const char* format, ...);
extern void infohelper_output_plain(int loglevel, const char* format, ...);
extern void espcomm_delay_ms(int ms);

const int progress_line_width = 80;

bootloader_packet send_packet;
bootloader_packet receive_packet;

static espcomm_board_t* espcomm_board = 0;
static bool sync_stage = false;
static bool upload_stage = false;
static bool espcomm_is_open = false;

//static const char *espcomm_port =
//#if defined(__APPLE__) && defined(__MACH__)
//"/dev/tty.usbserial";
//#elif defined(_WIN32)
//"COM1";
//#elif defined(__linux__)
//"/dev/ttyUSB0";
//#else
//"";
//#endif



static unsigned int espcomm_baudrate = 115200;
static uint32_t espcomm_address = 0x00000;

static unsigned char sync_frame[36] = { 0x07, 0x07, 0x12, 0x20,
                               0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                               0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                               0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                               0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };

static uint32_t flash_packet[BLOCKSIZE_FLASH+32];
static uint32_t ram_packet[BLOCKSIZE_RAM+32];

static int file_uploaded = 0;



static void espcomm_enter_boot(void)
{
	espcomm_board_reset_into_bootloader(espcomm_board);
}

static void espcomm_reset_to_exec(void)
{
	espcomm_board_reset_into_app(espcomm_board);
}

uint32_t espcomm_calc_checksum(unsigned char *data, uint16_t data_size)
{
	uint16_t cnt;
	uint32_t result;

	result = 0xEF;

	for (cnt = 0; cnt < data_size; cnt++) {
		result ^= data[cnt];
	}

	return result;
}

static uint32_t espcomm_send_command(unsigned char command, unsigned char *data, uint16_t data_size, int reply_timeout)
{
	uint32_t result;
	uint32_t cnt;

	result = 0;
	if (command != NO_COMMAND) {
		send_packet.direction = 0x00;
		send_packet.command = command;
		send_packet.size_f = data_size;

		serialport_send_C0();

		if (!upload_stage)
			Serial.println("espcomm_send_command: sending command header");
		else
			Serial.println("espcomm_send_command: sending command header");

		serialport_send_slip((unsigned char*) &send_packet, 8);

		if (data_size) {
			if (!upload_stage)
				Serial.println("espcomm_send_command: sending command payload");
			else
				Serial.println("espcomm_send_command: sending command payload");
			serialport_send_slip(data, data_size);
		} else {
			  Serial.println("espcomm_send_command: no payload");
		}

		serialport_send_C0();
	}

	espcomm_delay_ms(5);
	serialport_drain();

	int old_timeout=0;

	if (reply_timeout) {
		old_timeout = serialport_get_timeout();
		serialport_set_timeout(reply_timeout);
	}

    if (serialport_receive_C0()) {
		if (old_timeout)
			serialport_set_timeout(old_timeout);

		if (serialport_receive_slip((unsigned char*) &receive_packet, 8)) {
			if (receive_packet.size_f) {
				if (!upload_stage)
					LOGDEBUG("espcomm_send_command: receiving %i bytes of data", receive_packet.size_f);
				else
					LOGVERBOSE("espcomm_send_command: receiving %i bytes of data", receive_packet.size_f);

				if (receive_packet.data) {
					free(receive_packet.data);
					receive_packet.data = NULL;
				}

				receive_packet.data = (unsigned char *)malloc(receive_packet.size_f);

				if (serialport_receive_slip(receive_packet.data, receive_packet.size_f) == 0) {
					LOGWARN("espcomm_send_command: can't receive slip payload data");
         Serial.println("espcomm_send_command: can't receive slip payload data");
					return 0;
				} else {
					LOGVERBOSE("espcomm_send_command: received %x bytes: ", receive_packet.size_f);
					for (cnt = 0; cnt < receive_packet.size_f; cnt++) {
						LOGVERBOSE("0x%02X ", receive_packet.data[cnt]);
					}
				}
			}

			if (serialport_receive_C0()) {
				if (receive_packet.direction == 0x01 &&
					(command == NO_COMMAND || receive_packet.command == command)) {
					result = receive_packet.response;
				} else {
					LOGWARN("espcomm_send_command: wrong direction/command: 0x%02X 0x%02X, expected 0x%02X 0x%02X",
						receive_packet.direction, receive_packet.command, 1, command);
            Serial.println("espcomm_send_command: wrong direction/command:");
					return 0;
				}
			} else {
				if (!sync_stage)
					LOGWARN("espcomm_send_command: no final C0");
				else
					LOGVERBOSE("espcomm_send_command: no final C0");
          Serial.println("espcomm_send_command: no final C0");
				return 0;
			}
		} else {
			LOGWARN("espcomm_send_command: can't receive command response header");
     Serial.println("espcomm_send_command: can't receive command response header");
			return 0;
		}
	} else {
		if (old_timeout)
			serialport_set_timeout(old_timeout);

		if (!sync_stage)
			LOGWARN("espcomm_send_command: didn't receive command response");
		else
			LOGVERBOSE("espcomm_send_command: didn't receive command response");
      Serial.println("espcomm_send_command: didn't receive command response");
		return 0;
	}

	LOGVERBOSE("espcomm_send_command: response 0x%08X", result);
	return result;
}

static int espcomm_sync(void)
{
	sync_stage = true;
	for (int retry_boot = 0; retry_boot < 3; ++retry_boot) {
		LOGINFO("resetting board");
		espcomm_enter_boot();
		for (int retry_sync = 0; retry_sync < 3; ++retry_sync) {
			LOGINFO("trying to connect");
			espcomm_delay_ms(100);
			serialport_flush();

			send_packet.checksum = espcomm_calc_checksum((unsigned char*)&sync_frame, 36);
			if (espcomm_send_command(SYNC_FRAME, (unsigned char*) &sync_frame, 36, 0) == 0x20120707) {
				bool error=false;

				for (int i = 0; i < 7; ++i) {
					if (espcomm_send_command(NO_COMMAND, 0, 0, 0) != 0x20120707) {
						error = true;
						break;
					}
				}
				if (!error) {
					sync_stage = false;
					return 1;
				}
			}
		}
	}
	sync_stage = false;
	LOGWARN("espcomm_sync failed");
	return 0;
}

int espcomm_open(void)
{
//	if (espcomm_is_open)
//		return 1;
//
//	if (serialport_open(espcomm_port, espcomm_baudrate)) {
//		LOGINFO("opening bootloader");
//		if (espcomm_sync()) {
//			espcomm_is_open = true;
//			return 1;
//		}
//	}
//
//	return 0;
}

void espcomm_close(void)
{
	LOGINFO("closing bootloader");
	serialport_close();
}

int espcomm_set_flash_params(uint32_t device_id, uint32_t chip_size,
	uint32_t block_size, uint32_t sector_size,
	uint32_t page_size, uint32_t status_mask)
{
	LOGDEBUG("espcomm_set_flash_params: %x %x %x %x %x %x", device_id, chip_size,
		block_size, sector_size, page_size, status_mask);

	flash_packet[0] = device_id;
	flash_packet[1] = chip_size;
	flash_packet[2] = block_size;
	flash_packet[3] = sector_size;
	flash_packet[4] = page_size;
	flash_packet[5] = status_mask;

	send_packet.checksum = espcomm_calc_checksum((unsigned char*) flash_packet, 24);

	uint32_t res = espcomm_send_command(SET_FLASH_PARAMS, (unsigned char*) &flash_packet, 24, 0);
	return res;
}

int espcomm_start_flash(uint32_t size, uint32_t address)
{
	uint32_t res;
  Serial.println("inside espcomm start flash");
	LOGDEBUG("size: %06x address: %06x", size, address);

	uint32_t min_chip_size = size + address;
	uint32_t chip_size = 0x400000;
	/* Opportunistically assume that the flash chip size is sufficient.
       Todo: detect flash chip size using a stub */
	if (min_chip_size > 0x1000000) {
		LOGWARN("Invalid size/address, too large: %x %x", size, address);
		return 0;
	}
	if (min_chip_size > 0x800000) {
		chip_size = 0x1000000;
	} else if (min_chip_size > 0x400000) {
		chip_size = 0x800000;
	}

	if (chip_size > 0x400000) {
		LOGINFO("Assuming flash chip size=%dMB", chip_size / 0x100000);
		res = espcomm_set_flash_params(0, chip_size, 64*1024, 16*1024, 256, 0xffff);
		if (res == 0) {
			LOGWARN("espcomm_send_command(SET_FLASH_PARAMS) failed");
			return res;
		}
	}

	const int sector_size = 4096;
	const int sectors_per_block  = 16;
	const int first_sector_index = address / sector_size;
	LOGDEBUG("first_sector_index: %d", first_sector_index);

	const int total_sector_count = ((size % sector_size) == 0) ?
									(size / sector_size) : (size / sector_size + 1);
	LOGDEBUG("total_sector_count: %d", total_sector_count);

	const int max_head_sector_count  = sectors_per_block - (first_sector_index % sectors_per_block);
	const int head_sector_count = (max_head_sector_count > total_sector_count) ?
									total_sector_count : max_head_sector_count;
	LOGDEBUG("head_sector_count: %d", head_sector_count);

	// SPIEraseArea function in the esp8266 ROM has a bug which causes extra area to be erased.
	// If the address range to be erased crosses the block boundary,
	// then extra head_sector_count sectors are erased.
	// If the address range doesn't cross the block boundary,
	// then extra total_sector_count sectors are erased.

	const int adjusted_sector_count = (total_sector_count > 2 * head_sector_count) ?
									(total_sector_count - head_sector_count):
									(total_sector_count + 1) / 2;
	LOGDEBUG("adjusted_sector_count: %d", adjusted_sector_count);

	uint32_t erase_size = adjusted_sector_count * sector_size;

	LOGDEBUG("erase_size: %06x", erase_size);

	flash_packet[0] = erase_size;
	flash_packet[1] = (size + BLOCKSIZE_FLASH - 1) / BLOCKSIZE_FLASH;
	flash_packet[2] = BLOCKSIZE_FLASH;
	flash_packet[3] = address;

	send_packet.checksum = espcomm_calc_checksum((unsigned char*) flash_packet, 16);

	int delay_per_erase_block_ms = 7;
	int timeout_ms = erase_size / BLOCKSIZE_FLASH * delay_per_erase_block_ms + 5000;
	if (timeout_ms < 15000) timeout_ms = 15000;
	//LOGDEBUG("calculated erase delay: %dms = max(%db / %db * %dms + 5000, 15000)", timeout_ms, erase_size, BLOCKSIZE_FLASH, delay_per_erase_block_ms);
	res = espcomm_send_command(FLASH_DOWNLOAD_BEGIN, (unsigned char*) &flash_packet, 16, timeout_ms);
	return res;
}

bool espcomm_upload_mem(uint8_t* src, size_t size_f, const char* source_name)
{
	//LOGDEBUG("espcomm_upload_mem");
//	if (!espcomm_open()) {
//		LOGERR("espcomm_open failed");
//		return false;
//	}

	Serial.print("Uploading bytes from to flash\n");
  Serial.print(size_f);
  Serial.println(source_name);
	//LOGDEBUG("erasing flash");
	int res = espcomm_start_flash(size_f, espcomm_address);
 Serial.print("returning from espcomm_start_flash_ with res :");
 Serial.println(res);
	if (res == 0) {
		LOGWARN("espcomm_send_command(FLASH_DOWNLOAD_BEGIN) failed");
		espcomm_close();
		return false;
	}

	//LOGDEBUG("writing flash");
	upload_stage = true;
	size_t total_count = (size_f + BLOCKSIZE_FLASH - 1) / BLOCKSIZE_FLASH;
	size_t count = 0;

	while(size_f) {
		flash_packet[0] = BLOCKSIZE_FLASH;
		flash_packet[1] = count;
		flash_packet[2] = 0;
		flash_packet[3] = 0;

		memset(flash_packet + 4, 0xff, BLOCKSIZE_FLASH);

		size_t write_size = (size_f < BLOCKSIZE_FLASH)?size_f:BLOCKSIZE_FLASH;
		memcpy(flash_packet + 4, src, write_size);
		size_f -= write_size;
		src += write_size;

		send_packet.checksum = espcomm_calc_checksum((unsigned char *) (flash_packet + 4), BLOCKSIZE_FLASH);
		res = espcomm_send_command(FLASH_DOWNLOAD_DATA, (unsigned char*) flash_packet, BLOCKSIZE_FLASH + 16, 0);
    Serial.print("res :");
    Serial.println(res);
		if (res == 0) {
			//LOGWARN("espcomm_send_command(FLASH_DOWNLOAD_DATA) failed");
			res = espcomm_send_command(FLASH_DOWNLOAD_DONE, (unsigned char*) flash_packet, 4, 0);
			espcomm_close();
			upload_stage = false;
			return false;
		}

		++count;
		INFO(".");
		if (count % progress_line_width == 0) {
			INFO(" [ %2d%% ]\n", count * 100 / total_count);
		}
		fflush(stdout);
    }
	if (count % progress_line_width) {
		while (++count % progress_line_width) {
			INFO(" ");
		}
		INFO("  [ 100%% ]\n");
	}
	upload_stage = false;
	file_uploaded = 1;
	return true;
}

bool espcomm_upload_mem_to_RAM(uint8_t* src, size_t size_f, int address, int entry)
{
	//LOGDEBUG("espcomm_upload_mem_to_RAM");
	if (!espcomm_open()) {
		//LOGERR("espcomm_open failed");
		return false;
	}

	//INFO("Uploading %i bytes to RAM at 0x%08X\n", size, address);

	ram_packet[0] = size_f;
	ram_packet[1] = 0x00000200;
	ram_packet[2] = BLOCKSIZE_RAM;
	ram_packet[3] = address;

	send_packet.checksum = espcomm_calc_checksum((unsigned char*) ram_packet, 16);
	int res = espcomm_send_command(RAM_DOWNLOAD_BEGIN, (unsigned char*) &ram_packet, 16, 0);
	if (res == 0) {
		//LOGWARN("espcomm_send_command(RAM_DOWNLOAD_BEGIN) failed");
		espcomm_close();
		return false;
	}

	//LOGDEBUG("writing to RAM");
	upload_stage = true;

	size_t count = 0;

	while (size_f) {
		size_t will_write = (size_f < BLOCKSIZE_RAM)?size_f:BLOCKSIZE_RAM;

		will_write = (will_write + 3) & (~3);

		ram_packet[0] = will_write;
		ram_packet[1] = count;
		ram_packet[2] = 0;
		ram_packet[3] = 0;

		memset(ram_packet + 4, 0xff, BLOCKSIZE_RAM);

		size_t write_size = (size_f < BLOCKSIZE_RAM)?size_f:BLOCKSIZE_RAM;

		memcpy(ram_packet + 4, src, write_size);
		size_f -= write_size;
		src += write_size;

		send_packet.checksum = espcomm_calc_checksum((unsigned char *) (ram_packet + 4), will_write);
		res = espcomm_send_command(RAM_DOWNLOAD_DATA, (unsigned char*) ram_packet, will_write + 16, 0);

		if (res == 0) {
			//LOGWARN("espcomm_send_command(RAM_DOWNLOAD_DATA) failed");
			espcomm_close();
			upload_stage = false;
			return false;
		}
		++count;
		INFO(".");
		fflush(stdout);
	}
	upload_stage = false;
	INFO("\n");
	ram_packet[0] = (entry)?0:1;
	ram_packet[1] = entry;
	send_packet.checksum = 0;
	res = espcomm_send_command(RAM_DOWNLOAD_END, (unsigned char*) ram_packet, 8, 0);
	return true;
}

bool espcomm_upload_file(const char *name_f)
{
	LOGDEBUG("espcomm_upload_file");
//	struct stat st;
//
//	if (stat(name_f, &st) != 0) {
//		LOGERR("stat %s failed: %s", name, strerror(errno));
//		LOGERR("stat %s failed", name);
//		return false;
//	}

	File f = DOSFS.open(name_f,"r");
  
  
	
	if (!f) {
    Serial.println("failed to open file for reading");
		LOGERR("failed to open file for reading");
		return false;
	}
  uint16_t file_size = f.size();
  
	uint8_t* file_contents = (uint8_t*) malloc(file_size);

	if (!file_contents) {
    Serial.println("failed to allocate buffer for file contents");
		//LOGERR("failed to allocate buffer for file contents");
		//fclose(f);
    f.close();
		return false;
	}
    size_t cb = f.read(file_contents,file_size);
//	size_t cb = f.read(file_contents, 1, st.st_size, f);

  f.close();
//	fclose(f);

	if (cb != file_size) {
    Serial.println("failed to read file contents");
		//LOGERR("failed to read file contents");
		free(file_contents);
		return false;
	}
 Serial.print("befor calling espcomm upload mem :");
  Serial.println(file_size);
  for(int i =0 ; i<10 ; i++)
  {
	if (!espcomm_upload_mem(file_contents, file_size, name_f)) {
    Serial.println("espcomm_upload_mem failed");
		//LOGERR("espcomm_upload_mem failed");
		free(file_contents); 
  }

  delay(1000);
  }
  return false;
 

	free(file_contents);
	return true;
  
}

/*
bool espcomm_erase_region(const char* size)
{
	uint32_t erase_size = (uint32_t) strtol(size, NULL, 16);

	LOGDEBUG("setting erase size to 0x%08X",erase_size);
    
	if (!espcomm_open()) {
		LOGERR("espcomm_open failed");
		return false;
	}

	INFO("Erasing 0x%X bytes starting at 0x%08X\n", erase_size, espcomm_address);
	LOGDEBUG("erasing flash");
	int res = espcomm_start_flash(erase_size, espcomm_address);
	if (res == 0) {
		LOGWARN("espcomm_send_command(FLASH_DOWNLOAD_BEGIN) failed");
		espcomm_close();
		return false;
	}

	return true;
}
*/

int espcomm_start_app(int reboot)
{
	if (!espcomm_open()) {
		//LOGDEBUG("espcomm_open failed");
	}

	if (reboot) {
		//LOGINFO("starting app with reboot");
		flash_packet[0] = 0;
	} else {
		LOGINFO("starting app without reboot");
		flash_packet[0] = 1;
	}

	espcomm_send_command(FLASH_DOWNLOAD_DONE, (unsigned char*) &flash_packet, 4, 0);
	file_uploaded = 0;

	espcomm_close();
	return 1;
}

int espcomm_file_uploaded()
{
	return file_uploaded;
}

int espcomm_set_port(char *port)
{
//	LOGDEBUG("setting port from %s to %s", espcomm_port, port);
//	espcomm_port = port;
	return 1;
}

//int espcomm_set_baudrate(const char *baudrate)
int espcomm_set_baudrate(uint32_t new_baudrate)
{
//	uint32_t new_baudrate = (uint32_t) strtol(baudrate, NULL, 10);

//	LOGDEBUG("setting baudrate from %i to %i", espcomm_baudrate, new_baudrate);
	espcomm_baudrate = new_baudrate;
	return 1;
}

//int espcomm_set_address(const char *address)
int espcomm_set_address(uint32_t new_address)
{
//	uint32_t new_address = (uint32_t) strtol(address, NULL, 16);

	//LOGDEBUG("setting address from 0x%08X to 0x%08X", espcomm_address, new_address);
	espcomm_address = new_address;
	return 1;
}

int espcomm_set_board(const char* name)
{
	LOGDEBUG("setting board to %s", name);
	espcomm_board = espcomm_board_by_name(name);
	if (!espcomm_board) {
		LOGERR("unknown board: %s", name);
		INFO("known boards are: ");
		for (espcomm_board_t* b = espcomm_board_first(); b; b = espcomm_board_next(b)) {
			INFO("%s ", espcomm_board_name(b));
		}
		INFO("\n");
		return 0;
	}
	return 1;
}

bool espcomm_erase_flash()
{
	LOGDEBUG("espcomm_erase_flash");
	if (!espcomm_open()) {
		LOGERR("espcomm_open failed");
		return false;
	}

	flash_packet[0] = 0;
	flash_packet[1] = 0x00000200;
	flash_packet[2] = BLOCKSIZE_FLASH;
	flash_packet[3] = 0;
	send_packet.checksum = espcomm_calc_checksum((unsigned char*) flash_packet, 16);
	int res = espcomm_send_command(FLASH_DOWNLOAD_BEGIN, (unsigned char*) &flash_packet, 16, 1000);
	if (res == 0) {
		//LOGERR("espcomm_erase_flash: FLASH_DOWNLOAD_BEGIN failed");
		return false;
	}

	ram_packet[0] = 0;
	ram_packet[1] = 0x00000200;
	ram_packet[2] = BLOCKSIZE_RAM;
	ram_packet[3] = 0x40100000;

	send_packet.checksum = espcomm_calc_checksum((unsigned char*) ram_packet, 16);
	res = espcomm_send_command(RAM_DOWNLOAD_BEGIN, (unsigned char*) &ram_packet, 16, 0);
	if (res == 0) {
		//LOGERR("espcomm_erase_flash: RAM_DOWNLOAD_BEGIN failed");
		return false;
	}

	ram_packet[0] = 0;
	ram_packet[1] = 0x40004984;
	send_packet.checksum = 0;
	espcomm_send_command(RAM_DOWNLOAD_END, (unsigned char*) ram_packet, 8, 0);
	return true;
}

bool espcomm_reset()
{
//	LOGDEBUG("espcomm_reset");
//	if (!espcomm_is_open) {
//		LOGDEBUG("espcomm_reset: opening port");
//		if (!serialport_open(espcomm_port, espcomm_baudrate)) {
//			LOGERR("espcomm_reset: failed to open port");
//			return false;
//		}
//	}
	espcomm_reset_to_exec();
	return true;
}
