#ifndef IUUSB_H
#define IUUSB_H

#include <Arduino.h>

#include <IUSerial.h>


/**
 * Class to handle USB <-> Serial communications.
 */
class IUUSB : public IUSerial
{
    public:
        /***** Core *****/
        IUUSB(HardwareSerial *serialPort, char *charBuffer,
              uint16_t bufferSize, PROTOCOL_OPTIONS protocol, uint32_t rate,
              char stopChar, uint16_t dataReceptionTimeout);
    protected:
        /***** Custom Protocol *****/
        virtual bool readCharCustomProtocol();
};

#endif // IUUSB_H
