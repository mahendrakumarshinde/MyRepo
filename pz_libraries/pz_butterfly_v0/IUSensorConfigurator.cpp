#include "IUSensorConfigurator.h"

/* ======================= Sensor Default Configuration =========================== */

IUSensorConfigurator::sensorConfig IUSensorConfigurator::noSensor = {IUABCSensor::sensorType_none, "", 1};

IUSensorConfigurator::sensorConfig IUSensorConfigurator::defaultSensorConfigs[IUSensorConfigurator::defaultSensorCount] =
{
  {IUABCSensor::sensorType_battery,       "IUBattery",  2},
  {IUABCSensor::sensorType_rgbled,        "IURGBLed",   1},
  {IUABCSensor::sensorType_accelerometer, "IUBMX055",   2},
  {IUABCSensor::sensorType_thermometer,   "IUBMP280",   2},
  {IUABCSensor::sensorType_microphone,    "IUI2S",      2},
};

/* ============================== Methods =================================== */

IUSensorConfigurator::IUSensorConfigurator() :
  m_iuI2C(NULL),
  m_sensorCount(0)
{
  //ctor
}

IUSensorConfigurator::IUSensorConfigurator(IUI2C *iuI2C) :
  m_iuI2C(iuI2C),
  m_sensorCount(0)
{
  //ctor
}

IUSensorConfigurator::~IUSensorConfigurator()
{
  for (uint8_t i = 0; i < defaultSensorCount; i++)
  {
    delete m_sensors[i]; m_sensors[i] = NULL;
  }
  delete iuBattery; iuBattery = NULL;
  delete iuBattery; iuRGBLed = NULL;
  delete iuBattery; iuBMX055 = NULL;
  delete iuBattery; iuBMP280 = NULL;
  delete iuBattery; iuI2S = NULL;
}

/**
 * Return the sensor corresponging to the given sensorType
 * NB: Sensors are stored in m_sensors in the order defined by static
 * member defaultSensorConfigs.
 */
IUABCSensor* IUSensorConfigurator::getSensorFromType(char sensorType)
{
  uint8_t idx = 0;
  for (uint8_t i = 0; i < defaultSensorCount; i++)
  {
    if (defaultSensorConfigs[i].sensorType == sensorType)
    {
      return m_sensors[i];
    }
  }
  return NULL;
}

/**
 * Create and add a sensor to the instance sensor list
 * @return true if sensor was successfully added, else false
 */
bool IUSensorConfigurator::addSensor(IUSensorConfigurator::sensorConfig sconfig)
{
  if (sconfig.className == "IUBattery")
  {
    iuBattery = new IUBattery(m_iuI2C);
    if (!iuBattery)
    {
      iuBattery = NULL;
      return false;
    }
    m_sensors[m_sensorCount] = iuBattery;
  }
  else if (sconfig.className == "IURGBLed")
  {
    iuRGBLed = new IURGBLed(m_iuI2C);
    if (!iuRGBLed)
    {
      iuRGBLed = NULL;
      return false;
    }
    m_sensors[m_sensorCount] = iuRGBLed;
  }
  else if (sconfig.className == "IUBMX055")
  {
    iuBMX055 = new IUBMX055(m_iuI2C);
    if (!iuBMX055)
    {
      iuBMX055 = NULL;
      return false;
    }
    m_sensors[m_sensorCount] = iuBMX055;
  }
  else if (sconfig.className == "IUBMP280")
  {
    iuBMP280 = new IUBMP280(m_iuI2C);
    if (!iuBMP280)
    {
      iuBMP280 = NULL;
      return false;
    }
    m_sensors[m_sensorCount] = iuBMP280;;
  }
  else if (sconfig.className == "IUI2S")
  {
    iuI2S = new IUI2S(m_iuI2C);
    if (!iuI2S)
    {
      iuI2S = NULL;
      return false;
    }
    m_sensors[m_sensorCount] = iuI2S;
  }
  m_sensorCount++;
  return true;
}

/**
 * Use defaultSensorConfigs to create all sensors and set their default config
 * @return true if all sensors were successfully created, else false
 */
bool IUSensorConfigurator::createAllSensorsWithDefaultConfig()
{
  bool success = true;
  for (uint8_t i = 0; i < defaultSensorCount; i++)
  {
    success = addSensor(defaultSensorConfigs[i]);
    if (!success)
    {
      return false;
    }
    m_sensors[i]->setSamplingRate(defaultSensorConfigs[i].samplingRate);
  }
  return true;
}

void IUSensorConfigurator::wakeUpSensors()
{
  for (uint8_t i = 0; i < m_sensorCount; i++)
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
  bool newData = false;
  for (uint8_t i = 0; i < defaultSensorCount; i++)
  {
    newData = m_sensors[i]->acquireData();
    if (newData)
    {
      m_sensors[i]->sendToReceivers();
    }
  }
}

/**
 * Have each sensor acquire data and send them through I2C
 * NB: For "data collection" mode
 */
void IUSensorConfigurator::acquireDataAndDumpThroughI2C()
{
  bool newData = false;
  for (uint8_t i = 0; i < defaultSensorCount; i++)
  {
    newData = m_sensors[i]->acquireData();
    if (newData)
    {
      m_sensors[i]->dumpDataThroughI2C();
    }
  }
}

/**
 * Have each sensor acquire data and send them through I2C
 * NB: For "record" mode
 */
void IUSensorConfigurator::acquireAndStoreData()
{
  bool newData = false;
  for (uint8_t i = 0; i < defaultSensorCount; i++)
  {
    newData = m_sensors[i]->acquireData();
    if (newData)
    {
      //TODO implement record mode
    }
  }
}
