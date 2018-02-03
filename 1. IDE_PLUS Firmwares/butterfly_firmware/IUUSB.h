#ifndef IUUSB_H
#define IUUSB_H

#include <Arduino.h>

#include "IUSerial.h"


/**
 * Class to handle USB <-> Serial communications.
 */
class IUUSB : public IUSerial
{
    public:
        /***** Core *****/
        IUUSB(HardwareSerial *serialPort, char *charBuffer, uint16_t bufferSize,
              PROTOCOL_OPTIONS protocol, uint32_t rate=57600, char stopChar=';',
              uint16_t dataReceptionTimeout=2000);
        virtual ~IUUSB() {}

    protected:
        /***** Custom Protocol *****/
        virtual bool readCharCustomProtocol();
};


/***** Instanciation *****/

extern char iuUSBBuffer[20];
extern IUUSB iuUSB;

#endif // IUUSB_H
