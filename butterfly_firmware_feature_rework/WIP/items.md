
**1. For message Serialization, check Avro, thrift (FB), others ?**


**2. Feature Streaming:**

/***** Streaming *****/
virtual void stream(HardwareSerial *port);

/***** Streaming *****/
/**
 * Stream feature id and value through given port
 * @return true if value was available and streamed, else false
 */
void FeatureBuffer::stream(HardwareSerial *port)
{
  if (m_id < 10)
  {
    port->print("000");
  }
  else
  {
    port->print("00");
  }
  port->print(m_id);
  port->print(",");
  port->print(m_referenceValue);
}
// Use this?
bool m_streamingAcknowledged[maxSectionCount];



**3. When do we call, for each FeatureBuffer:**
- publishIfReady
- acknowledgeIfReady


**4. I2C message interpretation:**
const String START_COLLECTION = "IUCMD_START";
const String START_CALIBRATION = "IUCAL_START";
const String START_CONFIRM = "IUOK_START";
const String END_COLLECTION = "IUCMD_END";
const String END_CALIBRATION = "IUCAL_END";
const String END_CONFIRM = "IUOK_END";
bool checkIfStartCalibration();
bool checkIfEndCalibration();
bool checkIfStartCollection();
bool checkIfEndCollection();

/**
 * Check if we should start calibration
 */
bool IUI2C::checkIfStartCalibration()
{
  if (m_wireBuffer.indexOf(START_CALIBRATION) > -1)
  {
    port->println(START_CONFIRM);
    return true;
  }
  return false;
}

/**
 * Check if we should end calibration
 */
bool IUI2C::checkIfEndCalibration()
{
  if (m_wireBuffer.indexOf(END_CALIBRATION) > -1)
  {
    port->println(END_CONFIRM);
    return true;
  }
  return false;
}

/**
 * Check if we should start data collection, (ie enter collection mode)
 */
bool IUI2C::checkIfStartCollection()
{
  if (m_wireBuffer.indexOf(START_COLLECTION) > -1)
  {
    port->print(START_CONFIRM); // No new line !!
    return true;
  }
  return false;
}

/**
 * Check if we should end data collection, (ie return run mode)
 */
bool IUI2C::checkIfEndCollection()
{
  if (m_wireBuffer == END_COLLECTION)
  {
    port->println(END_CONFIRM);
    return true;
  }
  return false;
}


**5. Resolve how to use the following in Sensors:**

/***** Communication *****/
// May be defined in Child class
virtual void dumpDataThroughI2C() {}
virtual void dumpDataForDebugging() {}
/***** Debugging *****/
// May be defined in Child class
virtual void exposeCalibration() {}


**6. For sensors, When to set up resolution, sampling rate and callback rate?**


**7. CAM-M8Q GNSS => the feature format is not a base type, it is GNSSLocation type**
How to handle the value propagation through the features / featureComputers?


**8. m_audioData is Q31 but audio feature is currently Q15 => Sort this out**


**9. The function updateSamplingRateFromI2C in I2S is a "receive configuration from I2C" type of function**
=> remove it from I2S and put it in the configuration module.


**20. Stop and restart I2S**
This was not working previously. What about now?
Clarify the process and document it.


**21. Info about RAM selection (among other)**
https://github.com/GrumpyOldPizza/arduino-STM32L4/blob/master/variants/STM32L432KC-NUCLEO/linker_scripts/STM32L432KC_FLASH.ld


