#ifndef IUCAMM8Q_H
#define IUCAMM8Q_H


#include <Arduino.h>

#include "IUABCSensor.h"
#include "IUI2C.h"

class IUCAMM8Q : public IUABCSensor
{
  public:
    static const uint8_t sensorTypeCount = 1;
    static char sensorTypes[sensorTypeCount];

    /*===== DEVICE CONFIGURATION AND CONSTANTS =======*/
    static const uint8_t ADDRESS        = 0x42;  // I2C address of UBLOX CAM M8Q GPS Module
    static const uint8_t I2C_BYTES      = 0xFD;  // Address of CAM M8Q number of data bytes ready register, two bytes
    static const uint8_t I2C_DATA       = 0xFF;  // Address of CAM M8Q I2C data buffer

    static const uint32_t defaultSamplingRate = 1000; // Hz
    static const uint32_t baudRate = 115200;
    enum dataSendOption : uint8_t {optionCount = 1};

    // Constructor, destructor, getters and setters
    IUCAMM8Q(IUI2C *iuI2C);
    virtual ~IUCAMM8Q() {}
    virtual uint8_t getSensorTypeCount() { return sensorTypeCount; }
    virtual char getSensorType(uint8_t index) { return sensorTypes[index]; }
    //Methods
    virtual void wakeUp();
    void initSensor();
    virtual void readData();
    virtual void sendToReceivers();
    void dumpDataThroughI2C();


  protected:
    IUI2C *m_iuI2C;
    HardwareSerial *m_port;

};

#endif // IUCAMM8Q_H
