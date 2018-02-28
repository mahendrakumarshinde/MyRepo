#ifndef IUMAX31865_H
#define IUMAX31865_H

#include <Arduino.h>
#include <SPI.h>

#include "Sensor.h"


/**
 * Temperature sensor: RTD + ADC
 *
 * Component:
 *  Name:
 *    MAX31865 + 2, 3 or 4 wires PT100
 *  Description:
 *      - temperature: a FloatFeature with section size = 1
 */
class IUMAX31865 : public LowFreqSensor
{
    public:
        /***** Preset values and default settings *****/
        // Data read addresses
        static const uint8_t CONFIG_RW            = 0x00;
        static const uint8_t RTD_MSB              = 0x01;
        static const uint8_t RTD_LSB              = 0x02;
        static const uint8_t HIGH_FAULT_MSB       = 0x03;
        static const uint8_t HIGH_FAULT_LSB       = 0x04;
        static const uint8_t LOW_FAULT_MSB        = 0x05;
        static const uint8_t LOW_FAULT_LSB        = 0x06;
        static const uint8_t FAULT_STATUS         = 0x07;
        // Configuration register bit
        static const uint8_t CFG_BIAS_BIT         = 0x80;
        static const uint8_t CFG_CONVERSION_BIT   = 0x40;
        static const uint8_t CFG_1SHOT_BIT        = 0x20;
        static const uint8_t CFG_3WIRE_BIT        = 0x10;
        static const uint8_t CFG_FAULT_BIT_1      = 0x08;
        static const uint8_t CFG_FAULT_BIT_2      = 0x04;
        static const uint8_t CFG_CLEAR_FAULT_BIT  = 0x02;
        static const uint8_t CFG_FILTER_BIT       = 0x01;
        // Fault register values and meaning
        static const uint8_t FAULT_HIGH_THRESH    = 0x80;
        static const uint8_t FAULT_LOW_THRESH     = 0x40;
        static const uint8_t FAULT_REFIN_LOW      = 0x20;
        static const uint8_t FAULT_REFIN_HIGH     = 0x10;
        static const uint8_t FAULT_RTDIN_LOW      = 0x08;
        static const uint8_t FAULT_OV_UV          = 0x04;
        static const uint8_t FAULT_NO_FAULT       = 0x00;
        enum wireCount : uint8_t {WIRE2, WIRE3, WIRE4};
        enum filterFrequency : uint8_t {HZ50, HZ60};
        enum faultDetectionMode : uint8_t {NO_DETECTION,
                                           AUTOMATIC,
                                           MANUAL1,
                                           MANUAL2};
        static const wireCount defaultWireCount = WIRE2;
        static const filterFrequency defaultFilterFrequency = HZ50;
        static const faultDetectionMode defaultFaultMode = NO_DETECTION;
        static const bool defaultBiasCorrectionEnabled = true;
        static const bool defaultAutoConversionEnabled = true;
        static const uint32_t PREPARE_1SHOT_DELAY = 70;  // ms
        /***** Constructors & desctructors *****/
        IUMAX31865(SPIClass *spiPtr, uint8_t csPin, SPISettings settings,
                   const char* name, Feature *temperature);
        virtual ~IUMAX31865() { }
        /***** Hardware and power management *****/
        virtual void setupHardware();
        virtual void wakeUp();
        virtual void sleep();
        virtual void suspend();
        /***** Configuration and calibration *****/
        virtual void switchToLowUsage();
        virtual void switchToRegularUsage();
        virtual void switchToEnhancedUsage();
        virtual void switchToHighUsage();
        void writeConfiguration();
        void setBiasCorrection(bool enable);
        void setAutoConversion(bool enable);
        void setWireCount(wireCount count);
        void setFilterFrequency(filterFrequency freq);
        void enable1ShotRead();
        void clearFault();
        /***** Data acquisition *****/
        virtual void acquireData(bool inCallback, bool force);
        virtual void readData();
        uint8_t readFault();
        void readTemperature();
        int16_t getTemperature() { return m_temperature; }
        bool isFaulty() { return m_fault != FAULT_NO_FAULT; }
        uint8_t getFaultCode() { return m_fault; }
        /***** Communication *****/
        void sendData(HardwareSerial *port);

    protected:
        /***** Core *****/
        SPIClass *m_SPI;
        uint8_t m_csPin;
        SPISettings m_spiSettings;
        /***** Hardware and power management *****/

        /***** Configuration and calibration *****/
        bool m_biasCorrectionEnabled;
        bool m_autoConversionEnabled;
        wireCount m_wireCount;
        filterFrequency m_filterFrequency;
        faultDetectionMode m_faultMode;
        /***** Data acquisition *****/
        uint8_t m_fault;
        float m_temperature;  // Temperature in degree Celsius
        bool m_onGoing1Shot;
        uint32_t m_1ShotStartTime;
        /***** SPI transactions utilities *****/
        void beginTransaction();
        void endTransaction();
        uint8_t readRegister(uint8_t address);
        void writeRegister(uint8_t address, uint8_t data);
};

#endif // IUMAX31865_H
