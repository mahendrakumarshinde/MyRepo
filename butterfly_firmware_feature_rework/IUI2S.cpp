#include "IUI2S.h"

using namespace IUComponent;


/* =============================================================================
    Constructors and destructors
============================================================================= */

uint16_t IUI2S::availableClockRate[IUI2S::availableClockRateCount] =
      {8000, 12000, 16000, 24000, 32000, 48000, 11025, 22050, 44100};

IUI2S::IUI2S(IUI2C *iuI2C, uint8_t id, FeatureBuffer *audio);
  Sensor(id, 1, audio),
  m_iuI2C(iuI2C),
  m_firstI2STrigger(true)
{
  setClockRate(defaultClockRate);
  m_asynchronous = true;
}


/* =============================================================================
    Hardware and power management
============================================================================= */


/* =============================================================================
    Configuration and calibration
============================================================================= */

/**
 * Set the clock rate: Audio sampling is based on this and drives the callback
 *
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
 * Set the audio sampling rate
 *
 * If the new sampling rate is greater than the clock rate, the later will be
 * increased to allow the former.
 * Sampling rate cannot be set to zero.
 * This function internally call prepareDataAcquisition at the end.
 */
void IUI2S::setSamplingRate(uint16_t samplingRate)
{
    if (samplingRate == 0)
    {
        m_samplingRate = defaultSamplingRate;
        return;
    }
    if (samplingRate > m_clockRate) // Need to increase the clock rate ?
    {
        for (uint8_t i = 0; i < availableClockRateCount; ++i)
        {
            if (availableClockRate[i] > samplingRate)
            {
                setClockRate(availableClockRate[i]);
                break;
            }
        }
    }
    m_samplingRate = samplingRate;
    prepareDataAcquisition();
}


/* =============================================================================
    Data acquisition
============================================================================= */

/**
 * Prepare data acquisition by setting up the sampling rates
 *
 * This must be called everytime the callback or sampling rates are modified.
 */
void IUI2S::prepareDataAcquisition()
{
    m_downclocking = m_samplingRate / getCallbackRate();
    m_downclockingCount = 0;
}

/**
 * Start the data acquisition cycle by emptying the I2S buffer
 *
 * The cycle is self-perpetuating since, when I2S buffer is full, it triggers a
 * callback that empty it, and then it will refill.
 * The callback rate can then be determined based on the buffer size and on I2S
 * sampling rate (= filling rate of the buffer).
 */
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
    if (loopDebugMode)
    {
        debugPrint("\nData acquisition triggered\n");
    }
    return true;
}

bool IUI2S::endDataAcquisition()
{
    //I2S.end();
    if (loopDebugMode) { debugPrint("\nData acquisition disabled\n"); }
    return true;
}

/**
 * Process an audio sample
 *
 * @param ICS43432 32bit sample
 */
void IUI2S::processAudioData(q31_t *data)
{
    for (int i = 0; i < m_downclocking; i++)
    {
        /* only keep 1 record every 2 records because stereo recording but we
        use only 1 canal. */
        m_audioData[i] = (q31_t) (data[i * 2]);
        m_destinations[i]->addQ15Value(m_audioData[i]);
    }
}

/**
 * Read audio data
 *
 * This function needs to run to empty I2S buffer, otherwise the process stop.
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
}


/* =============================================================================
    Communication
============================================================================= */

/**
 * Dump audio data to serial via I2C
 * NB: We want to do this in *DATA COLLECTION* mode
 */
void IUI2S::dumpDataThroughI2C()
{
    for (int j = 0; j < m_downclocking; j++)
    {
        // stream 3 most significant bytes from 32bits value
        m_iuI2C->port->write((m_audioData[j] >> 24) & 0xFF);
        m_iuI2C->port->write((m_audioData[j] >> 16) & 0xFF);
        m_iuI2C->port->write((m_audioData[j] >> 8) & 0xFF);
    }
}

/**
 * Dump audio data to serial via I2C
 * Note that we do not send all data otherwise it is too much for the Serial to keep up
 * NB: We want to do this in *DATA COLLECTION* mode, when debugMode is true
 */
void IUI2S::dumpDataForDebugging()
{
    m_iuI2C->port->print("S: ");
    // Dump only most significant 24bits
    m_iuI2C->port->println((m_audioData[0] >> 8));
    m_iuI2C->port->flush();
}


/* ================== Update and Control Functions ========================== */
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


/* =============================================================================
    Debugging
============================================================================= */

