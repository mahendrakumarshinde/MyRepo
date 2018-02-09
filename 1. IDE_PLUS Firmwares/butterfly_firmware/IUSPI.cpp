#include "IUSPI.h"


/* =============================================================================
    Core
============================================================================= */

IUSPI::IUSPI(SPIClass *spiPtr, uint8_t csPin, SPISettings settings) :
    m_csPin(csPin),
    m_spiSettings(settings)
{
    m_SPI = spiPtr;
}

/**
 *
 */
void IUSPI::begin()
{
    pinMode(m_csPin, OUTPUT);
    digitalWrite(m_csPin, HIGH);
    m_SPI->begin();
}


/* =============================================================================
    Transactions utilities
============================================================================= */

/**
 *
 */
void IUSPI::beginTransaction()
{
    m_SPI->beginTransaction(m_spiSettings);
    digitalWrite(m_csPin, LOW);
}

/**
 *
 */
void IUSPI::endTransaction()
{
    digitalWrite(m_csPin, HIGH);
    m_SPI->endTransaction();
}


/* =============================================================================
    Convenient synchronous read, write and transfer
============================================================================= */


uint8_t IUSPI::transactionTransfer(uint8_t data)
{
    beginTransaction();
    uint8_t c = m_SPI->transfer(data);
    endTransaction();
    return c;
}

uint16_t IUSPI::transactionTransfer16(uint16_t data)
{
    beginTransaction();
    uint16_t c = m_SPI->transfer16(data);
    endTransaction();
    return c;
}

void IUSPI::transactionTransfer(void *buffer, size_t count)
{
    beginTransaction();
    m_SPI->transfer(buffer, count);
    endTransaction();
}


/**
 *
 */
uint8_t IUSPI::transactionRead()
{
    beginTransaction();
    uint8_t c = m_SPI->read();
    endTransaction();
    return c;
}

/**
 *
 */
void IUSPI::transactionRead(void *buffer, size_t count)
{
    beginTransaction();
    m_SPI->read(buffer, count);
    endTransaction();
}

/**
 *
 */
void IUSPI::transactionWrite(uint8_t data)
{
    beginTransaction();
    m_SPI->write(data);
    endTransaction();
}

/**
 *
 */
void IUSPI::transactionWrite(const void *buffer, size_t count)
{
    beginTransaction();
    m_SPI->write(buffer, count);
    endTransaction();
}

/**
 *
 */
void IUSPI::transactionTransfer(const void *txBuffer, void *rxBuffer,
                                size_t count)
{
    beginTransaction();
    m_SPI->transfer(txBuffer, rxBuffer, count);
    endTransaction();
}
