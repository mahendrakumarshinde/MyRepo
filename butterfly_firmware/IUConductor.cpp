#include "IUConductor.h"

uint32_t IUConductor::availableClockRate[IUConductor::availableClockRateCount] =
      {8000, 12000, 16000, 24000, 32000, 48000, 96000, 11025, 22050, 44100};

IUConductor::IUConductor()
{
  iuI2C = NULL;
  iuBluetooth = NULL;
  iuWifi = NULL;
}

IUConductor::IUConductor(String macAddress) :
  m_macAddress(macAddress),
  m_bluesleeplimit(60000),
  m_autoSleepEnabled(false),
  m_opMode(operationMode::sleep),
  m_opState(operationState::idle),
  m_inDataAcquistion(false)
{
  iuI2C = NULL;
  iuBluetooth = NULL;
  iuWifi = NULL;
  m_dataSendPeriod = defaultDataSendPeriod;
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
}

/**
 *
 * @return true if the clock rate was correctly set, else false
 */
bool IUConductor::setClockRate(uint32_t clockRate)
{
  for (int i = 0; i < availableClockRateCount; i++)
  {
    if (availableClockRate[i] == clockRate)
    {
      m_clockRate = clockRate;
      if (sensorConfigurator.iuI2S)
      {
        sensorConfigurator.iuI2S->setClockRate(clockRate);
      }
      return true;
    }
  }
  return false;
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
  if (m_opMode)
  {
    return false;
  }
  //Timer to send data regularly//
  uint32_t current = (uint32_t) millis();
  if (current - m_lastSentTime >= (uint32_t) m_dataSendPeriod)
  {
    m_lastSentTime = current;
    return true;
  }
  return false;
}

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
  iuI2C->activate();
  iuI2C->scanDevices(); // Find components
  iuI2C->resetErrorMessage();
  
  iuBluetooth = new IUBMD350(iuI2C);
  if (!iuBluetooth)
  {
    iuBluetooth = NULL;
    return false;
  }
  iuBluetooth->activate();
  
  iuWifi = new IUESP8285(iuI2C);
  if (!iuWifi)
  {
    iuWifi = NULL;
    return false;
  }
  iuWifi->activate();
  
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
  sensorConfigurator.wakeUpSensors();
  return true;
}

/**
 * Link all features to all sensors
 */
void IUConductor::linkFeaturesToSensors()
{
  featureConfigurator.registerAllFeaturesInSensors(sensorConfigurator.getSensors(),
                                                   sensorConfigurator.sensorCount);
}

/**
 * Switch to a new operation mode
 * 1. When switching to "run", "record" or "data collection" mode, start data
 *    acquisition with beginDataAcquisition().
 * 2. When switching to any other modes, end data acquisition
 *    with endDataAcquisition().
 */
void IUConductor::switchToMode(operationMode mode)
{
  if (m_opMode == mode)
  {
    return; // do nothing if already in desired mode
  }
  m_opMode = mode;
  String msg;
  if (m_opMode == operationMode::charging)
  {
    endDataAcquisition();
    msg = "charging";
  }
  else if (m_opMode == operationMode::run)
  {
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::BLUE_NOOP);
    resetDataAcquisition();
    msg = "run";
  }
  else if (m_opMode == operationMode::dataCollection)
  {
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::CYAN_DATA);
    resetDataAcquisition();
    msg = "data collection";
  }
  else if (m_opMode == operationMode::configuration)
  {
    endDataAcquisition();
    msg = "configuration";
  }
  else if (m_opMode == operationMode::record)
  {
    resetDataAcquisition();
    msg = "record";
  }
  else if (m_opMode == operationMode::sleep)
  {
    m_sleepStartTime = millis();
    endDataAcquisition();
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::SLEEP_MODE);
    msg = "sleep";
  }
  if (!iuI2C->isSilent() || loopDebugMode)
  {
    iuI2C->port->print("Entering ");
    iuI2C->port->print(msg);
    iuI2C->port->println(" mode\n");
  }
}

/**
 * Switch to a new state and update the Led color accordingly
 * NB:Do not update state if:
 * - in sleep mode
 * - already in desired state
 * Also updates the idleStartTime timer
 */
void IUConductor::switchToState(operationState state)
{
  if (m_opState == state && m_opMode == operationMode::sleep)
  {
    return;
  }
  m_opState = state;
  String msg = "";
  if (m_opState == operationState::idle)
  {
    m_idleStartTime = millis(); // used for auto-sleep functionnality
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::BLUE_NOOP);
    msg = "idle";
  }
  else if (m_opState == operationState::normalCutting)
  {
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::GREEN_OK);
    msg = "normalCutting";
  }
  else if (m_opState == operationState::warningCutting)
  {
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::ORANGE_WARNING);
    msg = "warningCutting";
  }
  else if (m_opState == operationState::badCutting)
  {
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::RED_BAD);
    msg = "badCutting";
  }
  if (loopDebugMode) { debugPrint("Entering " + msg + " state\n"); }
}

/**
 * Check if the operation mode needs to be updated: if yes, update it
 * NB: To update the mode, this function calls method switchToMode.
 * @return true if the mode was updated, else false.
 */
bool IUConductor::checkAndUpdateMode()
{
  if (m_autoSleepEnabled)
  {
    if (m_opMode != operationMode::sleep && m_startSleepTimer < millis() - m_idleStartTime)
    {
      switchToMode(operationMode::sleep); // go to sleep
      return true;
    }
    else if (m_opMode == operationMode::sleep && m_opState == operationState::idle && m_endSleepTimer < millis() - m_sleepStartTime)
    {
      switchToMode(operationMode::run); // exit sleep mode into run mode
      return true;
    }
  }
  return false;
}

/**
 * Check if the operation state needs to be updated: if yes, update it
 * NB1: The conductor operation state is based on the highest state from features.
 * NB2: To update the state, this function calls method switchToState.
 * NB3: operation state update is disabled when not in "run" operation mode.
 * @return true if the state was updated, else false.
 */
bool IUConductor::checkAndUpdateState()
{
  if (m_opMode != operationMode::run)
  {
    return false;
  }
  operationState newState = featureConfigurator.getOperationStateFromFeatures();
  if (m_opState != newState)
  {
    switchToState(newState);
    return true;
  }
  return false;
}

/**
 * Data acquisition function to be used as callback
 * We use the I2S callback to do both audio and acceleration data collection
 * @return true if new data acquisition were made, else false
 * Method benchmarked for all sensors and 6 features at 10microseconds.
 */
bool IUConductor::acquireAndSendData()
{
  if (!m_inDataAcquistion) { return false; }
  bool newData = false;
  if (iuI2C->getErrorMessage().equals("ALL_OK")) // Acquire data only if all ok
  {
    if (m_opMode == operationMode::run)
    {
      sensorConfigurator.acquireDataAndSendToReceivers();
      newData = true;
    }
    if (m_opMode == operationMode::dataCollection)
    {
      sensorConfigurator.acquireDataAndDumpThroughI2C();
      newData = true;
    }
    if (m_opMode == operationMode::record)
    {
      sensorConfigurator.acquireAndStoreData();
      newData = true;
    }
  }
  else
  {
    sensorConfigurator.iuI2S->readData(); // Empty I2S buffer
  }
  return newData;
}

/**
 * Compute each feature that is ready to be computed
 * NB: Does nothing when not in "run" operation mode
 */
void IUConductor::computeFeatures()
{
  if (m_opMode == operationMode::run)
  {
    featureConfigurator.computeAndSendToReceivers();
  }
}

/**
 * Send feature data through an interface (serial, bluetooth, wifi)
 * @return true if data was sent, else false
 */
bool IUConductor::streamData()
{
  //TODO: Need to implement also for wifi and for data sending over wifi
  HardwareSerial *port;
  if (m_opMode == operationMode::run)
  {
    port = iuBluetooth->port;
  }
  else if (m_opMode == operationMode::dataCollection)
  {
    port = iuI2C->port;
  }
  else
  {
    return false;
  }
  if (isDataSendTime())
  {
    port->print(m_macAddress);                                                 // MAC Address
    port->print(",0"); port->print((int) m_opState, DEC); port->print(",");    // Operation State
    port->print(sensorConfigurator.iuBattery->getVoltage()); port->print(","); // Battery status
    featureConfigurator.streamFeatures(port);                                  // Each feature value
    port->print(","); port->print(getDatetime()); port->print(";");            // Datetime
    port->flush();                                                             // End
    return true;
  }
  return false;
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
  I2S.onReceive(m_callback); // add the receiver callback
  if (!I2S.begin(I2S_PHILIPS_MODE, m_clockRate, IUI2S::bitsPerSample)) {
    return false;
  }
  // trigger a read to kick things off
  I2S.read();
  m_inDataAcquistion = true;
  if (loopDebugMode) { debugPrint("Data acquisition triggered\n"); }
  return true;
}

/**
 * End data acquisition by disabling I2S data acquisition
 * NB: Other sensor data acquisition depends on I2S drumbeat
 */
void IUConductor::endDataAcquisition()
{
  if (m_inDataAcquistion)
  {
    I2S.end();
  }
  m_inDataAcquistion = false;
  if (loopDebugMode) { debugPrint("Data acquisition disabled"); }
}

/**
 * End and restart the data acquisition
 * @return the output of beginDataAcquisition (if it was successful or not)
 */
bool IUConductor::resetDataAcquisition()
{
  if (loopDebugMode) { debugPrint("Resetting data acquisition..."); }
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
  if (iuI2C->getBuffer().length() > 0)
  {
    if(setupDebugMode) { debugPrint("I2C input is:"); }
    if(setupDebugMode) { debugPrint(iuI2C->getBuffer()); }
    if (m_opMode == operationMode::run)
    {
      if(iuI2C->checkIfStartCollection()) // if data collection request, change the mode
      {
        switchToMode(operationMode::dataCollection);
      }
    }
    else if (m_opMode == operationMode::dataCollection)
    {
      iuI2C->printBuffer();
      //TODO REBUILD
      /*
      sensorConfigurator.iuRGBLed->updateFromI2C();            // Check for LED color update
      sensorConfigurator.iuBMX055->updateAccelRangeFromI2C();  // check for accel range update
      bool accUpdate = iuBMX055->updateSamplingRateFromI2C();  // check for accel sampling rate update
      bool audioUpdate = iuI2S->updateSamplingRateFromI2C();   // check for accel sampling rate update
      if (accUpdate || audioUpdate) //Accelerometer or Audio sampling rate changed
      {
        ACCEL_COUNTER_TARGET = iuI2S.getTargetSample() / iuBMX055.getTargetSample();
        accel_counter = 0;
        subsample_counter = 0;
      }
      */
      if (iuI2C->checkIfEndCollection()) //if data collection finished, change the mode
      {
        switchToMode(operationMode::run);
        sensorConfigurator.iuBMX055->resetAccelScale();
      }
    }
    iuI2C->resetBuffer();    // Clear wire buffer
  }
}

/**
 * Process the instructions sent over Bluetooth to adjust the configuration
 * Concretely:
 * 1. read and fill the bluetooth receiving buffer (with readToBuffer). This
 *    include a check on data reception timeout.
 * 2. scan the content of the buffer and apply changes where needed
 */
void IUConductor::processInstructionsFromBluetooth()
{
  if (m_opMode != operationMode::run)
  {
    //TODO Currently only in run mode => check why? What about the configuration mode
    //See also processInstructionFromI2C
    return;
  }
  char *bleBuffer = NULL;
  while (iuBluetooth->readToBuffer())
  {
    bleBuffer = iuBluetooth->getBuffer();
    if(setupDebugMode) { debugPrint("Bluetooth input is:"); }
    if(setupDebugMode) { debugPrint(bleBuffer); }
    if (!bleBuffer)
    {
      break;
    }
    switch(bleBuffer[0])
    {
      case 0: // Set Thresholds
        if (bleBuffer[4] == '-' && bleBuffer[9] == '-' && bleBuffer[14] == '-')
        {
          int featureIdx = 0, newThres = 0, newThres2 = 0, newThres3 = 0;
          sscanf(bleBuffer, "%d-%d-%d-%d", &featureIdx, &newThres, &newThres2, &newThres3);
          if (featureIdx == 1 || featureIdx == 2 || featureIdx == 3)
          {
            featureConfigurator.getFeature(featureIdx)->setThresholds((float)newThres / 100.,
                                                                      (float)newThres2 / 100.,
                                                                      (float)newThres3 / 100.);
          }
          else
          {
          featureConfigurator.getFeature(featureIdx)->setThresholds((float)newThres,
                                                                    (float)newThres2,
                                                                    (float)newThres3);
          }
        }
        break;
      case 1: // Receive the timestamp data from the bluetooth hub
        if (bleBuffer[1] == ':' && bleBuffer[12] == '.')
        {
          int date(0), dateset(0), dateyear(0);
          sscanf(bleBuffer, "%d:%d.%d", &date, &dateset, &dateyear);
          m_refDatetime = double(dateset) + double(dateyear) / double(1000000);
          m_lastSynchroTime = millis();
        }
        break;

      case 2: // Bluetooth parameter setting
        if (bleBuffer[1] == ':' && bleBuffer[7] == '-' && bleBuffer[13] == '-')
        {
          int dataRecTimeout(0), paramtag(0), dataSendPeriod(0);
          sscanf(bleBuffer, "%d:%d-%d-%d", &paramtag, &dataSendPeriod, m_startSleepTimer, &dataRecTimeout);
          m_endSleepTimer = m_startSleepTimer;
          setDataSendPeriod((uint16_t) dataSendPeriod);
          iuBluetooth->setDataReceptionTimeout((uint16_t) dataRecTimeout);
        }
        break;

      case 3: // Record button pressed - go into record mode to record FFTs
        //TODO Implement
        /*
        if (bleBuffer[7] == '0' && bleBuffer[9] == '0' && bleBuffer[11] == '0' && bleBuffer[13] == '0' && bleBuffer[15] == '0' && bleBuffer[17] == '0') {
          iuI2C->port->print("Time to record data and send FFTs");
          recordmode = true;
          iuBMX055->showRecordFFT(buffer_compute_index, MAC_ADDRESS);
          recordmode = false;
        }
        */
        break;
      case 4: // Stop button pressed - go out of record mode back into RUN mode
        //TODO Implement
        /*
        if (bleBuffer[7] == '0' && bleBuffer[9] == '0' && bleBuffer[11] == '0' && bleBuffer[13] == '0' && bleBuffer[15] == '0' && bleBuffer[17] == '0') {
          iuI2C->port->print("Stop recording and sending FFTs");
        }
        */
        break;

      case 5:
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

      case 6:
        //TODO: Reimplement
        if (bleBuffer[7] == ':' && bleBuffer[9] == '.' && bleBuffer[11] == '.' && bleBuffer[13] == '.' && bleBuffer[15] == '.' && bleBuffer[17] == '.')
        {
          int parametertag(0);
          int fcheck[6] = {0, 0, 0, 0, 0, 0};
          sscanf(bleBuffer, "%d:%d.%d.%d.%d.%d.%d", &parametertag, &fcheck[0], &fcheck[1], &fcheck[2], &fcheck[3], &fcheck[4], &fcheck[5]);
          for (uint8_t i = 0; i < 6; i++)
          {
            IUFeature *feat = featureConfigurator.getFeature(i);
            feat->setFeatureCheck((bool) fcheck[i]);
          }
        }
        break;
    }
  }
}

void IUConductor::processInstructionsFromWifi()
{

}

void IUConductor::printMsg(String msg)
{
  iuI2C->port->println(msg);
}
