#ifndef IUTMP116_H
#define IUTMP116_H

#include <Arduino.h>
#include "Sensor.h"
#include "IUI2C.h"
#include "FeatureUtilities.h"


class IUTMP116 : public LowFreqSensor
{
    public:
        static const uint8_t ADDRESS          = 0x48;           // TMP116 I2C address
        static const uint8_t I_AM             = 0x48;
        static const uint8_t TEMP             = 0x00;           // Temperature register
        static const uint8_t CFGR             = 0x01;           // Configuration register
        static const uint8_t HIGH_LIM         = 0x02;           // High limit register
        static const uint8_t LOW_LIM          = 0x03;           // Low limit register
        static const uint8_t EEPROM_UL        = 0x04;           // EEPROM unlock register
        static const uint8_t DEVICE_ID        = 0x0F;           // Device ID register

        const uint8_t highlimH = B00001101;            // High byte of high lim
        const uint8_t highlimL = B10000000;            // Low byte of high lim  - High 27 C
        const uint8_t lowlimH  = B00001100;            // High byte of low lim
        const uint8_t lowlimL  = B00000000;            // Low byte of low lim   - Low 24 C
        
        IUTMP116(IUI2C *iuI2C, const char* name,
                    void (*i2cReadCallback)(uint8_t wireStatus),
                    FeatureTemplate<float> *temperature);
        virtual ~IUTMP116() {}

        virtual void setupHardware();
        virtual void setPowerMode(PowerMode::option pMode);
        virtual void setTempHighLimit();
        virtual void setTempLowLimit();

        void processTemperatureData(uint8_t wireStatus);
        float getTemperature() { return m_temperature; }

        virtual void readData();
        void sendData(HardwareSerial *port);
        float getData(HardwareSerial *port);

	protected:
        /***** Core *****/
        IUI2C *m_iuI2C;
 //       uint8_t m_powerByte;
        void (*m_readCallback)(uint8_t wireStatus);
        float m_temperature;                      // Temperature in degree Celsius
        uint8_t m_rawBytes[2];                // 16-bit temperature data from register 
};

#endif
