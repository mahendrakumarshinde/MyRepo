#include "IUCAMM8Q.h"

char IUCAMM8Q::sensorTypes[IUCAMM8Q::sensorTypeCount] =
{
  IUABCSensor::sensorType_gps
};

IUCAMM8Q::IUCAMM8Q(IUI2C *iuI2C) :
  IUABCSensor(),
  m_iuI2C(iuI2C),
  m_port(&Serial2)
{
  // ctor
}

void IUCAMM8Q::wakeUp()
{
  m_port->begin(baudRate);
}

void IUCAMM8Q::initSensor()
{

}

void IUCAMM8Q::readData()
{
  while (m_port->available())
  {
    m_iuI2C->port->write( m_port->read() );
  }
}
