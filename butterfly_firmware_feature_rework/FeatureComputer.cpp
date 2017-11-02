#include "FeatureComputer.h"


/* =============================================================================
    Feature Computer Base Class
============================================================================= */

FeatureComputer::FeatureComputer(uint8_t id, uint8_t destinationCount,
                                 Feature *destination0, Feature *destination1,
                                 Feature *destination2, Feature *destination3,
                                 Feature *destination4) :
    m_id(id),
    m_active(false),
    m_sourceCount(0),
    m_destinationCount(destinationCount)
{
    m_destinations[0] = destination0;
    m_destinations[1] = destination1;
    m_destinations[2] = destination2;
    m_destinations[3] = destination3;
    m_destinations[4] = destination4;
    for (uint8_t i = 0; i < m_destinationCount; ++i)
    {
        m_destinations[i]->setComputerId(m_id);
    }
}


/***** Sources and destinations *****/

/**
 * Add a source (a Feature) to the feature
 *
 * Note that a feature can have no more than maxSourceCount sources.
 * @param source        The source to add
 * @param sectionCount  The number of sections of the source computed together
 */
void FeatureComputer::addSource(Feature *source, uint8_t sectionCount)
{
    m_sources[m_sourceCount] = source;
    m_indexesAsReceiver[m_sourceCount] = source->addReceiver(m_id);
    m_sectionCount[m_sourceCount] = sectionCount;
    m_sourceCount++;
}


/***** Computation *****/

/**
 * Compute the feature value and store it
 *
 * @return true if a new value was calculated, else false
 */
bool FeatureComputer::compute()
{
    uint32_t startT = 0;
    if (loopDebugMode && highVerbosity) { startT = millis(); }
    // Check if sources are ready
    for (uint8_t i = 0; i < m_sourceCount; ++i)
    {
        if(!m_sources[i]->isReadyToCompute(m_indexesAsReceiver[i],
                                           m_sectionCount[i]))
        {
            return false;
        }
    }
    // Check if destinations are ready
    for (uint8_t i = 0; i < m_destinationCount; ++i)
    {
        if(!m_destinations[i]->isReadyToRecord())
        {
            return false;
        }
    }
    m_specializedCompute();
    // Acknowledge the computed sections for each source
    for (uint8_t i = 0; i < m_sourceCount; ++i )
    {
        m_sources[i]->acknowledge(m_indexesAsReceiver[i], m_sectionCount[i]);
    }
    if (loopDebugMode && highVerbosity)
    {
        debugPrint(m_id, false);
        debugPrint(F(" computed in "), false);
        debugPrint(millis() - startT, false);
        debugPrint(F("ms"));
    }
    return true;
}


/***** Debugging *****/

/**
 * Print the feature name and configuration (active, computer & receivers)
 */
void FeatureComputer::exposeConfig()
{
    #ifdef DEBUGMODE
    debugPrint(F("Computer #"), false);
    debugPrint(m_id);
    debugPrint(F("  active: "), false);
    debugPrint(m_active);
    debugPrint(F("  "), false);
    debugPrint(m_sourceCount, false);
    debugPrint(F(" sources: "));
    for (uint8_t i = 0; i < m_sourceCount; ++i)
    {
        debugPrint(F("    "), false);
        debugPrint(m_sources[i]->getName(), false);
        debugPrint(F(", index as receiver "), false);
        debugPrint(m_indexesAsReceiver[i], false);
        debugPrint(F(", section count "), false);
        debugPrint(m_sectionCount[i]);
    }
    debugPrint(F("  "), false);
    debugPrint(m_destinationCount, false);
    debugPrint(F(" destinations: "), false);
    for (uint8_t i = 0; i < m_destinationCount; ++i)
    {
        debugPrint(m_destinations[i]->getName(), false);
        debugPrint(F(", "), false);
    }
    debugPrint("");
    #endif
}


/* =============================================================================
    Feature Computers
============================================================================= */

/***** Signal Energy, Power and RMS *****/

SignalRMSComputer::SignalRMSComputer(uint8_t id, Feature *rms, bool removeMean,
                                     bool normalize) :
    FeatureComputer(id, 1, rms),
    m_removeMean(removeMean),
    m_normalize(normalize)
{
    // Constructor
}

/**
 * Compute the total energy, power and RMS of a signal
 *
 * NB: Since the signal energy is squared, the resulting scaling factor will be
 * squared as well.
 * - Signal energy: sum((x(t)- x_mean)^2 * dt) for t in [0, dt, 2 *dt,
 * ..., T] where x_mean = Mean(x) if removeMean is true else 0,
 * dt = 1 / samplingFreq and T = sampleCount / samplingFreq.
 * Signal power: signal energy divided by Period
 * Signal RMS: square root of signal power
 */
void SignalRMSComputer::m_specializedCompute()
{
    float resolution = m_sources[0]->getResolution();
    q15_t *values = m_sources[0]->getNextQ15Values(m_indexesAsReceiver[0]);
    uint16_t sectionSize = m_sources[0]->getSectionSize();
    uint16_t totalSize = m_sectionCount[0] * sectionSize;
    float total = 0;
    q15_t avg = 0;
    if (m_removeMean)
    {
        arm_mean_q15(values, sectionSize, &avg);
    }
    for (int i = 0; i < totalSize; ++i)
    {
        total += sq((values[i] - avg) * resolution);
    }
    if (m_normalize)
    {
        total = total / (float) totalSize;
    }
    m_destinations[0]->setSamplingRate(m_sources[0]->getSamplingRate());
    m_destinations[0]->setResolution(resolution);
    m_destinations[0]->addFloatValue(sqrt(total));
}


/***** Sums *****/

SectionSumComputer::SectionSumComputer(uint8_t id, uint8_t destinationCount,
                                       Feature *destination0,
                                       Feature *destination1,
                                       Feature *destination2,
                                       bool normalize) :
    FeatureComputer(id, destinationCount, destination0, destination1,
                    destination2),
    m_normalize(normalize)
{
    // Constructor
}

/**
 * Compute and return the sum of several section, source by source
 */
void SectionSumComputer::m_specializedCompute()
{
    uint16_t length = 0;
    float *values;
    float total;
    for (uint8_t i = 0; i < m_sourceCount; ++i)
    {
        m_destinations[i]->setSamplingRate(m_sources[i]->getSamplingRate());
        m_destinations[i]->setResolution(m_sources[i]->getResolution());
        length = m_sources[i]->getSectionSize() * m_sectionCount[i];
        values = m_sources[i]->getNextFloatValues(m_indexesAsReceiver[i]);
        total = 0;
        for (uint16_t j = 0; j < length; ++j)
        {
            total += values[j];
        }
        if (m_normalize)
        {
            total = total / (float) length;
        }
        m_destinations[i]->addFloatValue(total);
    }
}


MultiSourceSumComputer::MultiSourceSumComputer(uint8_t id,
                                               Feature *destination0,
                                               bool normalize) :
    FeatureComputer(id, 1, destination0),
    m_normalize(normalize)
{
    // Constructor
}

/**
 * Compute and return the sum of several section, source by source
 *
 * It is expected that the sources are compatible, ie they have the same
 * sampling rate, resolution, number of sections and the same section size.
 */
void MultiSourceSumComputer::m_specializedCompute()
{
    m_destinations[0]->setSamplingRate(m_sources[0]->getSamplingRate());
    m_destinations[0]->setResolution(m_sources[0]->getResolution());
    uint16_t length = m_sources[0]->getSectionSize() * m_sectionCount[0];
    float total;
    for (uint16_t k = 0; k < length; ++k)
    {
        total = 0;
        for (uint8_t i = 0; i < m_sourceCount; ++i)
        {
            total += m_sources[i]->getNextFloatValues(
                m_indexesAsReceiver[i])[k];
        }
        if (m_normalize)
        {
            total = total / (float) m_sourceCount;
        }
        m_destinations[0]->addFloatValue(total);
    }
}


/***** FFTs *****/

Q15FFTComputer::Q15FFTComputer(uint8_t id,
                               Feature *reducedFFT,
                               Feature *integralRMS,
                               Feature *doubleIntegralRMS,
                               q15_t *allocatedFFTSpace,
                               uint16_t lowCutFrequency,
                               uint16_t highCutFrequency) :
    FeatureComputer(id, 3, reducedFFT, integralRMS, doubleIntegralRMS),
    m_lowCutFrequency(lowCutFrequency),
    m_highCutFrequency(highCutFrequency)
{
    m_allocatedFFTSpace = allocatedFFTSpace;
}

/**
 * RFFT computation + RMS of intregrated & double-integrated signal
 *
 * Internally creates 2 Q15 arrays of size sampleCount / 2.
 * => Total size in bytes is 2 * sampleCount.
 */
void Q15FFTComputer::m_specializedCompute()
{
    // 0. Preparation
    uint16_t samplingRate = m_sources[0]->getSamplingRate();
    for (uint8_t i = 0; i < m_destinationCount; ++i)
    {
        m_destinations[i]->setSamplingRate(samplingRate);
    }
    float resolution = m_sources[0]->getResolution();
    uint16_t sampleCount = m_sources[0]->getSectionSize() * m_sectionCount[0];
    uint16_t reducedLength = m_destinations[0]->getSectionSize() / 3;
    q15_t *values = m_sources[0]->getNextQ15Values(m_indexesAsReceiver[0]);
    uint32_t amplitudeCount = sampleCount / 2 + 1;
    q15_t amplitudes[amplitudeCount];
    // 1. Compute FFT and get amplitudes
    RFFT::computeRFFT(values, m_allocatedFFTSpace, sampleCount, false);
    RFFTAmplitudes::getAmplitudes(m_allocatedFFTSpace, sampleCount, amplitudes);
    // 2. Keep the K max coefficients (K = reducedLength)
    q15_t maxVal;
    uint32_t maxIdx;
    q15_t amplitudesCopy[amplitudeCount];
    copyArray(amplitudes, amplitudesCopy, amplitudeCount);
    for (uint16_t i = 0; i < reducedLength; ++i)
    {
        arm_max_q15(amplitudesCopy, amplitudeCount, &maxVal, &maxIdx);
        amplitudesCopy[maxIdx] = 0;
        m_destinations[0]->addQ15Value((q15_t) maxIdx);
        m_destinations[0]->addQ15Value(m_allocatedFFTSpace[2 * maxIdx]);
        m_destinations[0]->addQ15Value(m_allocatedFFTSpace[2 * maxIdx + 1]);
    }
    // 3. 1st integration in frequency domain
    q15_t scaling1 = RFFTAmplitudes::getRescalingFactorForIntegral(
        amplitudes, sampleCount, samplingRate);
    RFFTAmplitudes::filterAndIntegrate(amplitudes, sampleCount, samplingRate,
                                       m_lowCutFrequency, m_highCutFrequency,
                                       scaling1, false);
    float integratedRMS1 = RFFTAmplitudes::getRMS(amplitudes, sampleCount);
    integratedRMS1 *= 1000 / ((float) scaling1);
    m_destinations[1]->addFloatValue(integratedRMS1);
    m_destinations[1]->setResolution(resolution / 1000.0);
    // 4. 2nd integration in frequency domain
    q15_t scaling2 = RFFTAmplitudes::getRescalingFactorForIntegral(
        amplitudes, sampleCount, samplingRate);
    RFFTAmplitudes::filterAndIntegrate(amplitudes, sampleCount, samplingRate,
                                       m_lowCutFrequency, m_highCutFrequency,
                                       scaling2, false);
    float integratedRMS2 = RFFTAmplitudes::getRMS(amplitudes, sampleCount);
    integratedRMS2 *= 1000 / ((float) scaling1 * (float) scaling2);
    m_destinations[2]->addFloatValue(integratedRMS2);
    m_destinations[2]->setResolution(resolution / 1000.0);
}


/***** Audio DB *****/

AudioDBComputer::AudioDBComputer(uint8_t id, Feature *audioDB) :
    FeatureComputer(id, 1, audioDB)
{
}

/**
 * Return the average sound level in DB over an array of sound pressure data
 *
 * Acoustic decibel formula: 20 * log10(p / p_ref) where
 * p is the sound pressure
 * p_ref is the reference pressure for sound in air = 20 mPa (or 1 mPa in water)
 */
void AudioDBComputer::m_specializedCompute()
{
    m_destinations[0]->setSamplingRate(m_sources[0]->getSamplingRate());
    m_destinations[0]->setResolution(m_sources[0]->getResolution());
    uint16_t length = m_sources[0]->getSectionSize() * m_sectionCount[0];
    q15_t* values = m_sources[0]->getNextQ15Values(m_indexesAsReceiver[0]);
    /* Oddly, log10 is very slow. It's better to use it the least possible,
    so we multiply data points as much as possible and then log10 the results
    when we have to. We can multiply data points until we reach uint64_t limit
    (so 2^64).*/
    //First, compute the limit for our accumulation
    q15_t maxVal = 0;
    uint32_t maxIdx = 0;
    arm_max_q15(values, length, &maxVal, &maxIdx); // Get max data value
    uint64_t limit = pow(2, 63 - round(log(maxVal) / log(2)));
    // Then, use it to compute our data
    int data = 0;
    uint64_t accu = 1;
    float audioDB = 0.;
    for (uint16_t j = 0; j < length; ++j)
    {
        data = abs(values[j]);
        if (data > 0)
        {
            accu *= data;
        }
        if (accu >= limit)
        {
            audioDB += log10(accu);
            accu = 1;
        }
    }
    audioDB += log10(accu);
    m_destinations[0]->addFloatValue(20.0 * audioDB / (float) length);
}
