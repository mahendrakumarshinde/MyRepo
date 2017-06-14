#include "IUSensorConfigurator.h"

/* ======================= Sensor Default Configuration =========================== */

// Sensors are ordered: battery, Accel, Gyro, Mag, BMP280, Sound, GNSS
uint16_t IUSensorConfigurator::defaultSamplingRates[IUSensorConfigurator::sensorCount] =
  {2, 1000, 2, 2, 2, 8000, 2};

powerMode::option IUSensorConfigurator::defaultPowerMode[IUSensorConfigurator::sensorCount] =
  {
    powerMode::ACTIVE,
    powerMode::ACTIVE,
    powerMode::SUSPEND,
    powerMode::SUSPEND,
    powerMode::ACTIVE,
    powerMode::ACTIVE,
    powerMode::SUSPEND
  };

/* ============================== Methods =================================== */

IUSensorConfigurator::IUSensorConfigurator() :
  m_iuI2C(NULL)
{
  resetSensorPointers();
}

IUSensorConfigurator::IUSensorConfigurator(IUI2C *iuI2C) :
  m_iuI2C(iuI2C)
{
  resetSensorPointers();
}

IUSensorConfigurator::~IUSensorConfigurator()
{
  resetSensorPointers();
}

void IUSensorConfigurator::resetSensorPointers()
{
  iuBattery = NULL;
  iuAccelerometer = NULL;
  iuGyroscope = NULL;
  iuMagnetometer = NULL;
  iuBMP280 = NULL;
  iuI2S = NULL;
  iuGNSS = NULL;
  for (int i = 0; i < sensorCount; i++)
  {
    m_sensors[i] = NULL;
  }
}

/**
 * Use defaultSensorConfigs to create all sensors and set their default config
 * Sensors are ordered: battery, led, BMX055, BMP280, Sound
 * @return true if all sensors were successfully created, else false
 */
bool IUSensorConfigurator::createAllSensorsWithDefaultConfig()
{

  iuBattery = new IUBattery(m_iuI2C);
  iuAccelerometer = new IUBMX055Acc(m_iuI2C);
  iuGyroscope = new IUBMX055Gyro(m_iuI2C);
  iuMagnetometer = new IUBMX055Mag(m_iuI2C);
  iuBMP280 = new IUBMP280(m_iuI2C);
  iuI2S = new IUI2S(m_iuI2C);
  iuGNSS = new IUCAMM8Q(m_iuI2C);

  m_sensors[0] = iuBattery;
  m_sensors[1] = iuAccelerometer;
  m_sensors[2] = iuGyroscope;
  m_sensors[3] = iuMagnetometer;
  m_sensors[4] = iuBMP280;
  m_sensors[5] = iuI2S;
  m_sensors[6] = iuGNSS;

  for (int i = 0; i < sensorCount; i++)
  {
    if (m_sensors[i] == NULL)
    {
      if (setupDebugMode)
      {
        debugPrint("Failed to create sensor #", false);
        debugPrint(i);
      }
      return false;
    }
    m_sensors[i]->setSamplingRate(defaultSamplingRates[i]);
    m_sensors[i]->switchToPowerMode(defaultPowerMode[i]);
  }
  return true;
}

/**
 * Have each sensor acquire data and send them to their respective receivers
 * NB: For "run" mode
 * @param asynchronous  if true, acquire data only from asynchronous sensors,
                        else acquire data only synchronous sensors
 */
void IUSensorConfigurator::acquireDataAndSendToReceivers(bool asynchronous)
{
  /*
  Due to asynchrone data acquisition, the sendToReceivers functions actually
  send the previous data reading (to allow completion of the reading in the meantime).
  So sendToReceivers are actually called before acquireData. Each sensor handles its own data
  availability, so nothing is sent if data is not ready.
  */
  for (uint8_t i = 0; i < sensorCount; i++)
  {
    m_sensors[i]->sendToReceivers();
    m_sensors[i]->acquireData();
  }
}

/**
 * Have each sensor acquire data and send them through I2C
 * NB: For "data collection" mode - only send Sound and Acceleration data (asynchronously)
 * NB: method duration is about 17 microseconds
 */
void IUSensorConfigurator::acquireDataAndDumpThroughI2C()
{
  // Audio data first, then Accel
  if(readableDataCollection)
  {
    iuI2S->dumpDataForDebugging();
    iuAccelerometer->dumpDataForDebugging();
  }
  else
  {
    iuI2S->dumpDataThroughI2C();
    iuAccelerometer->dumpDataThroughI2C();
  }
  iuI2S->acquireData();
  iuAccelerometer->acquireData();
}

/**
 * Have each sensor acquire data and send them through I2C
 * NB: For "record" mode
 * @param asynchronous  if true, acquire data only from asynchronous sensors,
                        else acquire data only synchronous sensors
 */
void IUSensorConfigurator::acquireAndStoreData(bool asynchronous)
{
  bool newData = false;
  IUABCSensor *sensor;
  for (uint8_t i = 0; i < sensorCount; i++)
  {
    newData = m_sensors[i]->acquireData();
    if (newData)
    {
      //TODO implement record mode
    }
  }
}

/**
 * Reset the pointers of all the sensor receivers to NULL
 */
void IUSensorConfigurator::resetAllReceivers()
{
  for (uint8_t i = 0; i < sensorCount; i++)
  {
    m_sensors[i]->resetReceivers();
  }
}


/* ====================== Power management methods ====================== */

/**
 * Wake Up all sensors and put them into ACTIVE mode
 */
void IUSensorConfigurator::allSensorsWakeUp()
{
  for (uint8_t i = 0; i < sensorCount; i++)
  {
    m_sensors[i]->wakeUp();
  }
}

/**
 * Put all sensors into SLEEP Mode
 */
void IUSensorConfigurator::allSensorsSleep()
{
  for (uint8_t i = 0; i < sensorCount; i++)
  {
    m_sensors[i]->sleep();
  }
}

/**
 * Put all sensors into SUSPEND Mode
 */
void IUSensorConfigurator::allSensorsSuspend()
{
  for (uint8_t i = 0; i < sensorCount; i++)
  {
    m_sensors[i]->suspend();
  }
}

/* ====================== Diagnostic Functions, only active when setupDebugMode = true ====================== */

/**
 * Shows all sensors and the name of features receiving their data
 */
void IUSensorConfigurator::exposeSensorsAndReceivers()
{
  #ifdef DEBUGMODE
  if (sensorCount == 0)
  {
    debugPrint("No sensor");
    return;
  }
  for (int i = 0; i <  sensorCount; i++)
  {
    debugPrint("Sensor #", false);
    debugPrint(i, false);
    debugPrint(": ", false);
    debugPrint(m_sensors[i]->getSensorType());
    m_sensors[i]->exposeCalibration();
    m_sensors[i]->exposeReceivers();
    debugPrint("\n");
  }
  #endif
}

