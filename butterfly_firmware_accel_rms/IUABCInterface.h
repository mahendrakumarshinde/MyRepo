#ifndef IUABCINTERFACE_H
#define IUABCINTERFACE_H

#include <Arduino.h>

class IUABCInterface
{
  public:
    static const uint32_t ABCDefaultBaudRate = 115200;
    static constexpr HardwareSerial *ABCPort = NULL;
    IUABCInterface();
    virtual ~IUABCInterface();
    virtual HardwareSerial* getPort() { return ABCPort; }
    virtual uint32_t getDefaultBaudRate() { return ABCDefaultBaudRate; }
    virtual void setBaudRate( uint32_t baudRate);
    virtual uint32_t getBaudRate() { return m_baudRate; }

  protected:
    uint32_t m_baudRate;
};

#endif // IUABCINTERFACE_H
