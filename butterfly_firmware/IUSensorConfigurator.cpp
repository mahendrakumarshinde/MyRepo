#include "IUSensorConfigurator.h"

/* ======================= Sensor Default Configuration =========================== */

// Sensors are ordered: battery, led, BMX055, BMP280, Sound
uint16_t IUSensorConfigurator::defaultSamplingRates[IUSensorConfigurator::sensorCount] = {2, 1, 1000, 2, 8000};

/* ============================== Methods =================================== */

IUSensorConfigurator::IUSensorConfigurator() :
  m_iuI2C(NULL)
{
  iuBattery = NULL;
  iuRGBLed = NULL;
  iuBMX055 = NULL;
  iuBMP280 = NULL;
  iuI2S = NULL;
  for (int i = 0; i < sensorCount; i++)
  {
    m_sensors[i] = NULL;
  }
}

IUSensorConfigurator::IUSensorConfigurator(IUI2C *iuI2C) :
  m_iuI2C(iuI2C)
{
  iuBattery = NULL;
  iuRGBLed = NULL;
  iuBMX055 = NULL;
  iuBMP280 = NULL;
  iuI2S = NULL;
  for (int i = 0; i < sensorCount; i++)
  {
    m_sensors[i] = NULL;
  }
}

IUSensorConfigurator::~IUSensorConfigurator()
{
  iuBattery = NULL;
  iuRGBLed = NULL;
  iuBMX055 = NULL;
  iuBMP280 = NULL;
  iuI2S = NULL;
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
  iuRGBLed = new IURGBLed(m_iuI2C);
  iuBMX055 = new IUBMX055(m_iuI2C);
  iuBMP280 = new IUBMP280(m_iuI2C);
  iuI2S = new IUI2S(m_iuI2C);

  m_sensors[0] = iuBattery;
  m_sensors[1] = iuRGBLed;
  m_sensors[2] = iuBMX055;
  m_sensors[3] = iuBMP280;
  m_sensors[4] = iuI2S;

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
  }
  return true;
}

void IUSensorConfigurator::wakeUpSensors()
{
  for (uint8_t i = 0; i < sensorCount; i++)
  {
    m_sensors[i]->wakeUp();
  }
}

/**
 * Have each sensor acquire data and send them to their respective receivers
 * NB: For "run" mode
 */
void IUSensorConfigurator::acquireDataAndSendToReceivers()
{
  for (uint8_t i = 0; i < sensorCount; i++)
  {
    m_sensors[i]->sendToReceivers();
    m_sensors[i]->acquireData();
  }
  /*
  Due to asynchrone data acquisition, the sendToReceivers functions actually
  send the previous data reading (to allow completion of the reading in the meantime).
  So sendToReceivers are actually called before acquireData. Each sensor handles its own data 
  availability, so nothing is sent if data is not ready.
  iuBMX055->sendToReceivers();
  iuBMX055->acquireData();
  iuBMP280->sendToReceivers();
  iuBMP280->acquireData();
  iuI2S->sendToReceivers();
  iuI2S->acquireData();
  */
}

/**
 * Have each sensor acquire data and send them through I2C
 * NB: For "data collection" mode
 * NB: in data collection kode, for 6 feature, method duration is about 17 microseconds
 */
void IUSensorConfigurator::acquireDataAndDumpThroughI2C()
{
  for (uint8_t i = 0; i < sensorCount; i++)
  {
    if(debugMode)
    {
      m_sensors[i]->dumpDataForDebugging();
    }
    else
    {
      m_sensors[i]->dumpDataThroughI2C();
    }
    m_sensors[i]->acquireData();
  }
}

/**
 * Have each sensor acquire data and send them through I2C
 * NB: For "record" mode
 */
void IUSensorConfigurator::acquireAndStoreData()
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
    debugPrint(i);
    for (int j = 0; j < m_sensors[i]->getSensorTypeCount(); j++)
    {
      debugPrint(m_sensors[i]->getSensorType(j));
    }
    m_sensors[i]->exposeCalibration();
    m_sensors[i]->exposeReceivers();
    debugPrint("\n");
  }
  #endif
}

