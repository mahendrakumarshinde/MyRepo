#ifndef IUESP8285_H
#define IUESP8285_H

#include <Arduino.h>
#include "Host_WiFiserial.h"
#include "IUI2C.h"


namespace IUComponent
{
    /**
     * Wifi chip
     *
     * Component:
     *   Name:
     *   ESP-8285
     * Description:
     *
     */
    class IUESP8285 : public ABCComponent
    {
    public:
        static constexpr HardwareSerial *port = &Serial3;
        /***** Constructors & desctructors *****/
        IUESP8285(IUI2C *iuI2C);
        virtual ~IUESP8285() {}
        /***** Communication with components *****/
        virtual void wakeUp();
        virtual void sleep();
        virtual void suspend();
        /***** WiFi Communication *****/
        virtual void setBaudRate(uint32_t baudRate);
        virtual uint32_t getBaudRate() { return m_baudRate; }

    protected:
        IUI2C *m_iuI2C;
        uint32_t m_baudRate;
    };

    /***** Instanciation *****/
    extern IUESP8285 iuWiFi;
};

#endif // IUESP8285_H
