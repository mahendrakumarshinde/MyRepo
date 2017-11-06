/*
  Infinite Uptime Firmware

  Update:
    03/03/2017

  Component:
    Name:
      BMP280
    Description:
      Temperature and Pression sensor for Butterfly board
*/

#ifndef IUBMP280_H
#define IUBMP280_H

#include <Arduino.h>

#include "IUABCSensor.h"
#include "IUI2C.h"

class IUBMP280 : public IUABCSensor
{
  public:
    static const sensorTypeOptions sensorType = IUABCSensor::ATMOSPHERE;
    /*===== DEVICE CONFIGURATION AND CONSTANTS =======*/
    static const uint8_t WHO_AM_I           = 0xD0;
    static const uint8_t WHO_AM_I_ANSWER    = 0x58; // should be 0x58
    static const uint8_t TEMP_MSB           = 0xFA;
    static const uint8_t PRESS_MSB          = 0xF7;
    static const uint8_t CONFIG             = 0xF5;
    static const uint8_t CTRL_MEAS          = 0xF4;
    static const uint8_t STATUS             = 0xF3;
    static const uint8_t RESET              = 0xE0;
    static const uint8_t CALIB00            = 0x88;
    static const uint8_t ADDRESS            = 0x76; // Address of BMP280 altimeter
    enum powerModeBits : uint8_t {SLEEP = 0,
                                  FORCED = 1,
                                  NORMAL = 3};
    // Oversampling improves reading accuracy and reduce noise, but increase the power consumption
    enum overSamplingRates : uint8_t {SKIPPED = 0,
                                      OSR_01  = 1,
                                      OSR_02  = 2,
                                      OSR_04  = 3,
                                      OSR_08  = 4,
                                      OSR_16  = 5};
    // Internal IIR filtering, to suppress short term disturbance (door slamming, wind...)
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
    enum dataSendOption : uint8_t {temperature = 0,
                                   pressure    = 1,
                                   optionCount = 2};
    static const overSamplingRates defaultPressureOSR     = SKIPPED;
    static const overSamplingRates defaultTmperatureOSR   = OSR_01;
    static const IIRFilterCoeffs  defaultIIRFilter        = OFF;
    static const StandByDurations  defaultStandByDuration = t_62_5ms;

    static const uint16_t defaultSamplingRate = 1; // Hz

    // Constructors, destructor, getters, setters
    IUBMP280(IUI2C *iuI2C);
    virtual ~IUBMP280() {}
    virtual char getSensorType() { return (char) sensorType; }
    // Hardware and power management methods
    void softReset();
    virtual void wakeUp();
    virtual void sleep();
    virtual void suspend();
    void setOverSamplingRates(overSamplingRates pressureOSR, overSamplingRates temperatureOSR);
    void setIIRFiltering(IIRFilterCoeffs iirFilter);
    void setStandbyDuration(StandByDurations duration);
    void calibrate();
    // Data acquisition methods
    void readTemperature();
    void readPressure();
    virtual void readData();
    virtual void sendToReceivers();
    virtual void dumpDataThroughI2C();
    virtual void dumpDataForDebugging();
    int32_t readRawTemperature();
    // Communication methods
    bool isNewData() {return m_newData; }
    int32_t getFineTemperature() { return m_fineTemperature; }
    int16_t getTemperature() { return m_temperature; }
    int16_t getPressure() { return m_pressure; }
    // Diagnostic Functions
    virtual void exposeCalibration();

  protected:
    static IUI2C *m_iuI2C;
    bool m_newData;
    // Hardware configuration
    overSamplingRates m_pressureOSR;
    overSamplingRates m_temperatureOSR;
    IIRFilterCoeffs m_iirFilter;
    StandByDurations m_standByDuration;
    void writeControlMeasureRegister();
    void writeConfigRegister();
    // Temperature reading
    static uint8_t m_rawTempBytes[3]; // 20-bit temperature register data stored here
    static int32_t m_fineTemperature;
    static int16_t m_digTemperature[3];
    static float m_temperature;
    static void processTemperatureData(uint8_t wireStatus);
    static float compensateTemperature(int32_t rawT);
    //Pressure reading
    static uint8_t m_rawPressureBytes[3];
    static int16_t m_digPressure[9];
    static float m_pressure;
    static void processPressureData(uint8_t wireStatus);
    static float compensatePressure(int32_t rawP);
};

#endif // IUBMP280_H
