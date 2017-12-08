#include "IUConductor.h"


/* ============================ Constructors, destructor, getters and setters ============================ */

IUConductor::IUConductor()
{
  iuI2C = NULL;
  iuBluetooth = NULL;
  iuWifi = NULL;
  iuRGBLed = NULL;
}

IUConductor::IUConductor(String macAddress) :
  m_macAddress(macAddress),
  m_bluesleeplimit(60000),
  m_autoSleepEnabled(false),
  m_inDataAcquistion(false),
  m_lastSentTime(0),
  m_lastSynchroTime(0),
  m_usagePreset(usagePreset::OPERATION),    
  m_acquisitionMode(acquisitionMode::NONE),
  m_streamingMode(streamingMode::BLE),
  m_operationState(operationState::IDLE)
{
  iuI2C = NULL;
  iuBluetooth = NULL;
  iuWifi = NULL;
  iuRGBLed = NULL;
  m_dataSendPeriod = defaultDataSendPeriod;
  m_refDatetime = defaultTimestamp;
  setClockRate(defaultClockRate);
}

IUConductor::~IUConductor()
{
  if(iuI2C)
  {
    delete iuI2C; iuI2C = NULL;
  }
  if(iuBluetooth)
  {
    delete iuBluetooth; iuBluetooth = NULL;
  }
  if(iuWifi)
  {
    delete iuWifi; iuWifi = NULL;
  }
  if(iuRGBLed)
  {
    delete iuRGBLed; iuRGBLed = NULL;
  }
}


/* ============================ Time management methods ============================ */


/**
 * Set the master clock rate (I2S clock is used as master clock by the conductor)
 * @return true if the clock rate was correctly set, else false
 */
bool IUConductor::setClockRate(uint16_t clockRate)
{
  m_clockRate = clockRate;
  if (!sensorConfigurator.iuI2S)
  {
    // if I2S has not been initialized initialized yet, return.
    // I2S clock rate will be set when it is initialized (see initSensors)
    return false;
  }
  if (!sensorConfigurator.iuI2S->setClockRate(clockRate))
  {
    return false;
  }
  int callbackRate = sensorConfigurator.iuI2S->getCallbackRate();
  // Let the other sensors know the new callback rate
  sensorConfigurator.iuAccelerometer->setCallbackRate(callbackRate);
  sensorConfigurator.iuBMP280->setCallbackRate(callbackRate);
  sensorConfigurator.iuBattery->setCallbackRate(callbackRate);
}

void IUConductor::setDataSendPeriod(uint16_t dataSendPeriod)
{
  m_dataSendPeriod = dataSendPeriod;
  if (!iuI2C->isSilent())
  {
    iuI2C->port->print("Data send period set to : ");
    iuI2C->port->println(m_dataSendPeriod);
  }
}

/**
 * Check if it is time to send data based on m_dataSendPeriod
 * NB: Data streaming is always disabled when in "sleep" operation mode.
 */
bool IUConductor::isDataSendTime()
{
  //Timer to send data regularly//
  uint32_t current = millis();
  if (current - m_lastSentTime >= m_dataSendPeriod)
  {
    m_lastSentTime = current;
    return true;
  }
  return false;
}

/**
 *
 */
double IUConductor::getDatetime()
{
  uint32_t now = millis();
  return m_refDatetime + (double) (now - m_lastSynchroTime) / 1000.;
}


/* ============================ Usage, acquisition, power, streaming and state mgmt ============================ */

/**
 * Place the device in a short sleep (reduced power consumption but fast start up)
 */
void IUConductor::millisleep(uint32_t duration)
{
  sensorConfigurator.allSensorsSleep();
  STM32.stop(duration);
  doDefaultPowerConfig();
}

/**
 * Put the whole device to sleep for a long time (suspend the components and stop the STM32)
 */
void IUConductor::sleep(uint32_t duration)
{
  doMinimalPowerConfig();
  STM32.stop(duration);
  doDefaultPowerConfig();
  /*
  STM32.sleep();
  STM32.stop(uint32_t timeout = 0);
  STM32.standby(uint32_t timeout = 0);
  STM32.standby(uint32_t pin, uint32_t mode, uint32_t timeout = 0);
  STM32.shutdown(uint32_t timeout = 0);
  STM32.shutdown(uint32_t pin, uint32_t mode, uint32_t timeout = 0);
  */
}

void IUConductor::doMinimalPowerConfig()
{
  if (iuBluetooth->getPowerMode() != powerMode::SUSPEND) { iuBluetooth->suspend(); }
  if (iuWifi->getPowerMode() != powerMode::SUSPEND) { iuWifi->suspend(); }
  sensorConfigurator.allSensorsSuspend();
}

void IUConductor::doDefaultPowerConfig()
{
  if (iuBluetooth->getPowerMode() != powerMode::ACTIVE) { iuBluetooth->wakeUp(); }
  if (iuWifi->getPowerMode() != powerMode::SUSPEND) { iuWifi->suspend(); }
  sensorConfigurator.doDefaultPowerConfig();
}


/**
 * Automatically puts the device to sleep (if the setting is enabled) when its idle for a long time
 * 
 * NB: Autosleep is always disabled when thee streamingMode is wired (the device get continual power anyway)
 */
void IUConductor::manageAutoSleep()
{
  if (m_streamingMode == streamingMode::WIRED)
  {
    return;
  }
  if (m_autoSleepEnabled)
  {
    if (m_acquisitionMode != acquisitionMode::NONE && m_startSleepTimer < millis() - m_idleStartTime)
    {
      millisleep(m_autoSleepDuration);
    }
  }
}

/**
 * Enable auto-sleep management and set up the parameters
 * 
 * @param startSleepTimer  The timer (in ms) during which the device must stay idle before going to sleep
 * @param sleepDuration  The duration (in ms) of the sleep
 */
void IUConductor::configureAutoSleep(bool enabled, uint16_t startSleepTimer, uint32_t sleepDuration)
{
  m_autoSleepEnabled = enabled;
  m_startSleepTimer = startSleepTimer;
  m_autoSleepDuration = sleepDuration;
}

/**
 * Manage the active / sleep cycle
 * 
 * NB: Autosleep is always disabled when thee streamingMode is wired (the device get continual power anyway)
 */
void IUConductor::manageSleepCycle()
{
  uint32_t now = millis() / 1000;
  if (now > m_cycleStartTime + m_onTime)
  {
    sleep(m_cycleTime - m_onTime);
    m_cycleStartTime = millis() / 1000;
  }
}

/**
 * Enable the active / sleep cycle management and set up the active and total cycle time
 * 
 * @param onTime     the duration (in s) during which the device is active
 * @param cycleTime  the duration (in s) of the total cycle (active + sleep)
 * eg: (onTime=60, cycleTime=3600) => the device will be active 1min per hour
 */
void IUConductor::configureSleepCycle(bool enabled, uint32_t onTime, uint32_t cycleTime)
{
  m_autoSleepEnabled = enabled;
  m_onTime = onTime;
  m_cycleTime = cycleTime;
  m_cycleStartTime = millis() / 1000;
}

/**
 * Switch to a new operation mode
 * 1. When switching to "run", "record" or "data collection" mode, start data
 *    acquisition with beginDataAcquisition().
 * 2. When switching to any other modes, end data acquisition
 *    with endDataAcquisition().
 */
void IUConductor::changeAcquisitionMode(acquisitionMode::option mode)
{
  if (m_acquisitionMode == mode)
  {
    return; // Nothing to do
  }
  m_acquisitionMode = mode;
  String msg;
  // TODO => normally we should resetDataAcquisition in RAWDATA and FEATURE modes, but that blocks so far,
  // need to check why
  switch (m_acquisitionMode)
  {
    case acquisitionMode::RAWDATA:
      iuRGBLed->changeColor(IURGBLed::CYAN);
      break;
    case acquisitionMode::FEATURE:
      iuRGBLed->changeColor(IURGBLed::BLUE);
      beginDataAcquisition();
      msg = "features";
      break;
    case acquisitionMode::NONE:
      endDataAcquisition();
      msg = "nothing";
      break;
    default:
      if (loopDebugMode) { debugPrint(F("Invalid acquisition Mode")); }
      return;
  }
  if (loopDebugMode) { debugPrint("\nAcquiring " + msg + "\n"); }
}

/**
 * Switch to a streamingMode
 */
void IUConductor::changeStreamingMode(streamingMode::option mode)
{
  if (m_streamingMode == mode)
  {
    return; // Nothing to do
  }
  m_streamingMode = mode;
  String msg;
  switch (m_streamingMode)
  {
    case streamingMode::WIRED:
      iuI2C->silence();
      msg = "Serial";
      break;
    case streamingMode::BLE:
      iuI2C->unsilence();
      msg = "bluetooth";
      break;
    case streamingMode::WIFI:
      iuI2C->unsilence();
      msg = "Wifi";
      break;
    case streamingMode::STORE:
      // TODO Implement storage in SPI
      if (loopDebugMode) { debugPrint("\nNow storing data in SPI Flash\n"); }
      break;
    default:
      if (loopDebugMode) { debugPrint(F("Invalid streaming Mode")); }
      return;
  }
  if (loopDebugMode) { debugPrint("\nNow sending data over " + msg + "\n"); }
}

/**
 * Switch to a new Usage Preset
 */
void IUConductor::changeUsagePreset(usagePreset::option usage)
{
  m_usagePreset = usage;
  changeAcquisitionMode(usagePreset::acquisitionModeDetails[m_usagePreset]);
  changeStreamingMode(usagePreset::streamingModeDetails[m_usagePreset]);
  String msg;
  switch (m_usagePreset)
  {
    case usagePreset::CALIBRATION:
      setDataSendPeriod(shortestDataSendPeriod);
      featureConfigurator.setCalibrationStreaming();
      iuRGBLed->changeColor(IURGBLed::CYAN);
      iuRGBLed->lock();
      msg = "calibration";
      break;
    case usagePreset::EXPERIMENT:
      msg = "experiment";
      break;
    case usagePreset::OPERATION:
      iuRGBLed->unlock();
      iuRGBLed->changeColor(IURGBLed::BLUE);
      setDataSendPeriod(defaultDataSendPeriod);
      featureConfigurator.setStandardStreaming();
      sensorConfigurator.iuAccelerometer->resetScale();
      msg = "operation";
      break;
    default:
      if (loopDebugMode) { debugPrint(F("Invalid usage mode preset")); }
      return;
  }
  if (loopDebugMode) { debugPrint("\nSet up for " + msg + "\n"); }
}

/**
 * Switch to a new state and update the Led color accordingly
 * 
 * Also updates the idleStartTime timer for auto-sleep management
 */
void IUConductor::changeOperationState(operationState::option state)
{
  if (m_operationState == state)
  {
    return;
  }
  m_operationState = state;
  String msg = "";
  if (m_operationState == operationState::IDLE)
  {
    m_idleStartTime = millis();
    iuRGBLed->changeColor(IURGBLed::BLUE);
    msg = "Idle";
  }
  else if (m_operationState == operationState::NORMAL)
  {
    iuRGBLed->changeColor(IURGBLed::GREEN);
    msg = "Normal";
  }
  else if (m_operationState == operationState::WARNING)
  {
    iuRGBLed->changeColor(IURGBLed::ORANGE);
    msg = "Warning";
  }
  else if (m_operationState == operationState::DANGER)
  {
    iuRGBLed->changeColor(IURGBLed::RED);
    msg = "Danger";
  }
  if (loopDebugMode) { debugPrint("\nIn '" + msg + "' state\n"); }
}

/**
 * Check if the operation state needs to be updated: if yes, update it
 * The conductor operation state is based on the highest state from features.
 * To update the state, this function calls method changeOperationState.
 * operationState is only updated when in acquisitionMode::FEATURE.
 * @return true if the state was updated, else false.
 */
void IUConductor::checkAndUpdateOperationState()
{
  if (m_acquisitionMode != acquisitionMode::FEATURE)
  {
    return;
  }
  operationState::option newState = featureConfigurator.getOperationStateFromFeatures();
  if (m_operationState != newState)
  {
    changeOperationState(newState);
  }
}


/* ============================ Time management methods ============================ */

/**
 * Create and activate interfaces
 * NB: Initialization order must be: Interfaces, then Configurators, then Sensors
 * @return true if the interfaces were correctly initialized, else false
 */
bool IUConductor::initInterfaces()
{
  iuI2C = new IUI2C();
  if (!iuI2C)
  {
    iuI2C = NULL;
    return false;
  }
  iuI2C->scanDevices(); // Find components
  iuI2C->resetErrorMessage();

  iuBluetooth = new IUBMD350(iuI2C);
  if (!iuBluetooth)
  {
    iuBluetooth = NULL;
    return false;
  }
  
  iuWifi = new IUESP8285(iuI2C);
  if (!iuWifi)
  {
    iuWifi = NULL;
    return false;
  }

  // Also create and init the LED
  iuRGBLed = new IURGBLed();
  if (!iuRGBLed)
  {
    iuRGBLed = NULL;
    return false;
  }
  iuRGBLed->changeColor(IURGBLed::WHITE);

  return true;
}

/**
 * Create and activate sensors
 * NB: Initialization order must be: Interfaces, then Configurators, then Sensors
 * @return true if the Configurators were correctly initialized
 */
bool IUConductor::initConfigurators()
{
  sensorConfigurator = IUSensorConfigurator(iuI2C);
  featureConfigurator = IUFeatureConfigurator();
  return true;
}

/**
 * Create and activate sensors
 * NB: Initialization order must be: Interfaces, then Configurators, then Sensors
 * @return true if the sensors were correctly initialized, else false
 */
bool IUConductor::initSensors()
{
  bool success = sensorConfigurator.createAllSensorsWithDefaultConfig();
  if (!success)
  {
    return false;
  }
  sensorConfigurator.iuI2S->setClockRate(m_clockRate);
  return true;
}

/**
 * Link all features to all sensors
 */
bool IUConductor::linkFeaturesToSensors()
{
  return featureConfigurator.registerAllFeaturesInSensors(sensorConfigurator.getSensors(),
                                                          sensorConfigurator.sensorCount);
}

/**
 * Data acquisition function
 *
 * @param  When asynchronous is true, we use the I2S callback read data. We do this
 *         for audio and acceleration data collection. When asynchronous is False,
 *         the function should be called during the main loop runtime.
 * Method benchmarked for (accel + sound + temp) and 6 features at 10microseconds.
 */
void IUConductor::acquireAndSendData(bool asynchronous)
{
  if (!m_inDataAcquistion)
  {
    return;
  }
  if (iuI2C->getErrorMessage().equals("ALL_OK")) // Acquire data only if all ok
  {
    switch (m_acquisitionMode)
    {
      case acquisitionMode::NONE:
        break; // Do not acquire
      case acquisitionMode::RAWDATA:
        switch (m_streamingMode)
        {
          case streamingMode::WIRED:
            sensorConfigurator.dumpDataThroughI2C(asynchronous);
            // TODO: In EXPERIMENT mode, need to process I2C data here otherwise they are not properly processed
            // Can we fix this ?
            processInstructionsFromI2C();
            break;
          default:
            if (loopDebugMode) { debugPrint(F("'acquire and send data' case not implemented!")); }
        }
        sensorConfigurator.acquireData(asynchronous);
        break;
      case acquisitionMode::FEATURE:
        sensorConfigurator.sendDataToReceivers(asynchronous);
        sensorConfigurator.acquireData(asynchronous);
        break;
      default:
        if (loopDebugMode) { debugPrint(F("'acquire and send data' case not implemented!")); }
    }
  }
  else
  {
    if (debugMode)
    {
      debugPrint("I2C error: ", false);
      debugPrint(iuI2C->getErrorMessage());
    }
    iuI2C->resetErrorMessage();
    sensorConfigurator.iuI2S->readData(false);; // Empty I2S buffer to continue
  }
}

/**
 * Compute each feature that is ready to be computed
 * NB: Does nothing when not in "run" operation mode
 */
void IUConductor::computeFeatures()
{
  if (m_acquisitionMode == acquisitionMode::FEATURE)
  {
    featureConfigurator.computeAndSendToReceivers();
  }
}

/**
 * Send feature data through Serial, BLE or WiFi depending on streamingMode
 * NB: If the acquisitionMode is not FEATURE, does nothing.
 * @return true if data was sent, else false
 */
bool IUConductor::streamFeatures(bool newLine)
{
  if (!isDataSendTime() || m_acquisitionMode != acquisitionMode::FEATURE)
  {
    return false;
  }
  HardwareSerial *port = NULL;
  switch (m_streamingMode)
  {
    case streamingMode::WIRED:
      port = iuI2C->port;
      break;
    case streamingMode::BLE:
      port = iuBluetooth->port;
      break;
    case streamingMode::WIFI:
      port = iuWifi->port;
      break;
  }
  port->print(m_macAddress); port->print(",0");                     // MAC Address
  port->print((uint8_t) m_operationState); port->print(",");        // Operation State
  port->print(sensorConfigurator.iuBattery->getBatteryStatus());    // % Battery charge
  featureConfigurator.streamFeatures(port);                         // Each feature value
  port->print(","); port->print(getDatetime()); port->print(";");   // Datetime
  if (newLine)
  {
    port->println("");
  }
  port->flush();
  return true;
}

/**
 * Start data acquisition by beginning I2S data acquisition
 * NB: Other sensor data acquisition depends on I2S drumbeat
 */
bool IUConductor::beginDataAcquisition()
{
  if (m_inDataAcquistion)
  {
    return true; // Already in data acquisition
  }
  // m_inDataAcquistion should be set to true BEFORE triggering the data acquisition,
  // otherwise I2S buffer won't be emptied in time
  m_inDataAcquistion = true;
  if (!sensorConfigurator.iuI2S->triggerDataAcquisition(m_callback))
  {
    m_inDataAcquistion = false;
  }
  return m_inDataAcquistion;
}

/**
 * End data acquisition by disabling I2S data acquisition
 * NB: Other sensor data acquisition depends on I2S drumbeat
 */
void IUConductor::endDataAcquisition()
{
  if (!m_inDataAcquistion)
  {
    return; // nothing to do
  }
  sensorConfigurator.iuI2S->endDataAcquisition();
  m_inDataAcquistion = false;
  // Delay to make sure that the I2S callback function is called for the last time
  delay(50);
}

/**
 * End and restart the data acquisition
 * @return the output of beginDataAcquisition (if it was successful or not)
 */
bool IUConductor::resetDataAcquisition()
{
  endDataAcquisition();
  featureConfigurator.resetFeaturesCounters();
  delay(500);
  return beginDataAcquisition();
}

/**
 * Process the instructions sent over I2C to adjust the configuration
 */
void IUConductor::processInstructionsFromI2C()
{
  iuI2C->updateBuffer();
  String msg = iuI2C->getBuffer();
  if (msg.length() == 0)
  {
    return;
  }
  if(setupDebugMode) { debugPrint("I2C input is:"); }
  if(setupDebugMode) { debugPrint(msg); }

  // TODO => make the processing of the command not depend on the usagePreset
  
  // Usage mode Preset switching
  switch (m_usagePreset)
  {
    case usagePreset::CALIBRATION:
      if (iuI2C->checkIfEndCalibration()) { changeUsagePreset(usagePreset::OPERATION); }
      break;
    case usagePreset::EXPERIMENT:
      if (iuI2C->checkIfEndCollection())
      {
        changeUsagePreset(usagePreset::OPERATION);
        return;
      }
      if (msg.indexOf("Arange") > -1)
      {
        // Update Accel range
        int loc = msg.indexOf("Arange") + 7;
        String accelRangeStr = (String) (msg.charAt(loc));
        int accelRange = accelRangeStr.toInt();
        switch (accelRange)
        {
          case 0:
            sensorConfigurator.iuAccelerometer->setScale(sensorConfigurator.iuAccelerometer->AFS_2G);
            break;
          case 1:
            sensorConfigurator.iuAccelerometer->setScale(sensorConfigurator.iuAccelerometer->AFS_4G);
            break;
          case 2:
            sensorConfigurator.iuAccelerometer->setScale(sensorConfigurator.iuAccelerometer->AFS_8G);
            break;
          case 3:
            sensorConfigurator.iuAccelerometer->setScale(sensorConfigurator.iuAccelerometer->AFS_16G);
            break;
        }
      }
      else if (msg.indexOf("rgb") > -1)
      {
        // Change LED color
        int loc = msg.indexOf("rgb") + 7;
        int R = msg.charAt(loc) - 48;
        int G = msg.charAt(loc + 1) - 48;
        int B = msg.charAt(loc + 2) - 48;
        iuRGBLed->changeColor((bool) R, (bool) G, (bool) B);
      }
      else if (msg.indexOf("acosr") > -1)
      {
        // Change audio sampling rate
        int loc = msg.indexOf("acosr") + 6;
        int A = msg.charAt(loc) - 48;
        int B = msg.charAt(loc + 1) - 48;
        uint16_t samplingRate = (uint16_t) ((A * 10 + B) * 1000);
        sensorConfigurator.iuI2S->setSamplingRate(samplingRate);
        // Update the conductor clock rate (since it's based on I2S clock)
        setClockRate(sensorConfigurator.iuI2S->getClockRate());
      }
      else if (msg.indexOf("accsr") > -1)
      {
        int loc = msg.indexOf("accsr") + 6;
        int A = msg.charAt(loc) - 48;
        int B = msg.charAt(loc + 1) - 48;
        int C = msg.charAt(loc + 2) - 48;
        int D = msg.charAt(loc + 3) - 48;
        int samplingRate = (A * 1000 + B * 100 + C * 10 + D);
        sensorConfigurator.iuAccelerometer->setSamplingRate(samplingRate);
      }
      break;
    case usagePreset::OPERATION:
      if (iuI2C->checkIfStartCalibration()) { changeUsagePreset(usagePreset::CALIBRATION); }
      if (iuI2C->checkIfStartCollection()) { changeUsagePreset(usagePreset::EXPERIMENT); }
      break;
  }
  iuI2C->resetBuffer();    // Clear wire buffer
}

/**
 * Process the instructions sent over Bluetooth
 * 
 * NB: when streamingMode is WIRED, we ignore BLE instructions
 * Process:
 * 1. read and fill the bluetooth receiving buffer (with readToBuffer). This
 *    include a check on data reception timeout.
 * 2. scan the content of the buffer and apply changes where needed
 */
void IUConductor::processInstructionsFromBluetooth()
{
  if (m_streamingMode == streamingMode::WIRED)
  {
    return; // Do not listen to BLE when wired
  }
  char *bleBuffer = NULL;
  while (iuBluetooth->readToBuffer())
  {
    bleBuffer = iuBluetooth->getBuffer();
    if (!bleBuffer)
    {
      break;
    }
    if(loopDebugMode)
    {
      debugPrint(F("BLE input received:"), false);
      debugPrint(bleBuffer);
    }
    switch(bleBuffer[0])
    {
      case '0': // Set Thresholds
        if (bleBuffer[4] == '-' && bleBuffer[9] == '-' && bleBuffer[14] == '-')
        {
          int featureIdx = 0, newThres = 0, newThres2 = 0, newThres3 = 0;
          sscanf(bleBuffer, "%d-%d-%d-%d", &featureIdx, &newThres, &newThres2, &newThres3);
          IUFeature *feat = featureConfigurator.getFeatureById(featureIdx + 1);
          if (featureIdx == 1 || featureIdx == 2 || featureIdx == 3)
          {
            feat->setThresholds((float)newThres / 100., (float)newThres2 / 100., (float)newThres3 / 100.);
          }
          else
          {
            feat->setThresholds((float)newThres, (float)newThres2, (float)newThres3);
          }
          if (loopDebugMode)
          {
            debugPrint(feat->getName(), false); debugPrint(':', false);
            debugPrint(feat->getThreshold(0), false); debugPrint(" - ", false);
            debugPrint(feat->getThreshold(1), false); debugPrint(" - ", false);
            debugPrint(feat->getThreshold(2));
          }
        }
        break;
      case '1': // Receive the timestamp data from the bluetooth hub
        if (bleBuffer[1] == ':' && bleBuffer[12] == '.')
        {
          int flag(0), timestamp(0), microsec(0);
          sscanf(bleBuffer, "%d:%d.%d", &flag, &timestamp, &microsec);
          m_refDatetime = double(timestamp) + double(microsec) / double(1000000);
          m_lastSynchroTime = millis();
          if (loopDebugMode)
          {
            debugPrint("Time sync: ", false);
            debugPrint(getDatetime());
          }
        }
        break;

      case '2': // Bluetooth parameter setting
        if (bleBuffer[1] == ':' && bleBuffer[7] == '-' && bleBuffer[13] == '-')
        {
          int dataRecTimeout(0), paramtag(0), startSleepTimer(0), dataSendPeriod(0);
          sscanf(bleBuffer, "%d:%d-%d-%d", &paramtag, &dataSendPeriod, startSleepTimer, &dataRecTimeout);
          //TODO For now, autoSleep is deactivated => check its behavior and activate it
          configureAutoSleep(false, startSleepTimer, startSleepTimer);
          setDataSendPeriod((uint16_t) dataSendPeriod);
          iuBluetooth->setDataReceptionTimeout((uint16_t) dataRecTimeout);
        }
        break;

      case '3': // Record button pressed - go into EXTENSIVE mode to record FFTs
        if (bleBuffer[7] == '0' && bleBuffer[9] == '0' && bleBuffer[11] == '0' && bleBuffer[13] == '0' && bleBuffer[15] == '0' && bleBuffer[17] == '0')
        {
          if (loopDebugMode) { debugPrint("Record mode"); }
          IUFeature *feat = NULL;
          feat = featureConfigurator.getFeatureByName("CX3");
          if (feat)
          {
            feat->streamSourceData(iuBluetooth->port, m_macAddress, "X");
          }
          feat = featureConfigurator.getFeatureByName("CY3");
          if (feat)
          {
            feat->streamSourceData(iuBluetooth->port, m_macAddress, "Y");
          }
          feat = featureConfigurator.getFeatureByName("CZ3");
          if (feat)
          {
            feat->streamSourceData(iuBluetooth->port, m_macAddress, "Z");
          }
        }
        break;
      case '4': // Stop button pressed - go out of record mode back into FEATURE mode
        if (bleBuffer[7] == '0' && bleBuffer[9] == '0' && bleBuffer[11] == '0' && bleBuffer[13] == '0' && bleBuffer[15] == '0' && bleBuffer[17] == '0')
        {
          iuI2C->port->print("Stop recording and sending FFTs");
        }
        break;

      case '5':
        if (bleBuffer[7] == '0' && bleBuffer[9] == '0' && bleBuffer[11] == '0' && bleBuffer[13] == '0' && bleBuffer[15] == '0' && bleBuffer[17] == '0')
        {
          iuBluetooth->port->print("HB,");
          iuBluetooth->port->print(m_macAddress);
          iuBluetooth->port->print(",");
          iuBluetooth->port->print(iuI2C->getErrorMessage());
          iuBluetooth->port->print(";");
          iuBluetooth->port->flush();
        }
        break;
      case '6':
        if (bleBuffer[7] == ':' && bleBuffer[9] == '.' && bleBuffer[11] == '.' && bleBuffer[13] == '.' && bleBuffer[15] == '.' && bleBuffer[17] == '.')
        {
          int parametertag(0);
          int fcheck[6] = {0, 0, 0, 0, 0, 0};
          sscanf(bleBuffer, "%d:%d.%d.%d.%d.%d.%d", &parametertag, &fcheck[0], &fcheck[1], &fcheck[2], &fcheck[3], &fcheck[4], &fcheck[5]);
          for (uint8_t i = 0; i < 6; i++)
          {
            IUFeature *feat = featureConfigurator.getFeatureById(i + 1);
            feat->setFeatureCheck((bool) fcheck[i]);
          }
        }
        break;
      case '7': // Power mode
        if (bleBuffer[1] == ':' && bleBuffer[3] == ':' && bleBuffer[11] == ':')
        {
          int parametertag(0), enabled(0), onTime(0), cycleTime(0);
          sscanf(bleBuffer, "%d:%d:%d:%d", &parametertag, &enabled, &onTime, &cycleTime);
          configureSleepCycle((bool) enabled, onTime, cycleTime);
        }
        break;
    }
  }
}

/**
 * Process the instructions sent over WiFi
 * 
 * NB: when streamingMode is WIRED, we ignore WiFi instructions
 */
void IUConductor::processInstructionsFromWifi()
{
  if (m_streamingMode == streamingMode::WIRED)
  {
    return; // Do not listen to WiFi when wired
  }
  // TODO Implement
}

void IUConductor::setup(void (*callback)())
{
  // Interfaces
  if (setupDebugMode) { debugPrint(F("\nInitializing interfaces...")); }
  if (!initInterfaces())
  {
    if (setupDebugMode){ debugPrint(F("Failed to initialize interfaces\n")); }
    while(1);
  }
  if (setupDebugMode)
  {
    memoryLog(F("=> Successfully initialized interfaces"));
    debugPrint(' ');
    iuBluetooth->exposeInfo();
  }
  
  // Configurators
  if (setupDebugMode) { debugPrint(F("\nInitializing configurators...")); }
  if (!initConfigurators())
  {
    if (setupDebugMode) { debugPrint(F("=> Failed to initialize configurators\n")); }
    while(1);
  }
  if (setupDebugMode)
  {
    memoryLog(F("=> Successfully initialized configurators"));
  }

  // Sensors
  if (setupDebugMode) { debugPrint(F("\nInitializing sensors...")); }
  if (!initSensors())
  {
    if (setupDebugMode) { debugPrint(F("Failed to initialize sensors\n")); }
    while(1);
  }
  if (setupDebugMode)
  {
      memoryLog(F("=> Successfully initialized sensors"));
  }

  // Default feature configuration
  if (setupDebugMode) { debugPrint(F("\nSetting up default feature configuration...")); }
  if (!featureConfigurator.doStandardSetup())
  {
    if (setupDebugMode) { debugPrint(F("Failed to configure features\n")); }
    while(1);                                                // hang
  }
  if (!linkFeaturesToSensors())
  {
    if (setupDebugMode) { debugPrint(F("Failed to link feature sources to sensors\n")); }
    while(1);                                                // hang
  }
  if (setupDebugMode)
  {
    memoryLog(F("Default features succesfully configured"));
  }
  if (setupDebugMode)
  {
    sensorConfigurator.exposeSensorsAndReceivers();
    featureConfigurator.exposeFeaturesAndReceivers();
    featureConfigurator.exposeFeatureStates();

    debugPrint(F("I2C status: "), false);
    debugPrint(iuI2C->getErrorMessage());
    debugPrint(F("\nFinished setup at (ms): "), false);
    debugPrint(millis());
    debugPrint(' ');
  }

  // Power Management
  if (setupDebugMode) { debugPrint(F("\nSetting up power modes...")); }
  doDefaultPowerConfig();
  if (setupDebugMode)
  {
    memoryLog(F("=> Successfully set up power modes"));
  }

  if (setupDebugMode) { debugPrint(F("\n***Done setting up components and configurations***\n")); }

  m_callback = callback;
  changeAcquisitionMode(acquisitionMode::FEATURE);
  changeOperationState(operationState::IDLE);
}

void IUConductor::loop()
{
  // Power saving
  manageAutoSleep();
  manageSleepCycle();
  // Configuration
  processInstructionsFromI2C();
  processInstructionsFromBluetooth();
  processInstructionsFromWifi();
  // Action
  acquireAndSendData(false);       // Acquire data from synchronous sensor
  computeFeatures();               // Feature computation depending on operation mode
  checkAndUpdateOperationState();  // Update the operationState
  streamFeatures();                // Stream features
}


/* ====================== Diagnostic Functions, only active when setupDebugMode = true ====================== */

