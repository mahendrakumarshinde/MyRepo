#ifndef IUBMP280_H
#define IUBMP280_H

#include <Arduino.h>

#include "IUUtilities.h"
#include "IUI2CTeensy.h"
#include "IUBLE.h"

class IUBMP280
{
  public:
    /*===== DEVICE CONFIGURATION AND CONSTANTS =======*/
    static const uint8_t WHO_AM_I           = 0xD0;
    static const uint8_t WHO_AM_I_ANSWER    = 0x58; // should be 0x58
    static const uint8_t TEMP_XLSB          = 0xFC;
    static const uint8_t TEMP_LSB           = 0xFB;
    static const uint8_t TEMP_MSB           = 0xFA;
    static const uint8_t PRESS_XLSB         = 0xF9;
    static const uint8_t PRESS_LSB          = 0xF8;
    static const uint8_t PRESS_MSB          = 0xF7;
    static const uint8_t CONFIG             = 0xF5;
    static const uint8_t CTRL_MEAS          = 0xF4;
    static const uint8_t STATUS             = 0xF3;
    static const uint8_t RESET              = 0xE0;
    static const uint8_t CALIB00            = 0x88;
    static const uint8_t ADDRESS            = 0x76; // Address of BMP280 altimeter
    //SDO = GND  0x77 if SDO = VCC
    enum posrOptions : uint8_t {P_OSR_00 = 0, /*no op*/ P_OSR_01, P_OSR_02,
                                P_OSR_04, P_OSR_08, P_OSR_16};
    enum tosrOptions : uint8_t {T_OSR_00 = 0,  /*no op*/ T_OSR_01, T_OSR_02,
                                T_OSR_04, T_OSR_08, T_OSR_16};
    enum IIRFilterOptions : uint8_t {full = 0,  // bandwidth at full sample rate
                                     BW0_223ODR,
                                     BW0_092ODR,
                                     BW0_042ODR,
                                     BW0_021ODR}; // bandwidth at 0.021 x sample rate
    enum ModeOptions : uint8_t {Sleep = 0, forced, forced2, normal};
    enum SByOptions : uint8_t { t_00_5ms = 0, t_62_5ms, t_125ms, t_250ms, 
                                t_500ms, t_1000ms, t_2000ms, t_4000ms,};

    // Constructors, destructors, getters, setters
    IUBMP280(IUI2CTeensy iuI2C, IUBLE iuBLE);
    IUBMP280(IUI2CTeensy iuI2C, IUBLE iuBLE, posrOptions posr, tosrOptions tosr, 
              IIRFilterOptions iirFilter, ModeOptions mode, SByOptions sby);
    virtual ~IUBMP280();
    int32_t getFineTemperature() { return m_fineTemperature; }
    int16_t getTemperature() { return m_temperature; }
    int16_t getPressure() { return m_pressure; }
    void setOptions(posrOptions posr, tosrOptions tosr, IIRFilterOptions iirFilter, ModeOptions mode, SByOptions sby);

    // Methods
    void initSensor();
    void wakeUp();
    float getCurrentTemperature();
    int32_t readRawTemperature();
    int32_t compensateTemperature(int32_t rawT);
    int32_t readRawPressure();
    uint32_t compensatePressure(int32_t rawP);

  private:
    IUI2CTeensy m_iuI2C;
    IUBLE m_iuBLE;
    int32_t m_fineTemperature; 
    int16_t m_digTemperature[3];
    int16_t m_digPressure[9];
    int16_t m_temperature, m_pressure;
    uint8_t m_posr, m_tosr, m_iirFilter, m_mode, m_sby;

};

#endif // IUBMP280_H
