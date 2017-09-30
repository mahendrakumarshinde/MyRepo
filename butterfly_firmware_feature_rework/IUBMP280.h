#ifndef IUBMP280_H
#define IUBMP280_H

#include <Arduino.h>
#include "Sensor.h"
#include "IUI2C.h"


namespace IUComponent
{
    /**
     * Barometer
     *
     * Component:
     *  Name:
     *    BMP280
     *  Description:
     *    Temperature and Pressure sensor for Butterfly board
     * Destinations:
     *      - temperature: a FloatFeature with section size = 1
     *      - pressure: a FloatFeature with section size = 1
     */
    class IUBMP280 : public Sensor
    {
        public:
            /***** Preset values and default settings *****/
            static const uint8_t WHO_AM_I           = 0xD0;
            static const uint8_t WHO_AM_I_ANSWER    = 0x58;
            static const uint8_t TEMP_MSB           = 0xFA;
            static const uint8_t PRESS_MSB          = 0xF7;
            static const uint8_t CONFIG             = 0xF5;
            static const uint8_t CTRL_MEAS          = 0xF4;
            static const uint8_t STATUS             = 0xF3;
            static const uint8_t RESET              = 0xE0;
            static const uint8_t CALIB00            = 0x88;
            static const uint8_t ADDRESS            = 0x76;
            enum powerModeBits : uint8_t {SLEEP = 0,
                                          FORCED = 1,
                                          NORMAL = 3};
            /* Oversampling improves reading accuracy and reduce noise, but
            increase the power consumption */
            enum overSamplingRates : uint8_t {SKIPPED = 0,
                                              OSR_01  = 1,
                                              OSR_02  = 2,
                                              OSR_04  = 3,
                                              OSR_08  = 4,
                                              OSR_16  = 5};
            /* Internal IIR filtering, to suppress short term disturbance (door
            slamming, wind...) */
            enum IIRFilterCoeffs : uint8_t {OFF,
                                            COEFF_02,
                                            COEFF_04,
                                            COEFF_08,
                                            COEFF_16};
            // Control inactive (standby) duration in Normal mode
            enum StandByDurations : uint8_t {t_00_5ms,
                                             t_62_5ms,
                                             t_125ms,
                                             t_250ms,
                                             t_500ms,
                                             t_1000ms,
                                             t_2000ms,
                                             t_4000ms};
            static const overSamplingRates defaultPressureOSR     = SKIPPED;
            static const overSamplingRates defaultTmperatureOSR   = OSR_01;
            static const IIRFilterCoeffs  defaultIIRFilter        = OFF;
            static const StandByDurations  defaultStandByDuration = t_62_5ms;
            static const uint16_t defaultSamplingRate = 1; // Hz
            /***** Constructors & desctructors *****/
            IUBMP280(IUI2C *iuI2C, uint8_t id=100,
                     FeatureBuffer *temperature=NULL,
                     FeatureBuffer *pressure=NULL);
            virtual ~IUBMP280() {}
            /***** Hardware and power management *****/
            void softReset();
            virtual void wakeUp();
            virtual void sleep();
            virtual void suspend();
            /***** Configuration and calibration *****/
            void setOverSamplingRates(overSamplingRates pressureOSR,
                                      overSamplingRates temperatureOSR);
            void setIIRFiltering(IIRFilterCoeffs iirFilter);
            void setStandbyDuration(StandByDurations duration);
            void calibrate();
            /***** Data acquisition *****/
            // Temperature reading
            void readTemperature();
            int32_t getFineTemperature() { return m_fineTemperature; }
            int16_t getTemperature() { return m_temperature; }
            //Pressure reading
            void readPressure();
            int16_t getPressure() { return m_pressure; }
            // Acquisition
            virtual void readData();
            /***** Communication *****/
            virtual void dumpDataThroughI2C();
            virtual void dumpDataForDebugging();
            /***** Debugging *****/
            virtual void exposeCalibration();

        protected:
            /***** Core *****/
            IUI2C *m_iuI2C;
            /***** Hardware and power management *****/
            overSamplingRates m_pressureOSR;
            overSamplingRates m_temperatureOSR;
            IIRFilterCoeffs m_iirFilter;
            StandByDurations m_standByDuration;
            void writeControlMeasureRegister();
            void writeConfigRegister();
            /***** Configuration and calibration *****/
            int16_t m_digTemperature[3];
            int16_t m_digPressure[9];
            /***** Data acquisition *****/
            // Temperature reading
            float m_temperature;  // Temperature in degree Celsius
            uint8_t m_rawTempBytes[3];  // 20-bit temperature data from register
            int32_t m_fineTemperature;
            void processTemperatureData(uint8_t wireStatus);
            float compensateTemperature(int32_t rawT);
            //Pressure reading
            float m_pressure;  // Pressure in hPa
            uint8_t m_rawPressureBytes[3];
            void processPressureData(uint8_t wireStatus);
            float compensatePressure(int32_t rawP);
    };
};

#endif // IUBMP280_H
