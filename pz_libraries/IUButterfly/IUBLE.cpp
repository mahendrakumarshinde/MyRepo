#include "IUBLE.h"

IUBLE::IUBLE(IUI2C iuI2C) : m_iuI2C(iuI2C), m_bufferIndex(0), m_buffNow(0), m_buffPrev(0),
                                  m_hubDatetime(0), lastReceivedDT(0), m_lastTimer(0),
                                  m_dataReceptionTimeout(2000), m_dataSendPeriod(500)
{
  // Fill out the buffer with meaningless data
  for (int i=0; i < m_bufferSize; i++)
  {
    m_buffer[i] = 'a';
  }
}

IUBLE::IUBLE(IUI2C iuI2C, uint16_t dataReceptionTimeout, int dataSendPeriod) : 
                                        m_iuI2C(iuI2C), m_bufferIndex(0), m_buffNow(0), m_buffPrev(0),
                                        m_hubDatetime(0), lastReceivedDT(0), m_lastTimer(0),
                                        m_dataReceptionTimeout(dataReceptionTimeout), m_dataSendPeriod(dataSendPeriod)
{
  // Fill out the buffer with meaningless data
  for (int i=0; i < m_bufferSize; i++)
  {
    m_buffer[i] = 'a';
  }
}

void IUBLE::activate()
{
  port->begin(9600);
}

/**
 * Print given buffer then flushes
 * @param buff the buffer
 * @param buffSize the buffer size
 */
void IUBLE::printBuffer(uint16_t buffSize, q15_t *buff)
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
void IUBLE::printBuffer(uint16_t buffSize, q15_t *buff, float (*transform) (int16_t))
{
    for (int i = 0; i < buffSize; i++)
    {
      port->print(transform(buff[i]));
      port->print(",");
      port->flush();
    }
}

/**
 * Reset the buffer if more time than m_dataReceptionTimeout passed since last reception.
 */
void IUBLE::autocleanBuffer()
{
  //Robustness for data reception//
  m_buffNow = millis();
  if (m_bufferIndex > 0) {
    if (m_buffNow -  m_buffPrev > m_dataReceptionTimeout) {
      m_bufferIndex = 0;
    }
  } else {
    m_buffPrev = m_buffNow;
  }
}

/**
 * Read available incoming bytes to fill the buffer, then process it
 * Note that the buffer can be filled and processed several times
 * @param processBuffer a function to process buffer
 */
void IUBLE::readToBuffer(void (*processBuffer) (char*))
{
  while (port->available() > 0 && m_bufferIndex < m_bufferSize)
  {
    m_buffer[m_bufferIndex] = port->read();
    m_bufferIndex++;

    if (m_bufferIndex >= m_bufferSize)
    {
      processBuffer(m_buffer);
      m_bufferIndex = 0;
    }
  }
}


//TODO: Create a class to handle all time functions

/**
 * Return the approximation of current datetime on board
 * datetime is kept updated thanks to m_hubDateyear and millis() func
 */
double IUBLE::getDatetime()
{
  unsigned long current = millis();
  return m_hubDatetime - (double) lastReceivedDT + (double) current;
}

/**
 * Update BLE componant datetime from bluetooth hub
 */
void IUBLE::updateHubDatetime()
{
  if (m_buffer[1] == ':' && m_buffer[12] == '.') {
    int date;
    int dateset;
    int dateyear;
    sscanf(m_buffer, "%d:%d.%d", &date, &dateset, &dateyear);
    m_hubDatetime = double(dateset) + double(dateyear) / double(1000000);
    lastReceivedDT = millis();
  }
}

void IUBLE::updateDataSendingReceptionTime(int *bluesleeplimit)
{
  if (m_buffer[1] == ':' && m_buffer[7] == '-' && m_buffer[13] == '-') {
    int dataRecTimeout(0), paramtag(0);
    sscanf(m_buffer, "%d:%d-%d-%d", &paramtag, &m_dataSendPeriod, bluesleeplimit, &dataRecTimeout);
    m_dataReceptionTimeout = (uint16_t) dataRecTimeout;
    m_iuI2C.port->print("Data send period is : ");
    m_iuI2C.port->println(m_dataSendPeriod);
  }
}

/**
 * Check if it is time to send data based on m_dataSendPeriod
 */
bool IUBLE::isDataSendTime()
{
  //Timer to send data regularly//
  unsigned long current = millis();
  if (current - m_lastTimer >= (unsigned long)m_dataSendPeriod) {
    m_lastTimer = current;
    return true;
  }
  return false;
}


