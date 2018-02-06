#ifndef IUESP8285_H
#define IUESP8285_H

#include <Arduino.h>

#include "IUSerial.h"
#include "Component.h"

/**
 * Wifi chip
 *
 * Component:
 *   Name:
 *   ESP-8285
 * Description:
 *
 */
class IUESP8285 : public IUSerial, public Component
{
    public:
        /***** Preset values and default settings *****/
//        static const uint8_t UART_TX_PIN = D9;
        /***** Constructors & desctructors *****/
        IUESP8285(HardwareSerial *serialPort, char *charBuffer,
                  uint16_t bufferSize, PROTOCOL_OPTIONS protocol,
                  uint32_t rate=115200, uint16_t dataReceptionTimeout=2000);
        virtual ~IUESP8285() {}
        /***** Hardware and power management *****/
        virtual void setupHardware();
        virtual void wakeUp();
        virtual void sleep();
        virtual void suspend();
        /***** Communication with WiFi chip *****/
        void sendBleMacAddress(char *macAddress);
        void preventFromSleeping() { port->print("WIFI-NOSLEEP;"); }
        void authorizeSleeping() { port->print("WIFI-SLEEPOK;"); }
};

#endif // IUESP8285_H
