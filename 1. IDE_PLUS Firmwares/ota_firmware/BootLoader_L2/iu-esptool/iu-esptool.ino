#include "serialport.h"
#include "espcomm.h"
#include "espcomm_boards.h"
#include "iu-esptool.h"
#include "Arduino.h"
#include <stdbool.h>
#include <stdint.h>
#include "iu_main.h"

void setup() {
  // put your setup code here, to run once:
Serial1.begin(115200);
Serial.begin(115200);
}

void loop() {
  iu_main();
  // put your main code here, to run repeatedly:

}

