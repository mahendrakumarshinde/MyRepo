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


void infohelper_output(int loglevel, const char* format, ...);
void infohelper_set_infolevel(char lvl);
void infohelper_output_plain(int loglevel, const char* format, ...);
int iu_main(void);
