#include "IUABCInterface.h"

IUABCInterface::IUABCInterface()
{
  //ctor
}

IUABCInterface::~IUABCInterface()
{
  //dtor
}

/**
 * Change the baud rate of the interface port
 * This requires to end and begin again the stream and can cause loss
 * of transmitted data.
 */
void IUABCInterface::setBaudRate( uint32_t baudRate)
{
  m_baudRate = baudRate;
  HardwareSerial *defaultPort = getPort();
  defaultPort->flush();
  delay(2);             // Wait for last byte to be sent
  defaultPort->end();
  delay(10);
  defaultPort->begin(m_baudRate);
}
