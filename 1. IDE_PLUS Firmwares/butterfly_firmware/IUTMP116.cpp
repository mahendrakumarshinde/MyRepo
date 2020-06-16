#include "IUTMP116.h"

extern float modbusFeaturesDestinations[8];
/* =============================================================================
    Constructors and destructors
============================================================================= */


IUTMP116::IUTMP116(IUI2C1 *iuI2C1, const char* name,
                   void (*i2cReadCallback)(uint8_t wireStatus),
                   FeatureTemplate<float> *temperature) :
    LowFreqSensor(name, 1, temperature),
    m_temperature(30.0),
    m_readCallback(i2cReadCallback)
{
    m_iuI2C1= iuI2C1;
}

/* =============================================================================
    Hardware & power management
============================================================================= */
/**
 * Set up the component and finalize the object initialization
 */

void IUTMP116::setupHardware()
{
    m_iuI2C1->readBytes(ADDRESS, DEVICE_ID, 2, &m_rawConfigBytes[0]);
    uint16_t deviceId =  ((uint16_t) (m_rawConfigBytes[0]) << 8 | m_rawConfigBytes[1]);
    if(deviceId != I_AM)
    {
        if (setupDebugMode)
        {
            debugPrint(F("Could not connect to "), false);
            debugPrint("TMP116", false);
            debugPrint("TMPERR");
            debugPrint(deviceId);
        }
        return;
    }
    if (setupDebugMode)
    {
        debugPrint("I2C1 Setup Hardware!!!");
        debugPrint("TMP116- ", false);
        debugPrint(F("DevID:0x"), false);
        debugPrint(deviceId, false);
        debugPrint(F(", I should be 0x"), false);
        debugPrint(I_AM);
    }
    setTempLimit(HIGH_LIM,HIGH_ALERT_TEMP);
    setTempLimit(LOW_LIM,LOW_ALERT_TEMP);
    setPowerMode(PowerMode::REGULAR);
}

/**
 * Set High Temperature limit
 */
uint16_t IUTMP116::ReadConfigReg()
{
    m_rawConfigBytes[0] = 0;
    m_rawConfigBytes[1] = 0;
    m_iuI2C1->readBytes(ADDRESS, CFGR, 2, &m_rawConfigBytes[0]);
    uint16_t config_value =  ((uint16_t) (m_rawConfigBytes[0]) << 8 | m_rawConfigBytes[1]);
    return config_value;           
}

void IUTMP116::setTempLimit(uint8_t reg, float value)
{
    if(value >= MIN_TEMP_RANGE && value <= MAX_TEMP_RANGE)  //TMP116 accuracy without calibration ±0.2°C (maximum) from –10°C to +85°C. (currently for +ve temp) 
    {
    	int16_t val = (int16_t) ( value/TEMP_COEFFICIENT );
    	if (!m_iuI2C1->writeWord(ADDRESS, reg, val))
    	{
        	if (debugMode) {
            	debugPrint("Temperature Limit Register Write Failure");
        	}
    	}
        if (debugMode)
        {
            uint8_t m_rawLimitBytes[2];           // 16-bit High and Low limit data from register
            if (!m_iuI2C1->readBytes(ADDRESS, reg, 2, &m_rawLimitBytes[0]))
            {
       		    debugPrint("Temperature Limit Register Read Failure");
            }
            int16_t r_val =  ((int16_t) (m_rawLimitBytes[0]) << 8 | m_rawLimitBytes[1]);
            debugPrint("Limit value After Read:");
            debugPrint(r_val);
        }
    }
    else
    {
    	if (debugMode) {
            	debugPrint("Not Valid Temperature limit range");
        }
    }
}

/**
 * Manage component power modes
 */

void IUTMP116::setPowerMode(PowerMode::option pMode)
{
    m_powerMode = pMode;    
    switch (m_powerMode)
    {
        case PowerMode::PERFORMANCE:                  // MOD 00:Continous conversion(CC) 01:Shutdown(SD) 10:reads back same as 00 11:One-shot conversion (OS)
        case PowerMode::ENHANCED:                     // CONV 0: afap, 1: 0.125s, 2: 0.250s, 3: 0.500s, 4: 1s, 5: 4s, 6: 8s, 7: 16s
        case PowerMode::REGULAR:                      // AVG  0: no_avg (15.5ms), 1: 8 avg(125ms), 2: 32 avg(500ms), 3: 64 avg(1000ms)
        case PowerMode::LOW_1:
        case PowerMode::LOW_2:
            setWorkingMode( CONTINOUS_CONVERSION1, CONV_4, AVG_0 ); 
            break;
        case PowerMode::SLEEP:
        case PowerMode::DEEP_SLEEP:
        case PowerMode::SUSPEND:
            setWorkingMode( SHUTDOWN, CONV_4, AVG_0 );
            // TODO: Implement
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unhandled power Mode "), false);
                debugPrint(m_powerMode);
            }
            break;
    }
}

void IUTMP116::setWorkingMode(uint8_t mod, uint8_t conv, uint8_t avg)
{
    m_iuI2C1->readBytes(ADDRESS, CFGR, 2, &m_rawConfigBytes[0]);
    uint16_t config_value =  ((uint16_t) (m_rawConfigBytes[0]) << 8 | m_rawConfigBytes[1]);
    uint8_t data[3];

    config_value = config_value | (mod & 0x03 ) << 10; //2 bits of mod go to position 11:10
    config_value = config_value | (conv & 0x07) << 7;  //3 of conv go to 9:7
    config_value = config_value | (avg & 0x03 ) << 5;  //2 of avg go to 6:5
    config_value = config_value | 1 << 3;              //always in make alert pin active high
   
    data[0] = (uint8_t) (config_value >> 8 ) & 0x0F;
    data[1] = (uint8_t) (data[0] & 0xFF);
    data[2] = (uint8_t)( config_value & 0xFF);

    config_value = (data[1]) << 8 | data[2];
    if (!m_iuI2C1->writeWord(ADDRESS, CFGR, config_value))
    {
        if (asyncDebugMode) {
            debugPrint("TMP116 write configuration failure");
        }
    }    
}

/***** Temperature Reading *****/
void IUTMP116::readData()
{
    m_rawBytes[0] = 0;
    m_rawBytes[1] = 0;
    iuI2C1.releaseReadLock();
    if (m_readingData) {
        // previous data reading is not done: flag errors in destinations and
        // skip reading
        // for (uint8_t i = 0; i < m_destinationCount; ++i) {
        //     //m_destinations[i]->flagDataError();
        // }
        if (debugMode) {
            debugPrint("Skip TMP116 read");
        }
    } else if (m_iuI2C1->readBytes(ADDRESS, TEMP, 2, &m_rawBytes[0],m_readCallback))
    {
        m_readingData = true;
        config_reg = ReadConfigReg();
        if((config_reg >> 15) & 0x01)
        {
            high_alert = true;
            low_alert = false;
            if (debugMode) {
                debugPrint("high alert");
            }
        }
        else
        {
            high_alert = false;
        }
        if((config_reg >> 14) & 0x01)
        {
            high_alert = false;
            low_alert = true;
            if (debugMode) {
                debugPrint("low alert");
            }
        } else
        {
            low_alert = false;
        }
    }else
    {
        if (debugMode) 
        {
            debugPrint("Temperature Read Failure");
        }
    }
   

    m_readingData = false;
}
/**
 * Process a raw Temperature reading
 */
void IUTMP116::processTemperatureData(uint8_t wireStatus)
{
    int iTemp = 0;
    iuI2C1.releaseReadLock();
    if (wireStatus != 0) {
        if (asyncDebugMode || debugMode) {
            debugPrint(micros(), false);
            debugPrint(F(" Temperature processing read error "), false);
            debugPrint(wireStatus);
            debugPrint("Applying Default temperature value ");
        }
        m_defaultTemperature = 56;
        //return;
    }
    
    m_temperature = 0.00;
    iTemp = ( (int16_t) (m_rawBytes[0] << 8) | m_rawBytes[1]);
    m_temperature = (iTemp * TEMP_COEFFICIENT);
    // TODO : Apply Temperature Algorithm here.
     m_temperature = m_temperature -  quadraticTemperatureCoorection(m_temperature);
     m_destinations[0]->addValue(m_temperature + m_defaultTemperature);
     //Append the Temperature data
     modbusFeaturesDestinations[5] = m_temperature + m_defaultTemperature;
}

/**
   Temperature data to serial

 * If loopDebugMode is true, will send humand readable content, else nothing.
 */

void IUTMP116::sendData(HardwareSerial *port)
{
    if (loopDebugMode)  // Human readable in the console
    {
        port->print("T: ");
        port->println(m_temperature, 2); // UPTO 2 decimal
    }else {

        float temp;
        byte* data;
        for (uint8_t i = 0; i < 2; ++i) {
            temp = m_temperature;
            data = (byte*) & temp;
            port->write(data, 4);
        }
    }
}

float IUTMP116::getData(HardwareSerial *port)
{
    if (loopDebugMode)  // Human readable in the console
    {
        port->print("T: ");
        port->println(m_temperature, 2); // UPTO 2 decimal
    }
    else 
    {
        return m_temperature;        
    }
}

/*
Linear-> (-0.11356094436553277) x + (8.340067532958916)

Quadratic -> (-0.000703201760759331) x^2 + (-0.03676297298757314) x + (6.469553023053652)

Cubic -> params: (-1.2353858157606742e-05) x^3 + (0.001307898233064794) x^2 + (-0.13881873878918416) x + (8.074500101831555)

Quartic -> params: (-3.077936514743978e-06) x^4 + (0.000650987627860888) x^3 + (-0.04968278164742072) x^2 + (1.5083763886028987) x + (-10.736161400887005)

Quintic -> params: (-4.1418566896899394e-08) x^5 + (8.129792039345673e-06) x^4 + -0.0005168360615951368 x^3 + (0.008647487003413851) x^2 + (0.11705963338766216) x + (1.9054682507634597)

*/


float IUTMP116::linearTemperatureCorrection(float m_temperature){
    //float inputTemperature;
    float outputFactor=0;

     outputFactor = (-0.11356094436553277) * m_temperature + (8.340067532958916);

return outputFactor; 
}

float IUTMP116::quadraticTemperatureCoorection(float m_temperature){
    //float inputTemperature;
    float outputFactor=0;

    outputFactor = (-0.000703201760759331)*pow(m_temperature,2) + (-0.03676297298757314)*m_temperature + (6.469553023053652);
    
return outputFactor;
}

float IUTMP116::cubicTemperatureCorrection(float m_temperature){
    //float inputTemperature;
    float outputFactor=0;

    outputFactor = (-1.2353858157606742e-05)*pow(m_temperature,3) + (0.001307898233064794)*pow(m_temperature,2) + (-0.13881873878918416)*m_temperature + (8.074500101831555);

return outputFactor;

}

float IUTMP116::quarticTemperatureCorrection(float m_temperature){
    //float inputTemperature;
    float outputFactor=0;

    outputFactor = (-3.077936514743978e-06)*pow(m_temperature,4) + (0.000650987627860888)*pow(m_temperature,3) + (-0.04968278164742072)*pow(m_temperature,2) + (1.5083763886028987)*m_temperature + (-10.736161400887005);

return outputFactor;

}

float IUTMP116::quinticTemperatureCorrection(float m_temperature){
    //float inputTemperature;
    float outputFactor=0;

    outputFactor = (-4.1418566896899394e-08)*pow(m_temperature,5) + (8.129792039345673e-06)*pow(m_temperature,4)+ (-0.0005168360615951368) *pow(m_temperature,3) + (0.008647487003413851)*pow(m_temperature,2) + (0.11705963338766216)*m_temperature + (1.9054682507634597);

return outputFactor;
}