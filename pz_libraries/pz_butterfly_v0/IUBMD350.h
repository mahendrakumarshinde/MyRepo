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

#ifndef IUBMD350_H
#define IUBMD350_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>
#include "IUI2C.h"
#include "IUInterface.h"

class IUBMD350 : public IUInterface
{
  public:
    static constexpr HardwareSerial *port = &Serial1;
    static const uint8_t bufferSize = 19;
    //Constructors, getters and setters
    IUBMD350(IUI2C *iuI2C);
    virtual ~IUBMD350() {}
    uint16_t getDataReceptionTimeout() { return m_dataReceptionTimeout; }
    void setDataReceptionTimeout(uint16_t dataRecTimeout) { m_dataReceptionTimeout = dataRecTimeout; }
    void activate();
    char* getBuffer() { return m_buffer; }
    // Methods
    bool checkDataReceptionTimeout();
    bool readToBuffer();
    void printFigures(uint16_t buffSize, q15_t *buff);
    void printFigures(uint16_t buffSize, q15_t *buff, float (*transform)(int16_t));

  private:
    IUI2C *m_iuI2C;
    char m_buffer[bufferSize];  // BLE Data buffer
    uint8_t m_bufferIndex; // BLE buffer Index
    //Data reception robustness variables: Buffer is cleansed if now - lastReadTime > dataReceptionTimeout
    uint16_t m_dataReceptionTimeout; // ms
    uint32_t m_lastReadTime;

};

#endif // IUBMD350_H

