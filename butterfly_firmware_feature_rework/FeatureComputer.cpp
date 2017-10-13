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
 * @param source              The source to add
 * @param sectionCount        The number of sections of the source computed
 *      together
 * @param skippedSectionCount The number of sections to skip between 2 groups of
 *      computed sections
 * Eg: For a source of 8 sections, with sectionCount=1 & skippedSectionCount=3,
 *      the sections used for computation will be the 1st and 5th (1 sections,
 *      then skip 3, then 1, then skip 3, etc).
 */
void FeatureComputer::addSource(Feature *source, uint8_t sectionCount,
                                uint8_t skippedSectionCount)
{
    if (m_sourceCount >= maxSourceCount)
    {
        raiseException("Too many sources");
    }
    m_sources[m_sourceCount] = source;
    m_indexesAsReceiver[m_sourceCount] = source->addReceiver(m_id);
    m_sectionCount[m_sourceCount] = sectionCount;
    m_skippedSectionCount[m_sourceCount] = skippedSectionCount;
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
    if (loopDebugMode) { startT = millis(); }
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
    if (loopDebugMode)
    {
        debugPrint(m_id, false);
        debugPrint(F(" computed in time "), false);
        debugPrint(millis() - startT);
    }
    // Acknowledge the computed sections for each source
    for (uint8_t i = 0; i < m_sourceCount; ++i )
    {
        m_sources[i]->partialAcknowledge(m_indexesAsReceiver[i],
                                         m_sectionCount[i]);
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
        debugPrint(m_sectionCount[i], false);
        debugPrint(F(", skipped section count "), false);
        debugPrint(m_skippedSectionCount[i]);
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

SignalEnergyComputer::SignalEnergyComputer(uint8_t id, Feature *destEnergy,
                                           Feature *destPower, Feature *destRMS,
                                           bool removeMean, bool normalize) :
    FeatureComputer(id, 3, destEnergy, destPower, destRMS),
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
void SignalEnergyComputer::m_specializedCompute()
{
    uint16_t samplingRate = m_sources[0]->getSamplingRate();
    uint16_t resolution = m_sources[0]->getResolution();
    for (uint8_t i = 0; i < 3; ++i)
    {
        m_destinations[i]->setSamplingRate(samplingRate);
        m_destinations[i]->setResolution(resolution);
    }
    q15_t *values = m_sources[0]->getNextQ15Values();
    uint16_t sectionSize = m_sources[0]->getSectionSize();
    uint16_t totalSize = m_sectionCount[0] * sectionSize;
    float total = 0;
    q15_t avg = 0;
    if (m_removeMean)
    {
        arm_mean_q15(values, sectionSize, &avg);
    }
    for (int i = 0; i < totalSize; i++)
    {
        total += sq((float) (values[i] - avg) * resolution);
    }
    if (m_normalize)
    {
        total = total / (float) totalSize;
    }
    m_destinations[0]->addFloatValue(total / (float) samplingRate);
    m_destinations[1]->addFloatValue(total);
    m_destinations[2]->addFloatValue(sqrt(total));
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
        values = m_sources[i]->getNextFloatValues();
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
    float total = 0.;
    for (uint16_t k = 0; k < length; ++k)
    {
        for (uint8_t i = 0; i < m_sourceCount; ++i)
        {
            total += m_sources[i]->getNextFloatValues()[k];
        }
        if (m_normalize)
        {
            total = total / (float) m_sourceCount;
        }
        m_destinations[0]->addFloatValue(total);
    }
}


/***** FFTs *****/

Q15FFTComputer::Q15FFTComputer(uint8_t id, Feature *reducedFFT,
                               Feature *integralRMS,
                               Feature *integralPeakToPeak,
                               Feature *doubleIntegralRMS,
                               Feature *doubleIntegralPeakToPeak,
                               q15_t *allocatedFFTSpace,
                               uint8_t outputFFTIntegration,
                               bool computeReducedFFT,
                               bool computeIntegral,
                               bool computeDoubleIntegral) :
    FeatureComputer(id, 5, reducedFFT, integralRMS, integralPeakToPeak,
                    doubleIntegralRMS, doubleIntegralPeakToPeak),
    m_outputFFTIntegration(outputFFTIntegration),
    m_computeReducedFFT(computeReducedFFT),
    m_computeIntegral(computeIntegral),
    m_computeDoubleIntegral(computeDoubleIntegral)
{
    m_allocatedFFTSpace = allocatedFFTSpace;
}

void Q15FFTComputer::m_specializedCompute()
{
    /*
    uint16_t FFTLength = m_sources[0]->getSectionSize() * m_sectionCount[0];
    arm_status armStatus = arm_rfft_init_q15(
        &m_rfftInstance, // RFFT instance
        FFTLength,       // FFT length
        0,               // 0: forward FFT, 1: inverse FFT
        1);              // 1: enable bit reversal output
    if (armStatus != ARM_MATH_SUCCESS)
    {
        if (loopDebugMode)
        {
            debugPrint("FFT computation failed");
        }
        return;
    }
    q15_t *source = m_sources[0]->getNextQ15Values();
    arm_rfft_q15(&m_rfftInstance, source, m_allocatedFFTSpace);
    */
}

/*
void FeatureComputerFFTQ31::m_specializedCompute(uint8_t specIndex)
{
  uint16_t FFTLength = m_sources[0]->getSectionLength() * m_sourceBufferSpecs[0][specIndex][1];
  arm_status armStatus = arm_rfft_init_q31(&m_rfftInstance, // RFFT instance
                                           FFTLength,       // FFT length
                                           0,               // 0: forward FFT, 1: inverse FFT
                                           1);              // 1: enable bit reversal output
  if (armStatus != ARM_MATH_SUCCESS)
  {
    if (loopDebugMode) { debugPrint("FFT / Inverse FFT computation failed"); }
    return;
  }
  q15_t *source = m_sources[0]->getBuffer(m_sourceBufferIndex[0])->getQ31ValuesForCompute();
  q15_t *destination = m_buffers[0]->getQ31ValuesForCompute();
  arm_rfft_q31(&m_rfftInstance, source, destination);
}
*/


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

    // TODO Reimplement
//    uint16_t length = m_sources[0]->getSectionSize() * m_sectionCount[0];
//    q15_t* values = m_sources[0]->getNextQ15Values();
//    // Strangely, log10 is very slow. It's better to use it the least possible, so we
//    // multiply data points as much as possible and then log10 the results when we have to.
//    // We can multiply data points until we reach uint64_t limit (so 2^64)
//    // First, compute the limit for our accumulation
//    q15_t maxVal = 0;
//    uint32_t maxIdx = 0;
//    arm_max_q15(values, length, &maxVal, &maxIdx); // Get max data value
//    uint64_t limit = pow(2, 63 - round(log(maxVal) / log(2)));
//    // Then, use it to compute our data
//    int data = 0;
//    uint64_t accu = 1;
//    float audioDB = 0.;
//    for (uint16_t j = 0; j < length; j++)
//    {
//        data = abs(values[j]);
//        if (data > 0)
//        {
//            accu *= data;
//        }
//        if (accu >= limit)
//        {
//            audioDB += log10(accu);
//            accu = 1;
//        }
//    }
//    audioDB += log10(accu);
//    m_destinations[0]->addFloatValue(20.0 * audioDB / (float) length);
}
