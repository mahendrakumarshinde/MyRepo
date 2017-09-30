#ifndef IUBATTERY_H
#define IUBATTERY_H

#include <Arduino.h>
#include "Sensor.h"
#include "IUI2C.h"


namespace IUComponent
{
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
    class IUBattery : public Sensor
    {
        public:
            /***** Preset values and default settings *****/
            // CHG pin to detect charging status
            static const uint8_t voltagePin  = A2;
            static const uint16_t maxVoltage = 4100; // mV full battery voltage
            /***** Constructors & desctructors *****/
            IUBattery(IUI2C *iuI2C, uint8_t id=100,
                      FeatureBuffer *batteryLoad=NULL);
            virtual ~IUBattery() {}
            /***** Data acquisition *****/
            virtual void readData();
            float getVDDA() { return m_VDDA; }
            float getVoltage() { return m_vBattery; }
            uint8_t getBatteryLoad() { return m_batteryLoad; }

        protected:
            IUI2C *m_iuI2C;
            float m_VDDA;
            float m_vBattery;
            float m_batteryLoad;
    };
};

#endif // IUBATTERY_H
