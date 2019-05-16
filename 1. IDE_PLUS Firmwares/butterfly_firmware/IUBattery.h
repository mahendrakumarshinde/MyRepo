#ifndef IUBATTERY_H
#define IUBATTERY_H

#include <Arduino.h>
#include "Sensor.h"


/**
 * LiPo battery
 *
 * Component:
 *  Name:
 *    Battery
 *  Description:
 *    Battery + LiPo charger
 * Destinations:
 *      - batteryLoad: Float data buffer with sectionSize = 1
 */
class IUBattery : public LowFreqSensor
{
    public:
        /***** Preset values and default settings *****/
        // CHG pin to detect charging status
        static const uint8_t voltagePin  = A1;
        // Full battery voltage (mV) => 1000 * (127.0f / 100.0f) * 3.30f = 4191
        static constexpr float maxVoltage = 4191.0f;
        /***** Constructors & desctructors *****/
        IUBattery(const char* name, FeatureTemplate<float> *batteryLoad);
        virtual ~IUBattery() {}
        /***** Hardware and power management *****/
        virtual void setupHardware();
        virtual void setPowerMode(PowerMode::option pMode);
        /***** Data acquisition *****/
        virtual void readData();
        float getVoltage() { return m_vBattery; }
        float getBatteryLoad() { return m_batteryLoad; }

    protected:
        float m_vBattery;
        float m_batteryLoad;
};

#endif // IUBATTERY_H
