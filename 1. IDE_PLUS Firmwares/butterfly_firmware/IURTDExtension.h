#ifndef IURTDEXTENSION_H
#define IURTDEXTENSION_H

#ifdef RTD_DAUGHTER_BOARD // Optionnal hardware

#include <Arduino.h>
#include "Sensor.h"
#include "IUI2C.h"

/**
 * Butterfly Daughter board with RTD PT100 and their amplifiers
 *
 * Component:
 *  Name:
 *    PCA9435 (GPIO Expander) + MAX31865 (RTD Amplifiers) + PT100s
 *  Description:
 *    Precision temperature sensors extension board
 * Destinations:
 *      - temperatures: a FloatFeature with section size = rtdCount
 */
class IURTDExtension : public LowFreqSensor
{
    public:
        /***** Preset values and default settings *****/
        /* PCA9534 & PCA9534A have a different slave addresses, allowing up to
        16 devices '9534' type devices on the same I2C bus */
        static const uint8_t ADDRESS            = 0x3c;
        static const uint8_t WHO_AM_I           = 0xD0;
        static const uint8_t WHO_AM_I_ANSWER    = 0x58;
        enum expanderRegister : uint8_t {PCA9435_INPUT              = 0,
                                         PCA9435_OUTPUT             = 1,
                                         PCA9435_POLARITY_INVERSION = 2,
                                         PCA9435_CONFIGURATION      = 3};
        /***** Constructors & desctructors *****/
        IURTDExtension(IUI2C *iuI2C, const char* name, Feature *temperatures,
                       uint8_t rtdCount=4);
        virtual ~IURTDExtension() { }
        /***** Hardware and power management *****/
        virtual void setupHardware();
        /***** Configuration and calibration *****/

        /***** Data acquisition *****/
        virtual void acquireData(bool inCallback=false,
                                 bool force=false);
        virtual void readData();
        /***** Debugging *****/
        virtual void exposeCalibration();

    protected:
        /***** Core *****/
        IUI2C *m_iuI2C;
        uint8_t m_rtdCount;
        /***** GPIO Expander Communication *****/
        bool writeToExpanderRegister(uint8_t exRegister, uint8_t data);
        bool readFromExpanderRegister(uint8_t exRegister, uint8_t *destination,
                                      uint8_t byteCount);
};


/***** Instanciation *****/

extern IURTDExtension iuRTDExtension;

#endif // RTD_DAUGHTER_BOARD

#endif // IURTDEXTENSION_H
