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
    static const uint8_t sensorTypeCount = 2;
    static char  sensorTypes[sensorTypeCount];
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
    //SDO = GND  0x77 if SDO = VCC
    enum posrOptions : uint8_t {P_OSR_00 = 0, /*no op*/
                                P_OSR_01,
                                P_OSR_02,
                                P_OSR_04,
                                P_OSR_08,
                                P_OSR_16};
    enum tosrOptions : uint8_t {T_OSR_00 = 0,  /*no op*/
                                T_OSR_01,
                                T_OSR_02,
                                T_OSR_04,
                                T_OSR_08,
                                T_OSR_16};
    enum IIRFilterOptions : uint8_t {full = 0,  // bandwidth at full sample rate
                                     BW0_223ODR,
                                     BW0_092ODR,
                                     BW0_042ODR,
                                     BW0_021ODR}; // bandwidth at 0.021 x sample rate
    enum ModeOptions : uint8_t {Sleep = 0, forced, forced2, normal};
    enum SByOptions : uint8_t { t_00_5ms = 0, t_62_5ms, t_125ms, t_250ms,
                                t_500ms, t_1000ms, t_2000ms, t_4000ms,};
    enum dataSendOption : uint8_t {temperature = 0,
                                   pressure    = 1,
                                   optionCount = 2};
    static const posrOptions defaultPosr             = P_OSR_00;
    static const tosrOptions defaultTosr             = T_OSR_01;
    static const IIRFilterOptions defaultIIRFilter   = BW0_042ODR;
    static const ModeOptions defaultMode             = normal;
    static const SByOptions defaultSBy               = t_62_5ms;
    
    static const uint16_t defaultSamplingRate = 1; // Hz

    // Constructors, destructors, getters, setters
    IUBMP280(IUI2C *iuI2C);
    virtual ~IUBMP280() {}
    virtual uint8_t getSensorTypeCount() { return sensorTypeCount; }
    virtual char getSensorType(uint8_t index) { return sensorTypes[index]; }
    bool isNewData() {return m_newData; }
    int32_t getFineTemperature() { return m_fineTemperature; }
    int16_t getTemperature() { return m_temperature; }
    int16_t getPressure() { return m_pressure; }
    void setOptions(posrOptions posr, tosrOptions tosr, IIRFilterOptions iirFilter, ModeOptions mode, SByOptions sby);
    void reset();
    // Methods
    void initSensor();
    virtual void wakeUp();
    void readTemperature();
    void readPressure();
    virtual void readData();
    virtual void sendToReceivers();
    virtual void dumpDataThroughI2C();
    virtual void dumpDataForDebugging();
    int32_t readRawTemperature();
    // Diagnostic Functions
    virtual void exposeCalibration();

  protected:
    static IUI2C *m_iuI2C;
    bool m_newData;
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
    //Config variables
    posrOptions m_posr;
    tosrOptions m_tosr;
    IIRFilterOptions m_iirFilter;
    ModeOptions m_mode;
    SByOptions m_sby;
    //Internal Methods
};

#endif // IUBMP280_H
