#ifndef IUBATTERY_H
#define IUBATTERY_H

#include <Arduino.h>
#include "Sensor.h"
#include "IUI2C.h"


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
class IUBattery : public SynchronousSensor
{
    public:
        /***** Preset values and default settings *****/
        // CHG pin to detect charging status
        static const uint8_t voltagePin  = A2;
        // Full battery voltage (mV) => 1000 * (127.0f / 100.0f) * 3.30f = 4191
        static constexpr float maxVoltage = 4191.0f;
        /***** Constructors & desctructors *****/
        IUBattery(IUI2C *iuI2C, const char* name, Feature *batteryLoad=NULL);
        virtual ~IUBattery() {}
        /***** Hardware and power management *****/
        virtual void setupHardware();
        /***** Configuration and calibration *****/
        virtual void switchToLowUsage();
        virtual void switchToRegularUsage();
        virtual void switchToEnhancedUsage();
        virtual void switchToHighUsage();
        /***** Data acquisition *****/
        virtual void readData();
        float getVDDA() { return m_VDDA; }
        float getVoltage() { return m_vBattery; }
        float getBatteryLoad() { return m_batteryLoad; }

    protected:
        IUI2C *m_iuI2C;
        float m_VDDA;
        float m_vBattery;
        float m_batteryLoad;
};

#endif // IUBATTERY_H
