#include "IUBMX055Gyro.h"

/* ================================= Static member definition ================================= */

IUI2C* IUBMX055Gyro::m_iuI2C = NULL;
q15_t IUBMX055Gyro::m_rawData[3] = {0, 0, 0};
q15_t IUBMX055Gyro::m_data[3] = {0, 0, 0};
q15_t IUBMX055Gyro::m_bias[3] = {0, 0, 0};


/* ================================= Method definition ================================= */

IUBMX055Gyro::IUBMX055Gyro(IUI2C *iuI2C) :
  IUABCSensor()
{
  m_iuI2C = iuI2C;
  if (!m_iuI2C->checkComponentWhoAmI("BMX055 Gyro", ADDRESS, WHO_AM_I, I_AM))
  {
    m_iuI2C->setErrorMessage("GYRERR");
    return;
  }
  m_samplingRate = defaultSamplingRate;
  softReset();
  setScale(defaultScale);
  setBandwidth(defaultBandwidth);
  setSamplingRate(defaultSamplingRate);
}


/* ============================  Hardware & power management methods ============================ */

/**
 * Soft reset gyroscope and return to normal power mode
 */
void IUBMX055Gyro::softReset()
{
  m_iuI2C->writeByte(ADDRESS,  BGW_SOFTRESET, 0xB6);
  delay(15);
}

/**
 * Set the power mode to ACTIVE (5mA)
 * Default mode after POR (Power-On Reset)
 */
void IUBMX055Gyro::wakeUp()
{
  // Write {0, 0} to {suspend (bit7), deep-suspend (bit5)}
  m_powerMode = powerMode::ACTIVE;
  m_iuI2C->writeByte(ADDRESS,  LPM1, 0x00);
}

/**
 * Set the power mode to SLEEP (25μA)
 * NB: IU SLEEP mode corresponds to the gyroscope 'suspend' mode.
 */
void IUBMX055Gyro::sleep()
{
  // Write {1, 0} to {suspend (bit7), deep-suspend (bit5)}
  m_powerMode = powerMode::SLEEP;
  m_iuI2C->writeByte(ADDRESS,  LPM1, 0x80);
}

/**
 * Set the power mode to SUSPEND (<5μA)
 * NB 1: IU SUSPEND mode corresponds to the gyroscope 'deep-suspend' mode.
 * NB 2: In SUSPEND mode configuration values are lost, they need to be re-set
 * when exiting the SUSPEND mode.
 */
void IUBMX055Gyro::suspend()
{
  // Write {0, 1} to {suspend (bit7), deep-suspend (bit5)}
  m_powerMode = powerMode::SUSPEND;
  m_iuI2C->writeByte(ADDRESS,  LPM1, 0x20);
}

/**
 * Set Fast Power Up option of SLEEP mode to true (2.5mA)
 * Fast power up, when enabled, modifies the SLEEP mode. Fast Power Up
 * must be enabled before calling sleep().
 */
void IUBMX055Gyro::enableFastPowerUp()
{
  m_fastPowerUpEnabled = true;
  sleep();
}

/**
 * Set Fast Power Up option of SLEEP mode to false (classic SLEEP MODE is then used)
 * Fast Power Up is disabled at POR (Power-On Reset)
 */
void IUBMX055Gyro::disableFastPowerUp()
{
  m_fastPowerUpEnabled = false;
  sleep();
}

/**
 * Set the scale then recompute resolution
 */
void IUBMX055Gyro::setScale(IUBMX055Gyro::scaleOption scale)
{
  m_scale = scale;
  m_iuI2C->writeByte(ADDRESS, RANGE, m_scale);
  computeResolution();
}

/**
 * Set the accelerometer bandwidth
 */
void IUBMX055Gyro::setBandwidth(IUBMX055Gyro::bandwidthOption bandwidth)
{
  m_bandwidth = bandwidth;
  m_iuI2C->writeByte(ADDRESS, BW, m_bandwidth);
}


/* ==================== Data Collection and Feature Calculation functions ======================== */

/**
 * Compute the gyroscope resolution based on the full scale range
 */
void IUBMX055Gyro::computeResolution()
{
  //TODO Implement
  m_resolution = 0;
}


/* ====================== Diagnostic Functions, only active when setupDebugMode = true ====================== */


