#include "IUMAX31865.h"


/* =============================================================================
    Constructors & desctructors
============================================================================= */

IUMAX31865::IUMAX31865(SPIClass *spiPtr, uint8_t csPin, SPISettings settings,
                       const char* name, Feature *temperature) :
    LowFreqSensor(name, 1, temperature),
    m_csPin(csPin),
    m_spiSettings(settings),
    m_biasCorrectionEnabled(defaultBiasCorrectionEnabled),
    m_autoConversionEnabled(defaultAutoConversionEnabled),
    m_wireCount(defaultWireCount),
    m_filterFrequency(defaultFilterFrequency),
    m_faultMode(defaultFaultMode),
    m_fault(FAULT_NO_FAULT),
    m_temperature(28),
    m_onGoing1Shot(false),
    m_1ShotStartTime(0)
{
    m_SPI = spiPtr;
}


/* =============================================================================
    Hardware and power management
============================================================================= */

/**
 *
 */
void IUMAX31865::setupHardware()
{
    pinMode(m_csPin, OUTPUT);
    digitalWrite(m_csPin, HIGH);
    m_SPI->begin();
    delayMicroseconds(10);
    writeConfiguration();
    wakeUp();
    switchToRegularUsage();
    delay(1000);
}

/**
 *
 */
void IUMAX31865::wakeUp()
{
    LowFreqSensor::wakeUp();
}

/**
 *
 */
void IUMAX31865::sleep()
{
    LowFreqSensor::sleep();
    // TODO Implement
}

/**
 *
 */
void IUMAX31865::suspend()
{
    LowFreqSensor::suspend();
    // TODO Implement
}


/* =============================================================================
    Configuration and calibration
============================================================================= */


void IUMAX31865::switchToLowUsage()
{
    setSamplingPeriod(30000);
    LowFreqSensor::switchToLowUsage();
}

void IUMAX31865::switchToRegularUsage()
{
    setSamplingPeriod(5000);
    LowFreqSensor::switchToRegularUsage();
}

void IUMAX31865::switchToEnhancedUsage()
{
    setSamplingPeriod(2000);
    LowFreqSensor::switchToEnhancedUsage();
}

void IUMAX31865::switchToHighUsage()
{
    setSamplingPeriod(1000);
    LowFreqSensor::switchToHighUsage();
}

void IUMAX31865::writeConfiguration()
{
    uint8_t c = readRegister(CONFIG_RW);
    // V Bias
    if (m_biasCorrectionEnabled) { c |= CFG_BIAS_BIT; }
    else { c &= ~CFG_BIAS_BIT; }
    // Conversion Mode (turn off to use 1 shot mode)
    if (m_autoConversionEnabled) { c |= CFG_CONVERSION_BIT; }
    else { c &= ~CFG_CONVERSION_BIT; }
    // Number of wires on PT-100
    if (m_wireCount == wireCount::WIRE3) { c |= CFG_3WIRE_BIT; }
    else { c &= ~CFG_3WIRE_BIT; }
    switch (m_faultMode)
    {
        case NO_DETECTION:
            c &= ~CFG_FAULT_BIT_1;
            c &= ~CFG_FAULT_BIT_2;
            break;
        case AUTOMATIC:
            c &= ~CFG_FAULT_BIT_1;
            c |= CFG_FAULT_BIT_2;
            break;
        case MANUAL1:
            c |= CFG_FAULT_BIT_1;
            c &= ~CFG_FAULT_BIT_2;
            break;
        case MANUAL2:
            c |= CFG_FAULT_BIT_1;
            c |= CFG_FAULT_BIT_2;
            break;
    }
    // Noise filtering - Select input voltage frequency
    if (m_filterFrequency == HZ50) { c |= CFG_FILTER_BIT; }
    else { c &= ~CFG_FILTER_BIT; }
    writeRegister(CONFIG_RW, c);
    delay(2000);
    c = readRegister(CONFIG_RW);
}

/**
 *
 */
void IUMAX31865::setBiasCorrection(bool enable)
{
    m_biasCorrectionEnabled = enable;
    writeConfiguration();
}

/**
 *
 */
void IUMAX31865::setAutoConversion(bool enable)
{
    m_autoConversionEnabled = enable;
    writeConfiguration();
}

/**
 *
 */
void IUMAX31865::setWireCount(IUMAX31865::wireCount count)
{
    m_wireCount = count;
    writeConfiguration();
}

/**
 *
 */
void IUMAX31865::setFilterFrequency(IUMAX31865::filterFrequency freq)
{
    m_filterFrequency = freq;
    writeConfiguration();
}

/**
 *
 */
void IUMAX31865::enable1ShotRead()
{
    if (m_onGoing1Shot)
    {
        return;
    }
    uint8_t c = readRegister(CONFIG_RW);
    c |= CFG_1SHOT_BIT;
    writeRegister(CONFIG_RW, c);
    m_onGoing1Shot = true;
    m_1ShotStartTime = millis();
}

/**
 *
 */
void IUMAX31865::clearFault()
{
    uint8_t c = readRegister(CONFIG_RW);
    c |= CFG_CLEAR_FAULT_BIT;
    writeRegister(CONFIG_RW, c);
    m_fault = FAULT_NO_FAULT;
}


/* =============================================================================
    Data acquisition
============================================================================= */


void IUMAX31865::acquireData(bool inCallback, bool force)
{
    // Read temperature if 1 shot read is ready
    if (m_onGoing1Shot)
    {
        uint32_t now = millis();
        if (now - m_1ShotStartTime > PREPARE_1SHOT_DELAY)
        {
            readTemperature();
        }
    }
    // Acquire new data
    LowFreqSensor::acquireData(inCallback, force);
}

/**
 *
 */
void IUMAX31865::readData()
{
//    clearFault();
//    delay(10);
    if (!m_autoConversionEnabled)
    {
        enable1ShotRead();
    }
    else
    {
        readTemperature();
    }
}

/**
 *
 */
void IUMAX31865::readTemperature()
{
    // Read 2 bytes RTD,taking into account that last bit is the fault marker
    beginTransaction();
    m_SPI->transfer(RTD_MSB);
    uint8_t rtdMSB = m_SPI->read();
    uint8_t rtdLSB = m_SPI->read();
    endTransaction();
    uint16_t rtd = ((uint16_t) rtdMSB << 8) | rtdLSB;
    rtd >>= 1;
    m_temperature = ((float) rtd / 32.0) - 256.0;
    m_destinations[0]->addFloatValue(m_temperature);
    m_onGoing1Shot = false;
}

/**
 *
 */
uint8_t IUMAX31865::readFault()
{
    beginTransaction();
    m_SPI->transfer(FAULT_STATUS);
    m_fault = m_SPI->read();
    endTransaction();
    return m_fault;
}


/* =============================================================================
    Communication
============================================================================= */

/**
 *
 */
void IUMAX31865::sendData(HardwareSerial *port)
{
    if (loopDebugMode)  // Human readable in the console
    {
        port->print("RTD T: ");
        port->println(m_temperature, 4);
    }
}


/* =============================================================================
    SPI transactions utilities
============================================================================= */

void IUMAX31865::beginTransaction()
{
    m_SPI->beginTransaction(m_spiSettings);
    digitalWrite(m_csPin, LOW);
}

void IUMAX31865::endTransaction()
{
    digitalWrite(m_csPin, HIGH);
    m_SPI->endTransaction();
}

uint8_t IUMAX31865::readRegister(uint8_t address)
{
    // Read address all have 1st bit = 0.
    address &= ~0x80;
    beginTransaction();
    m_SPI->transfer(address);
    uint8_t c = m_SPI->read();
    endTransaction();
    return c;
}

void IUMAX31865::writeRegister(uint8_t address, uint8_t data)
{
    // Write address all have 1st bit = 1.
    address |= 0x80;
    beginTransaction();
    m_SPI->transfer(address);
    m_SPI->write(data);
    endTransaction();
}
