#include "IUBMD350.h"

IUBMD350::IUBMD350(IUI2C *iuI2C) : IUInterface(), m_iuI2C(iuI2C), m_bufferIndex(0),
                                   m_dataReceptionTimeout(2000), m_lastReadTime(0)
{
  // Fill out the buffer with meaningless data
  for (int i=0; i < bufferSize; i++)
  {
    m_buffer[i] = 'a';
  }
}

void IUBMD350::activate()
{
  port->begin(9600);
}

/**
 * Reset the buffer if more time than m_dataReceptionTimeout passed since last reception.
 * @return true if a timeout happened and that the buffer was reset, else false.
 */
bool IUBMD350::checkDataReceptionTimeout()
{
  if (m_bufferIndex > 0 && (millis() -  m_lastReadTime > m_dataReceptionTimeout))
  {
    m_bufferIndex = 0;
    return true;
  }
  return false;
}

/**
 * Read available incoming bytes to fill the buffer
 * @param processBuffer a function to process buffer
 * @return true if the buffer is full and ready to be read, else false
 * NB1: if there are a lot of incoming data, the buffer should be
 * filled and processed several times.
 * NB2: a data reception timeout check is performed before reading (see
 * checkDataReceptionTimeout).
 */
bool IUBMD350::readToBuffer()
{
  if (m_bufferIndex == bufferSize)
  {
      m_bufferIndex = 0; // reset the buffer if it was previously filled
  }
  checkDataReceptionTimeout();
  while (port->available() > 0)
  {
    m_buffer[m_bufferIndex] = port->read();
    m_bufferIndex++;
    m_lastReadTime = millis();
    if (m_bufferIndex == bufferSize)
    {
      return true; // buffer is full => return true to warn it's ready to be processed
    }
  }
  return false;
}


/**
 * Print given buffer then flushes
 * @param buff the buffer
 * @param buffSize the buffer size
 */
void IUBMD350::printFigures(uint16_t buffSize, q15_t *buff)
{
  for (int i = 0; i < buffSize; i++)
  {
    port->print(buff[i]);
    port->print(",");
    port->flush();
  }
}

/**
 * Print given buffer then flushes
 * @param buff the buffer
 * @param buffSize the buffer size
 * @param transform a transfo to apply to each buffer element before printing
 */
void IUBMD350::printFigures(uint16_t buffSize, q15_t *buff, float (*transform) (int16_t))
{
    for (int i = 0; i < buffSize; i++)
    {
      port->print(transform(buff[i]));
      port->print(",");
      port->flush();
    }
}
