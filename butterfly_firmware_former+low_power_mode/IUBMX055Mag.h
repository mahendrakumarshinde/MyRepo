/*
  Infinite Uptime Firmware

  Update:
    03/03/2017

  Component:
    Name:
      BMX-055
    Description:
      Accelerometer, Gyroscope and Magnetometer for Butterfly board. Each of the sensors
      has its own registers, power management and data acquisition configuration, so each
      sensor has its own class.
      Data sheet => http://ae-bst.resource.bosch.com/media/products/dokumente/bmx055/BST-BMX055-DS000-01v2.pdf
*/

#ifndef IUBMX055MAG_H
#define IUBMX055MAG_H

#include <Arduino.h>

#include "IUABCSensor.h"
#include "IUFeature.h"
#include "IUI2C.h"


class IUBMX055Mag : public IUABCSensor
{
  public:
    enum Axis : uint8_t {X = 0,
                         Y = 1,
                         Z = 2};
    static const sensorTypeOptions sensorType = IUABCSensor::MAGNETISM;

    /*===== DEVICE CONFIGURATION AND CONSTANTS =======*/
    static const uint8_t ADDRESS   = 0x10;
    static const uint8_t WHO_AM_I  = 0x40;
    static const uint8_t I_AM      = 0x32;
    static const uint8_t PWR_CNTL1 = 0x4B; // Suspend or active / forced / sleep
    static const uint8_t PWR_CNTL2 = 0x4C; // Operation modes (active / forced / sleep)
    static const uint8_t REP_XY    = 0x51; // Repetitions (oversampling) register
    static const uint8_t REP_Z     = 0x52; // Repetitions (oversampling) register
    // ODR: Output Data Rate
    enum ODROption : uint8_t {ODR_10Hz,  // 000b (default)
                              ODR_2Hz,   // 001b
                              ODR_6Hz,   // 010b
                              ODR_8Hz,   // 011b
                              ODR_15Hz,  // 100b
                              ODR_20Hz,  // 101b
                              ODR_25Hz,  // 110b
                              ODR_30Hz}; // 111b
    enum accuracyPreset : uint8_t {LOWPOWER,        // RMS noise ~1.0 microTesla, 0.17 mA power
                                   REGULAR,         // RMS noise ~0.6 microTesla, 0.5 mA power
                                   ENHANCEDREGULAR, // RMS noise ~0.5 microTesla, 0.8 mA power
                                   HIGHACCURACY};   // RMS noise ~0.3 microTesla, 4.9 mA power
    static const ODROption defaultODR = ODR_10Hz;
    static const accuracyPreset defaultAccuracy = accuracyPreset::LOWPOWER;

    enum dataSendOption : uint8_t {dataX        = 0,
                                   dataY        = 1,
                                   dataZ        = 2,
                                   samplingRate = 3,
                                   optionCount  = 4};
    static const uint16_t defaultSamplingRate = 1000; // Hz

    // Constructor, destructor, getters and setters
    IUBMX055Mag(IUI2C *iuI2C);
    virtual ~IUBMX055Mag() {}
    virtual char getSensorType() { return (char) sensorType; }
    // Hardware & power management methods
    void softReset();
    virtual void wakeUp();
    virtual void sleep();
    virtual void suspend();
    void enterForcedMode();
    void exitForcedMode();
    void setODR(ODROption ODR);
    void setAccuracy(accuracyPreset accuracy);
    // Data acquisition methods
    virtual void readData() {}             // Not implemented
    // Communication methods
    virtual void sendToReceivers() {}      // Not implemented
    virtual void dumpDataThroughI2C() {}   // Not implemented
    virtual void dumpDataForDebugging() {} // Not implemented
    // Diagnostic Functions
    virtual void exposeCalibration() {}    // Not implemented

  protected:
    static IUI2C *m_iuI2C;
    // Configuration
    bool m_forcedMode;
    ODROption m_odr;
    accuracyPreset m_accuracy;
    void writeODRRegister();
    // Data acquisition
    static int16_t m_rawData[3];     // Stores the 13/15-bit signed magnetometer sensor output
    static q15_t m_data[3];          // Latest data values
    static q15_t m_bias[3];          // Bias corrections
};


#endif // IUBMX055MAG_H
