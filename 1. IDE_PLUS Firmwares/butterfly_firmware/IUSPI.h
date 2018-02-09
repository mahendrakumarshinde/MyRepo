#ifndef IUSPI_H
#define IUSPI_H

#include <Arduino.h>
#include <SPI.h>

#include "Logger.h"

class IUSPI
{
    public:
        /***** Core *****/
        IUSPI(SPIClass *spiPtr, uint8_t csPin, SPISettings settings);
        virtual ~IUSPI() {}
        virtual void begin();
        /***** Transactions utilities *****/
        virtual void beginTransaction();
        virtual void endTransaction();
        /***** Convenient synchronous read, write and transfer *****/
        uint8_t transactionTransfer(uint8_t data);
        uint16_t transactionTransfer16(uint16_t data);
        void transactionTransfer(void *buffer, size_t count);
        // STM32L4 EXTENSTION: synchronous inline read/write, transfer with
        // separate read/write buffer
        virtual uint8_t transactionRead();
        virtual void transactionRead(void *buffer, size_t count);
        virtual void transactionWrite(uint8_t data);
        virtual void transactionWrite(const void *buffer, size_t count);
        virtual void transactionTransfer(const void *txBuffer, void *rxBuffer,
                                         size_t count);

    protected:
        SPIClass *m_SPI;
        uint8_t m_csPin;
        SPISettings m_spiSettings;
};

#endif // IUSPI_H
