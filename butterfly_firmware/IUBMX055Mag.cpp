#include "IUBMX055Mag.h"

/* ================================= Static member definition ================================= */
IUI2C* IUBMX055Mag::m_iuI2C = NULL;
q15_t IUBMX055Mag::m_rawData[3] = {0, 0, 0};
q15_t IUBMX055Mag::m_data[3] = {0, 0, 0};
q15_t IUBMX055Mag::m_bias[3] = {0, 0, 0};

/* ================================= Method definition ================================= */

/**
 *
 * NB: The WHO AM I does not work when in SUSPEND mode, so the BMX055 magnetometer needs to
 * be soft-reset or waked up first.
 */
IUBMX055Mag::IUBMX055Mag(IUI2C *iuI2C) :
  IUABCSensor(),
  m_odr(defaultODR),
  m_accuracy(defaultAccuracy)
{
  m_iuI2C = iuI2C;
  softReset();
  if (!m_iuI2C->checkComponentWhoAmI("BMX055 Mag", ADDRESS, WHO_AM_I, I_AM))
  {
    m_iuI2C->setErrorMessage("MAGERR");
    return;
  }
  setSamplingRate(defaultSamplingRate);
}


/* ============================  Hardware & power management methods ============================ */

/**
 * Soft reset
 * NB: Soft reset always bring the magnetometer into sleep mode
 * Write 1 to both bit7 and bit1 from power control byte 0x4B
 */
void IUBMX055Mag::softReset()
{
  m_iuI2C->writeByte(ADDRESS, PWR_CNTL1, 0x82);
  delay(50);
}

/**
 * Set the power mode to ACTIVE (between 170μA and 4.9mA)
 * By default, the device will be waked up and put into forced mode (so it
 * will actually be asleep until a record is requested or until exitForcedMode
 * is called).
 */
void IUBMX055Mag::wakeUp()
{
  m_powerMode = powerMode::ACTIVE;
  m_iuI2C->writeByte(ADDRESS, PWR_CNTL1, 0x01);
  enterForcedMode();
}

/**
 * Set the power mode to SLEEP
 * NB: This is the default mode after soft-reset
 */
void IUBMX055Mag::sleep()
{
  m_powerMode = powerMode::SLEEP;
  m_iuI2C->writeByte(ADDRESS, PWR_CNTL1, 0x01);
  m_iuI2C->writeByte(ADDRESS, PWR_CNTL2, ((uint8_t) m_odr << 3) | 0x06);
}

/**
 * Set the power mode to SUSPEND (11μA)
 * This is the default after Power-On Reset
 */
void IUBMX055Mag::suspend()
{
  m_powerMode = powerMode::SUSPEND;
  m_iuI2C->writeByte(ADDRESS, PWR_CNTL1, 0x00);
}

/**
 * Enter Forced Mode => on demand data acquisition
 */
void IUBMX055Mag::enterForcedMode()
{
  m_forcedMode = true;
  writeODRRegister();
}

/**
 * Exit Forced Mode and go into Active Mode => records will be sample at set ODR
 * NB: ODR = Output Data Rate
 */
void IUBMX055Mag::exitForcedMode()
{
  m_forcedMode = false;
  writeODRRegister();
}

/**
 * Set the output data rate - only relevant in Normal (non-forced) mode
 */
void IUBMX055Mag::setODR(IUBMX055Mag::ODROption ODR)
{
  m_odr = ODR;
  if (m_forcedMode && debugMode)
  {
    debugPrint("Set Magnetometer ODR but Forced Mode is active: ODR will be ignored");
  }
  writeODRRegister();
}

void IUBMX055Mag::writeODRRegister()
{
  uint8_t opModeBit = (uint8_t) m_forcedMode;
  m_iuI2C->writeByte(ADDRESS, PWR_CNTL2, ((uint8_t) m_odr << 3) | (opModeBit << 1));

}

/**
 * Set the accuracy from an accuracyPreset, by changing the oversampling on XY and Z axis
 */
void IUBMX055Mag::setAccuracy(IUBMX055Mag::accuracyPreset accuracy)
{
  m_accuracy = accuracy;
  switch (m_accuracy)
  {
    case accuracyPreset::LOWPOWER:
      m_iuI2C->writeByte(ADDRESS, REP_XY, 0x01);  // 3 repetitions (oversampling)
      m_iuI2C->writeByte(ADDRESS, REP_Z,  0x02);  // 3 repetitions (oversampling)
      break;
    case accuracyPreset::REGULAR:
      m_iuI2C->writeByte(ADDRESS, REP_XY, 0x04);  //  9 repetitions (oversampling)
      m_iuI2C->writeByte(ADDRESS, REP_Z,  0x16);  // 15 repetitions (oversampling)
      break;
    case accuracyPreset::ENHANCEDREGULAR:
      m_iuI2C->writeByte(ADDRESS, REP_XY, 0x07);  // 15 repetitions (oversampling)
      m_iuI2C->writeByte(ADDRESS, REP_Z,  0x22);  // 27 repetitions (oversampling)
      break;
    case accuracyPreset::HIGHACCURACY:
      m_iuI2C->writeByte(ADDRESS, REP_XY, 0x17);  // 47 repetitions (oversampling)
      m_iuI2C->writeByte(ADDRESS, REP_Z,  0x51);  // 83 repetitions (oversampling)
      break;
  }
}

/* ==================== Data Collection and Feature Calculation functions ======================== */


/* ====================== Diagnostic Functions, only active when setupDebugMode = true ====================== */



