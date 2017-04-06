#include "IUI2S.h"

/* =============================== Static members =============================== */

uint16_t IUI2S::availableClockRate[IUI2S::availableClockRateCount] =
      {8000, 12000, 16000, 24000, 32000, 48000, 11025, 22050, 44100};

char IUI2S::sensorTypes[IUI2S::sensorTypeCount] = {IUABCSensor::sensorType_microphone};


/* ============================= Method definitions ============================= */

IUI2S::IUI2S(IUI2C *iuI2C) :
  IUABCSensor(),
  m_iuI2C(iuI2C),
  m_firstI2STrigger(true),
  m_newData(false)
{
  setClockRate(defaultClockRate);
}

/**
 * Set the clock rate: Audio sampling is based on this rate, and drives the callback
 * @return  true if the given clockrate is allowed and has been set.
 *          false if the callback rate is not allowed.
 */
bool IUI2S::setClockRate(uint16_t clockRate)
{
  
  for (int i = 0; i < availableClockRateCount; i++)
  {
    if (availableClockRate[i] == clockRate)
    {
      m_clockRate = clockRate;
      m_callbackRate = m_clockRate;
      prepareDataAcquisition();
      return true;
    }
  }
  return false;
}

/**
 * Rate cannot be set to zero + call prepareDataAcquisition at the end
 */
void IUI2S::setSamplingRate(uint16_t samplingRate)
{
  if (samplingRate == 0)
  {
    m_samplingRate = defaultSamplingRate;
  }
  else
  {
    m_samplingRate = samplingRate;
  }
  prepareDataAcquisition();
}

/**
 * Configure and Initialize the device
 */
void IUI2S::wakeUp()
{
  if (!m_iuI2C->isSilent())
  {
    m_iuI2C->port->println("Initialized I2S and ICS43432\n");
  }
}

/**
 * Prepare data acquisition by setting up the sampling rates
 * NB: Should be called everytime the callback or sampling rates
 * are modified.
 */
void IUI2S::prepareDataAcquisition()
{
  m_downclocking = m_samplingRate / getCallbackRate();
  m_downclockingCount = 0;
}

bool IUI2S::triggerDataAcquisition(void (*callback)())
{
  // trigger a read to kick things off
  if (m_firstI2STrigger)
  {
    I2S.onReceive(callback); // add the receiver callback
    if (!I2S.begin(I2S_PHILIPS_MODE, m_clockRate, bitsPerAudioSample))
    {
      return false;
    }
    I2S.read();
    m_firstI2STrigger = false;
  }
  else
  {
    readData(false);
  }
  if (loopDebugMode) { debugPrint("Data acquisition triggered\n"); }
  return true;
}

bool IUI2S::endDataAcquisition()
{
  //I2S.end();
  if (loopDebugMode) { debugPrint("Data acquisition disabled"); }
  return true;
}


/* ==================== Data Collection and Feature Calculation functions ======================== */

/**
 * Extract the 16 most significant bits out of ICS43432 32bit sample
 */
void IUI2S::processAudioData(q31_t *data)
{
  for (int i = 0; i < m_downclocking; i++)
  {
    // only keep 1 record every 2 records because stereo recording but we use only 1 canal
    m_audioData[i] = (q15_t) (data[i * 2] >> 16);
  }
}

/**
 * Read audio data
 * @param processData  if true, new data will be processed and update current data batch
 *                     if false, new data are discarded
 *                     default value is true
 * NB: NWe need to call this function to empty I2S buffer, otherwise the process stop. If
 * we don't want to keep the new data, we can pass false to processData argument.
 */
void IUI2S::readData(bool processData)  
{
  int readBitCount = I2S.read(m_rawAudioData, sizeof(m_rawAudioData));
  if (readBitCount)
  {
    processAudioData((q31_t*) m_rawAudioData);
    m_newData = true;
  }
}

bool IUI2S::acquireData()
{
  readData();
  return m_newData;
}

/**
 * Send data to the receivers
 */
void IUI2S::sendToReceivers()
{
  if (!m_newData)
  {
    return;
  }
  for (int i = 0; i < m_receiverCount; i++)
  {
    if (m_receivers[i] && m_toSend[i] == (uint8_t) dataSendOption::sound)
    {
      for (int j = 0; j < m_downclocking; j++)
      {
        m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_audioData[j]);
      }
    }
  }
  m_newData = false;
}



/**
 * Dump audio data to serial via I2C
 * NB: We want to do this in *DATA COLLECTION* mode
 */
void IUI2S::dumpDataThroughI2C()
{
  if (!m_newData)
  {
    return;
  }
  for (int j = 0; j < m_downclocking; j++)
  {
    // stream 16bits value in 2 bytes
    m_iuI2C->port->write((m_audioData[j] >> 8) & 0xFF);
    m_iuI2C->port->write(m_audioData[j] & 0xFF);
  }
  m_newData = false;
}

/**
 * Dump audio data to serial via I2C
 * Note that we do not send all data otherwise it is too much for the Serial to keep up
 * NB: We want to do this in *DATA COLLECTION* mode, when debugMode is true
 */
void IUI2S::dumpDataForDebugging()
{
  if (!m_newData)
  {
    return;
  }
  m_iuI2C->port->print("S: ");
  m_iuI2C->port->println(m_audioData[0]);
  m_iuI2C->port->flush();
  m_newData = false;
}

/* ==================== Update and Control Functions =============================== */
/*
 * The following functions check and read the I2C wire buffer to see if configuration updates
 * have been received. If so, the updates are done.
 * Functions return true if an update happened.
 */

/**
 * Check the I2C wire buffer for update of sampling rate
 */
bool IUI2S::updateSamplingRateFromI2C()
{
  String wireBuffer = m_iuI2C->getBuffer();
  if (wireBuffer.indexOf("acosr") > -1)
  {
    int acosrLocation = wireBuffer.indexOf("acosr");
    int target_sample_A = wireBuffer.charAt(acosrLocation + 6) - 48;
    int target_sample_B = wireBuffer.charAt(acosrLocation + 7) - 48;
    setSamplingRate((target_sample_A * 10 + target_sample_B) * 1000);
    setClockRate((target_sample_A * 10 + target_sample_B) * 1000);
    return true;
  }
  return false;
}
