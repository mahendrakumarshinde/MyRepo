#include "IUICS43432.h"


/* =============================================================================
    Constructors and destructors
============================================================================= */

IUICS43432::IUICS43432(I2SClass *I2SInstance, const char* name,
                       Feature *audio) :
  HighFreqSensor(name, 1, audio),
  m_I2S(I2SInstance),
  m_firstI2STrigger(true)
{
    setSamplingRate(defaultSamplingRate);
}


/* =============================================================================
    Configuration and calibration
============================================================================= */


/* =============================================================================
    Data acquisition
============================================================================= */

/**
 * Prepare data acquisition by setting up the sampling rates
 *
 * This must be called everytime the callback or sampling rates are modified.
 */
void IUICS43432::computeDownclockingRate()
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
bool IUICS43432::triggerDataAcquisition(void (*callback)())
{
    // trigger a read to kick things off
    if (m_firstI2STrigger)
    {
        m_I2S->onReceive(callback); // add the receiver callback
        if (!m_I2S->begin(I2S_PHILIPS_MODE, clockRate, bitsPerAudioSample))
        {
            return false;
        }
        m_I2S->read();
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

bool IUICS43432::endDataAcquisition()
{
    //m_I2S->end();
    if (loopDebugMode)
    {
        debugPrint("\nData acquisition disabled\n");
    }
    return true;
}

/**
 * Process an audio sample
 *
 * @param ICS43432 24bit sample, contained in a 32bit number (q31_t). We will
 *  only keep the 16 most significant bit (lose 8bits in precision) and store
 *  them as q15_t.
 */
void IUICS43432::processAudioData(q31_t *data)
{
    for (int i = 0; i < m_downclocking; ++i)
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
void IUICS43432::readData()
{
    int readBitCount = m_I2S->read(m_rawAudioData, sizeof(m_rawAudioData));
    if (readBitCount)
    {
        processAudioData((q31_t*) m_rawAudioData);
    }
}

void IUICS43432::acquireData(bool inCallback, bool force)
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
void IUICS43432::sendData(HardwareSerial *port)
{
    if (loopDebugMode)
    {
        // does nothing
    }
    else
    {
        for (int j = 0; j < m_downclocking; ++j)
        {
            // stream 3 most significant bytes from 32bits value
            port->write((m_rawAudioData[j * 2 + 1]) & 0xFF);
            port->write((m_rawAudioData[j * 2 + 2]) & 0xFF);
            port->write((m_rawAudioData[j * 2 + 3]) & 0xFF);
        }
    }
}


/* =============================================================================
    Debugging
============================================================================= */

/**
 * Shows the I2S + microphone config when in IUDEBUG_ANY
 */
void IUICS43432::expose()
{
    #ifdef IUDEBUG_ANY
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
