#include "serialport.h"
#include "espcomm.h"
#include "espcomm_boards.h"
#include "iu-esptool.h"
#include <stdbool.h>
#include <stdint.h>
#include "iu_main.h"
#include <Arduino.h>
#include "stm32l4_flash.h"
#include "stm32l4_wiring_private.h"
#include "FS.h"

void setup() {
  // put your setup code here, to run once:
Serial1.begin(115200);
Serial.begin(115200);
if(!DOSFS.begin())
{
  Serial.println("Memory failed ,or not present");
  }
}

void loop() {
  delay(1000);
  Serial.println("inside the loop");
  while(Serial.available())
  {
    int val = Serial.read();
    if(val == '1')
    {
       Serial.println("ESP flash begin");
       iu_main();
      }
    }


 
  // put your main code here, to run repeatedly:

}
