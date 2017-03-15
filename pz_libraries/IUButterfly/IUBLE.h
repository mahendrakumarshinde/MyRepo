/*
  Infinite Uptime Firmware

  Update:
    03/03/2017
  
  Component:
    Name:
      BMD-350
    Description:
      Bluetooth Low Energy for Butterfly board
*/

#ifndef IUBLE_H
#define IUBLE_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>
#include "IUI2C.h"

class IUBLE
{
  public:
    static constexpr HardwareSerial *port = &Serial1;
    /* do now IUBLE.port->func() instead of Serial1.func():
     * port->print
     * port->begin
     * port->flush
     * port->available
     * port->read
     */
    //Constructors, getters and setters
    IUBLE(IUI2C iuI2C);
    IUBLE(IUI2C iuI2C, uint16_t dataReceptionTimeout, int dataSendPeriod);
    uint16_t getDataReceptionTimeout() { return m_dataReceptionTimeout; }
    void setDataReceptionTimeout(uint16_t dataRecTimeout) { m_dataReceptionTimeout = dataRecTimeout; }
    void activate();
    void printBuffer(uint16_t buffSize, q15_t *buff);
    void printBuffer(uint16_t buffSize, q15_t *buff, float (*transform)(int16_t));
    void autocleanBuffer();
    void readToBuffer(void (*processBuffer) (char*));
    double getHubDatetime() { return m_hubDatetime; }
    double getDatetime();
    void updateHubDatetime();
    void updateDataSendingReceptionTime(int *bluesleeplimit);
    bool isDataSendTime();

  private:
    IUI2C m_iuI2C;
    enum : uint8_t {m_bufferSize = 19};
    char m_buffer[m_bufferSize];  // BLE Data buffer
    uint8_t m_bufferIndex; // BLE buffer Index
    //Data reception robustness variables
    int m_buffNow;
    int m_buffPrev;
    // Timestamp handling
    double m_hubDatetime; // The last timestamp received over bluetooth
    unsigned long lastReceivedDT;
    unsigned long m_lastTimer;
    uint16_t m_dataReceptionTimeout;
    int m_dataSendPeriod; // Send data every X milliseconds
        
};

#endif // IUBLE_H

