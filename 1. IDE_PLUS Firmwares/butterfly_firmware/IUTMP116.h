#ifndef IUTMP116_H
#define IUTMP116_H

#include <Arduino.h>
#include "Sensor.h"
#include "IUI2C.h"
#include "FeatureUtilities.h"

#define TEMP_COEFFICIENT 0.0078125
#define MIN_TEMP_RANGE -55.00
#define MAX_TEMP_RANGE 125.00

#define HIGH_ALERT_TEMP 50.00
#define LOW_ALERT_TEMP 15.00

class IUTMP116 : public LowFreqSensor
{
    public:
        static const uint8_t ADDRESS          = 0x48;           // TMP116 I2C address
        static const uint16_t I_AM            = 0x1116;         // Device ID Value read from DEVICE_ID regsiter
        static const uint8_t TEMP             = 0x00;           // Temperature register
        static const uint8_t CFGR             = 0x01;           // Configuration register
        static const uint8_t HIGH_LIM         = 0x02;           // High limit register
        static const uint8_t LOW_LIM          = 0x03;           // Low limit register
        static const uint8_t EEPROM_UL        = 0x04;           // EEPROM unlock register
        static const uint8_t DEVICE_ID        = 0x0F;           // Device ID register

	enum mode { CONTINOUS_CONVERSION,   /*MOD 00:Continous conversion(CC)*/
            	SHUTDOWN,               /*01:Shutdown(SD)*/
            	CONTINOUS_CONVERSION1,  /*10:reads back same as 00*/
            	ONE_SHOT_CONVERSION };  /*11:One-shot conversion (OS)*/
				                        // One_shot_conversion mode is not working right now.

	enum conversion { CONV_0,   /*CONV 0: afap*/
					  CONV_1,   /*CONV 1: 0.125s*/
					  CONV_2,   /*CONV 2: 0.250s*/
					  CONV_3,   /*CONV 3: 0.500s*/
					  CONV_4,   /*CONV 4: 1s*/
					  CONV_5,   /*CONV 5: 4s*/
					  CONV_6,   /*CONV 6: 8s*/
					  CONV_7 }; /*CONV 7: 16s*/

	enum average { AVG_0,   /*AVG 0: no_avg (15.5ms)*/
				   AVG_1,   /*AVG 1: 8 avg(125ms)*/
				   AVG_2,   /*AVG 2: 32 avg(500ms)*/
				   AVG_3 }; /*AVG 3: 64 avg(1000ms)*/
        
    IUTMP116(IUI2C *iuI2C, const char* name,
                    void (*i2cReadCallback)(uint8_t wireStatus),
                    FeatureTemplate<float> *temperature);
    virtual ~IUTMP116() {}

	virtual void setupHardware();
    void setPowerMode(PowerMode::option pMode);
    void setTempLimit(uint8_t reg, float value);
    void setWorkingMode(uint8_t mod, uint8_t conv, uint8_t avg);
    void processTemperatureData(uint8_t wireStatus);
    float getTemperature() { return m_temperature; }
	bool getHighAlert() { return high_alert; }
	bool getLowAlert() { return low_alert; }

	virtual void readData();
	uint16_t ReadConfigReg();
    void sendData(HardwareSerial *port);
    float getData(HardwareSerial *port);

	protected:
        /***** Core *****/
        IUI2C *m_iuI2C;
        uint8_t m_powerByte;
        void (*m_readCallback)(uint8_t wireStatus);
        float m_temperature;                      // Temperature in degree Celsius
        uint8_t m_rawBytes[2];                // 16-bit temperature data from register
        uint8_t m_rawConfigBytes[2];          // 16-bit Config register data
        bool high_alert = false;
        bool low_alert = false;
        uint16_t config_reg;
};

#endif
