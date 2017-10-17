#include "IUI2S.h"


/* =============================================================================
    Constructors and destructors
============================================================================= */

IUI2S::IUI2S(IUI2C *iuI2C, const char* name, Feature *audio) :
  AsynchronousSensor(name, 1, audio),
  m_iuI2C(iuI2C),
  m_firstI2STrigger(true)
{
    setSamplingRate(defaultSamplingRate);
}


/* =============================================================================
    Configuration and calibration
============================================================================= */

/**
 * Set the audio sampling rate
 *
 * If the new sampling rate is greater than the clock rate, the later will be
 * increased to allow the former.
 * Sampling rate cannot be set to zero.
 * This function internally call computeDownclockingRate at the end.
 */
void IUI2S::setSamplingRate(uint16_t samplingRate)
{
    m_samplingRate = samplingRate;
    computeDownclockingRate();
}


/* =============================================================================
    Data acquisition
============================================================================= */

/**
 * Prepare data acquisition by setting up the sampling rates
 *
 * This must be called everytime the callback or sampling rates are modified.
 */
void IUI2S::computeDownclockingRate()
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
        if (!I2S.begin(I2S_PHILIPS_MODE, clockRate, bitsPerAudioSample))
        {
            return false;
        }
        I2S.read();
        m_firstI2STrigger = false;
    }
    else
    {
        readData();
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
 * @param ICS43432 24bit sample, contained in a 32bit number (q31_t). We will
 *  only keep the 16 most significant bit (lose 8bits in precision) and store
 *  them as q15_t.
 */
void IUI2S::processAudioData(q31_t *data)
{
    for (int i = 0; i < m_downclocking; i++)
    {
        /* only keep 1 record every 2 records because stereo recording but we
        use only 1 canal. */
        // Send most significant 16bits
        m_audioData[i] = (q15_t) (data[i * 2] >> 16);
        m_destinations[0]->addQ15Value(m_audioData[i]);
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

void IUI2S::acquireData()
{
    readData();
}


/* =============================================================================
    Communication
============================================================================= */

/**
 * Dump audio data to serial via I2C
 *
 * This should be done in DATA COLLECTION mode.
 */
void IUI2S::sendData(HardwareSerial *port)
{
    if (loopDebugMode)
    {
        // does nothing
    }
    else
    {
        for (int j = 0; j < m_downclocking; j++)
        {
            // stream 3 most significant bytes from 32bits value
            port->write((m_audioData[j] >> 24) & 0xFF);
            port->write((m_audioData[j] >> 16) & 0xFF);
            port->write((m_audioData[j] >> 8) & 0xFF);
        }
    }
}


/* =============================================================================
    Debugging
============================================================================= */

/**
 * Shows the I2S + microphone config when in DEBUGMODE
 */
void IUI2S::expose()
{
    #ifdef DEBUGMODE
    Sensor::expose();
    debugPrint(F("Sampling config"));
    debugPrint(F("  clock rate: "), false);
    debugPrint(clockRate);
    debugPrint(F("  audio sample size: "), false);
    debugPrint(audioSampleSize);
    debugPrint(F("  callback rate: "), false);
    debugPrint(getCallbackRate());
    debugPrint(F("  sampling rate: "), false);
    debugPrint(m_samplingRate);
    debugPrint(F("  downclocking: "), false);
    debugPrint(m_downclocking);
    #endif
}

