#ifndef IUCAMM8Q_H
#define IUCAMM8Q_H

#include <Arduino.h>
#include <GNSS.h>
#include "Sensor.h"
#include "IUI2C.h"


/**
 * Geolocation device
 *
 * Component:
 *  Name:
 *    CAM-M8Q
 *  Description:
 *    GNSS
 * Destinations:
 */
class IUCAMM8Q : public SynchronousSensor
{
    public:
        /***** Preset values and default settings *****/
        static const uint32_t defaultOnTime = 10;
        static const uint32_t defaultPeriod = 1000;
        static const bool defaultForcedMode = true;
        /***** Constructor and destructor *****/
        IUCAMM8Q(IUI2C *iuI2C, const char* name);
        virtual ~IUCAMM8Q() {}
        /***** Hardware & power management *****/
        virtual void setupHardware();
        virtual void wakeUp();
        virtual void sleep();
        virtual void suspend();
        /***** Configuration and calibration *****/
        virtual bool configure(JsonVariant &config);
        void setPeriodic(uint32_t onTime, uint32_t period,
                         bool forced=false);
        void enterForcedMode();
        void exitForcedMode();
        /***** Data acquisition *****/
        virtual void readData();
        /***** Communication *****/
        void sendData(HardwareSerial *port);
        /***** Debugging *****/
        virtual void exposeCalibration();

    protected:
        /***** Core *****/
        IUI2C *m_iuI2C;
        /***** Configuration and calibration *****/
        uint32_t m_onTime;
        uint32_t m_period;
        bool m_forcedMode;
        /***** Data acquisition *****/
        GNSSLocation m_location;
//    enum dataSendOption : uint8_t {latitude    = 0,
//                                   longitude   = 1,
//                                   altitude    = 2,
//                                   satellites  = 3,
//                                   pdop        = 4,
//                                   fixType     = 5,
//                                   fixQuality  = 6,
//                                   year        = 7,
//                                   month       = 8,
//                                   day         = 9,
//                                   hour        = 10,
//                                   minute      = 11,
//                                   second      = 12,
//                                   millisecond = 13,
//                                   optionCount = 14};

};

#endif // IUCAMM8Q_H
