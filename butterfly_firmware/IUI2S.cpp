#include "IUI2S.h"

char IUI2S::sensorTypes[IUI2S::sensorTypeCount] = {IUABCSensor::sensorType_microphone};

IUI2S::IUI2S(IUI2C *iuI2C) :
  IUABCSensor(),
  m_iuI2C(iuI2C),
  m_newData(false)
{
  setClockRate(defaultClockRate);
}

/**
 * Audio sensor is based on the clock rate rather than on the main callback rate
 */
void IUI2S::setClockRate(uint32_t clockRate)
{
  m_clockRate = clockRate;
  m_callbackRate = m_clockRate;
  prepareDataAcquisition();
}

/**
 * Rate cannot be set to zero + call prepareDataAcquisition at the end
 */
void IUI2S::setSamplingRate(uint32_t samplingRate)
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
 * Prepare data acquisition by setting up the sampling rates
 * NB: Should be called everytime the callback or sampling rates
 * are modified.
 */
void IUI2S::prepareDataAcquisition()
{
  m_downclocking = m_samplingRate / getCallbackRate();
  m_downclockingCount = 0;
}

/**
 * Configure and Initialize the device
 */
void IUI2S::wakeUp()
{
  if (!m_iuI2C->isSilent())
  {
    m_iuI2C->port->println( "Initialized I2S and ICS43432" );
  }
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
    m_audioData[i * 2] = (q15_t) (data[i] >> 16);
  }
  m_newData = true;
}

/**
 * read audio data
 * NB: Need to call this function to empty I2S buffer, otherwise the process stop
 */
void IUI2S::readData()
{  
  int readBitCount = I2S.read(m_rawAudioData, sizeof(m_rawAudioData));
  if (readBitCount)
  {
    processAudioData((q31_t*) m_rawAudioData);
  }
}

bool IUI2S::acquireData()
{
  readData();
  if (m_newData)
  {
    m_newData = false;
    return true;
  }
  return false;
}

/**
 * Send data to the receivers
 */
void IUI2S::sendToReceivers()
{
  for (int i = 0; i < m_receiverCount; i++)
  {
    if (m_receivers[i] && m_toSend[i] == dataSendOption::sound)
    {
      int sampleCount = audioSampleSize / m_downclocking;
      for (int j = 0; j < sampleCount; j++)
      {
        m_receivers[i]->receive(m_receiverSourceIndex[i], m_audioData[j]);
      }
    }
  }
}

/**
 * Dump audio data to serial via I2C
 * NB: We want to do this in *DATA COLLECTION* mode
 */
void IUI2S::dumpDataThroughI2C()
{
  int sampleCount = audioSampleSize / m_downclocking;
  for (int j = 0; j < sampleCount; j++)
  {
    // stream 16bits value in 2 bytes
    m_iuI2C->port->write((m_audioData[j] >> 8) & 0xFF);
    m_iuI2C->port->write(m_audioData[j] & 0xFF);
  }
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
    m_targetSample = (target_sample_A * 10 + target_sample_B) * 1000;
    return true;
  }
  return false;
}
