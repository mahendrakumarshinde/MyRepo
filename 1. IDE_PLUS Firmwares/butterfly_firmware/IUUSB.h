#ifndef IUUSB_H
#define IUUSB_H

#include <Arduino.h>

#include "IUSerial.h"


/**
 * Class to handle USB <-> Serial communications.
 */
class IUUSB : public IUSerial
{
    protected:
        /***** Custom Protocol *****/
        virtual bool readCharCustomProtocol();
};

#endif // IUUSB_H
