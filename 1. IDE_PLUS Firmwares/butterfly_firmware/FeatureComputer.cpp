#include "FeatureComputer.h"


/* =============================================================================
    Feature Computer Base Class
============================================================================= */

uint8_t FeatureComputer::instanceCount = 0;

FeatureComputer *FeatureComputer::instances[
    FeatureComputer::MAX_INSTANCE_COUNT] = {
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

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
    // Instance registration
    m_instanceIdx = instanceCount;
    instances[m_instanceIdx] = this;
    instanceCount++;
}

FeatureComputer::~FeatureComputer()
{
    instances[m_instanceIdx] = NULL;
    for (uint8_t i = m_instanceIdx + 1; i < instanceCount; ++i)
    {
        instances[i]->m_instanceIdx--;
        instances[i -1] = instances[i];
    }
    instances[instanceCount] = NULL;
    instanceCount--;
}

FeatureComputer *FeatureComputer::getInstanceById(const uint8_t id)
{
    for (uint8_t i = 0; i < instanceCount; ++i)
    {
        if (instances[i] != NULL)
        {
            if(instances[i]->getId() == id)
            {
                return instances[i];
            }
        }
    }
    return NULL;
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
    if (m_active)
    {
        m_specializedCompute();
    }
    // Acknowledge the computed sections for each source
    for (uint8_t i = 0; i < m_sourceCount; ++i )
    {
        m_sources[i]->acknowledge(m_indexesAsReceiver[i], m_sectionCount[i]);
    }
    if (m_active && loopDebugMode && highVerbosity)
    {
        debugPrint(m_id, false);
        debugPrint(F(" computed in "), false);
        debugPrint(millis() - startT, false);
        debugPrint(F("ms"));
    }
    return true;
}

/**
 * Acknowledge the sections for each source
 *
 * Function should be called either after computation
 */
void FeatureComputer::acknowledgeSectionToSources()
{
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
                                     bool normalize, float calibrationScaling) :
    FeatureComputer(id, 1, rms),
    m_removeMean(removeMean),
    m_normalize(normalize),
    m_calibrationScaling(calibrationScaling)
{
    // Constructor
}

/**
 * Configure the computer parameters from given json
 */
void SignalRMSComputer::configure(JsonVariant &config)
{
    FeatureComputer::configure(config);
    JsonVariant my_config = config["NODC"];
    if (my_config.success())
    {
        setRemoveMean(my_config.as<int>() > 0);
    }
    my_config = config["NORM"];
    if (my_config.success())
    {
        setNormalize(my_config.as<int>() > 0);
    }
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
        total += sq((values[i] - avg));
    }
    if (m_normalize)
    {
        total = total / (float) totalSize;
    }
    total = sqrt(total);
    m_destinations[0]->setSamplingRate(m_sources[0]->getSamplingRate());
    m_destinations[0]->setResolution(resolution);
    m_destinations[0]->addFloatValue(total * m_calibrationScaling);
    if (featureDebugMode)
    {
        debugPrint(m_destinations[0]->getName(), false);
        debugPrint(": ", false);
        debugPrint(total * resolution);
    }
}


/***** Sums *****/

SectionSumComputer::SectionSumComputer(uint8_t id, uint8_t destinationCount,
                                       Feature *destination0,
                                       Feature *destination1,
                                       Feature *destination2,
                                       bool normalize, bool rmsLike) :
    FeatureComputer(id, destinationCount, destination0, destination1,
                    destination2),
    m_normalize(normalize),
    m_rmsLike(rmsLike)
{
    // Constructor
}

/**
 * Configure the computer parameters from given json
 */
void SectionSumComputer::configure(JsonVariant &config)
{
    FeatureComputer::configure(config);
    JsonVariant my_config = config["NORM"];
    if (my_config.success())
    {
        setNormalize(my_config.as<int>() > 0);
    }
    my_config = config["RMS"];
    if (my_config.success())
    {
        setRMSLike(my_config.as<int>() > 0);
    }
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
        if (m_rmsLike)
        {
            for (uint16_t j = 0; j < length; ++j)
            {
                total += sq(values[j]);
            }
        }
        else
        {
            for (uint16_t j = 0; j < length; ++j)
            {
                total += values[j];
            }
        }
        if (m_normalize || m_rmsLike)
        {
            total = total / (float) length;
        }
        if (m_rmsLike)
        {
            total = sqrt(total);
        }
        m_destinations[i]->addFloatValue(total);
        if (featureDebugMode)
        {
            debugPrint(m_destinations[i]->getName(), false);
            debugPrint(": ", false);
            debugPrint(total * m_sources[i]->getResolution());
        }
    }
}


MultiSourceSumComputer::MultiSourceSumComputer(uint8_t id,
                                               Feature *destination0,
                                               bool normalize, bool rmsLike) :
    FeatureComputer(id, 1, destination0),
    m_normalize(normalize),
    m_rmsLike(rmsLike)
{
    // Constructor
}

/**
 * Configure the computer parameters from given json
 */
void MultiSourceSumComputer::configure(JsonVariant &config)
{
    FeatureComputer::configure(config);
    JsonVariant my_config = config["NORM"];
    if (my_config.success())
    {
        setNormalize(my_config.as<int>() > 0);
    }
    my_config = config["RMS"];
    if (my_config.success())
    {
        setRMSLike(my_config.as<int>() > 0);
    }
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
        if (m_rmsLike)
        {
            for (uint8_t i = 0; i < m_sourceCount; ++i)
            {
                total += sq(m_sources[i]->getNextFloatValues(
                    m_indexesAsReceiver[i])[k]);
            }
        }
        else
        {
            for (uint8_t i = 0; i < m_sourceCount; ++i)
            {
                total += m_sources[i]->getNextFloatValues(
                    m_indexesAsReceiver[i])[k];
            }
        }
        if (m_normalize || m_rmsLike)
        {
            total = total / (float) m_sourceCount;
        }
        if (m_rmsLike)
        {
            total = sqrt(total);
        }
        m_destinations[0]->addFloatValue(total);
        if (featureDebugMode)
        {
            debugPrint(m_destinations[0]->getName(), false);
            debugPrint(": ", false);
            debugPrint(total * m_sources[0]->getResolution());
        }
    }
}


/***** FFTs *****/

Q15FFTComputer::Q15FFTComputer(uint8_t id,
                               Feature *reducedFFT,
                               Feature *mainFrequency,
                               Feature *integralRMS,
                               Feature *doubleIntegralRMS,
                               q15_t *allocatedFFTSpace,
                               uint16_t lowCutFrequency,
                               uint16_t highCutFrequency,
                               float minAgitationRMS,
                               float calibrationScaling1,
                               float calibrationScaling2) :
    FeatureComputer(id, 4, reducedFFT, mainFrequency, integralRMS,
                    doubleIntegralRMS),
    m_lowCutFrequency(lowCutFrequency),
    m_highCutFrequency(highCutFrequency),
    m_minAgitationRMS(minAgitationRMS),
    m_calibrationScaling1(calibrationScaling1),
    m_calibrationScaling2(calibrationScaling2)
{
    m_allocatedFFTSpace = allocatedFFTSpace;
}

/**
 * Configure the computer parameters from given json
 */
void Q15FFTComputer::configure(JsonVariant &config)
{
    FeatureComputer::configure(config);
    JsonVariant freq = config["FREQ"][0];
    if (freq.success())
    {
        setLowCutFrequency((uint16_t) (freq.as<int>()));
    }
    freq = config["FREQ"][1];
    if (freq.success())
    {
        setHighCutFrequency((uint16_t) (freq.as<int>()));
    }
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
    float resolution = m_sources[0]->getResolution();
    uint16_t sampleCount = m_sources[0]->getSectionSize() * m_sectionCount[0];
    float df = (float) samplingRate / (float) sampleCount;
    q15_t *values = m_sources[0]->getNextQ15Values(m_indexesAsReceiver[0]);
    for (uint8_t i = 0; i < m_destinationCount; ++i)
    {
        m_destinations[i]->setSamplingRate(samplingRate);
    }
    m_destinations[0]->setResolution(resolution);
    m_destinations[1]->setResolution(1);
    m_destinations[2]->setResolution(resolution);
    m_destinations[3]->setResolution(resolution);
    // 1. Compute FFT and get amplitudes
    uint32_t amplitudeCount = sampleCount / 2 + 1;
    q15_t amplitudes[amplitudeCount];
    RFFT::computeRFFT(values, m_allocatedFFTSpace, sampleCount, false);
    RFFTAmplitudes::getAmplitudes(m_allocatedFFTSpace, sampleCount, amplitudes);
    float agitation = RFFTAmplitudes::getRMS(amplitudes, sampleCount, true);
    bool isInMotion = (agitation * resolution) > m_minAgitationRMS ;
    if (!isInMotion && featureDebugMode)
    {
        debugPrint(F("Device is still - Freq, vel & disp defaulted to 0."));
    }
    // 2. Keep the K max coefficients (K = reducedLength)
    uint16_t reducedLength = m_destinations[0]->getSectionSize() / 3;
    q15_t maxVal;
    uint32_t maxIdx;
    // If board is still, freq is defaulted to 0.
    bool mainFreqSaved = false;
    float freq(0);
    if (!isInMotion)
    {
        m_destinations[1]->addFloatValue(freq);
        mainFreqSaved = true;
    }
    q15_t amplitudesCopy[amplitudeCount];
    copyArray(amplitudes, amplitudesCopy, amplitudeCount);
    for (uint16_t i = 0; i < reducedLength; ++i)
    {
        arm_max_q15(amplitudesCopy, amplitudeCount, &maxVal, &maxIdx);
        amplitudesCopy[maxIdx] = 0;
        m_destinations[0]->addQ15Value((q15_t) maxIdx);
        m_destinations[0]->addQ15Value(m_allocatedFFTSpace[2 * maxIdx]);
        m_destinations[0]->addQ15Value(m_allocatedFFTSpace[2 * maxIdx + 1]);
        if (!mainFreqSaved)
        {
            freq = df * (float) maxIdx;
            if (freq > m_lowCutFrequency && freq < m_highCutFrequency)
            {
                m_destinations[1]->addFloatValue(freq);
                mainFreqSaved = true;
            }
        }
    }
    if (featureDebugMode)
    {
        debugPrint(m_destinations[0]->getName(), false);
        debugPrint(": was computed");
        debugPrint(m_destinations[1]->getName(), false);
        debugPrint(": ", false);
        debugPrint(freq);
    }
    if (isInMotion)
    {
        // 3. 1st integration in frequency domain
        q15_t scaling1 = RFFTAmplitudes::getRescalingFactorForIntegral(
            amplitudes, sampleCount, samplingRate);
        RFFTAmplitudes::filterAndIntegrate(
            amplitudes, sampleCount, samplingRate, m_lowCutFrequency,
            m_highCutFrequency, scaling1, false);
        float integratedRMS1 = RFFTAmplitudes::getRMS(amplitudes, sampleCount);
        integratedRMS1 *= 1000 / ((float) scaling1) * m_calibrationScaling1;
        m_destinations[2]->addFloatValue(integratedRMS1);
        if (featureDebugMode)
        {
            debugPrint(m_destinations[2]->getName(), false);
            debugPrint(": ", false);
            debugPrint(integratedRMS1 * resolution);
        }
        // 4. 2nd integration in frequency domain
        q15_t scaling2 = RFFTAmplitudes::getRescalingFactorForIntegral(
            amplitudes, sampleCount, samplingRate);
        RFFTAmplitudes::filterAndIntegrate(
            amplitudes, sampleCount, samplingRate, m_lowCutFrequency,
            m_highCutFrequency, scaling2, false);
        float integratedRMS2 = RFFTAmplitudes::getRMS(amplitudes, sampleCount);
        integratedRMS2 *= 1000 / ((float) scaling1 * (float) scaling2) *
            m_calibrationScaling2;
        m_destinations[3]->addFloatValue(integratedRMS2);
        if (featureDebugMode)
        {
            debugPrint(m_destinations[3]->getName(), false);
            debugPrint(": ", false);
            debugPrint(integratedRMS2 * resolution);
        }
    }
    else
    {
        m_destinations[2]->addFloatValue(0);
        m_destinations[3]->addFloatValue(0);
        if (featureDebugMode)
        {
            debugPrint(m_destinations[2]->getName(), false);
            debugPrint(": 0");
            debugPrint(m_destinations[3]->getName(), false);
            debugPrint(": 0");
        }
    }
}


/***** Audio DB *****/

AudioDBComputer::AudioDBComputer(uint8_t id, Feature *audioDB,
                                 float calibrationScaling) :
    FeatureComputer(id, 1, audioDB),
    m_calibrationScaling(calibrationScaling)
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
    float result = 20.0 * audioDB / (float) length;
    result *= m_calibrationScaling;
    m_destinations[0]->addFloatValue(result);
    if (featureDebugMode)
    {
        debugPrint(m_destinations[0]->getName(), false);
        debugPrint(": ", false);
        debugPrint(result * m_sources[0]->getResolution());
    }
}


/* =============================================================================
    Instanciations
============================================================================= */


// Shared computation space
q15_t allocatedFFTSpace[1024];


// Note that computer_id 0 is reserved to designate an absence of computer.

/***** Accelerometer Features *****/

// 128 sample long accel computers
SignalRMSComputer accel128ComputerX(1,&accelRMS128X, true, true, ACCEL_RMS_SCALING);
SignalRMSComputer accel128ComputerY(2, &accelRMS128Y, true, true, ACCEL_RMS_SCALING);
SignalRMSComputer accel128ComputerZ(3, &accelRMS128Z, true, true, ACCEL_RMS_SCALING);
MultiSourceSumComputer accelRMS128TotalComputer(4, &accelRMS128Total,
                                                false, true);


// 512 sample long accel computers
SectionSumComputer accel512ComputerX(5, 1, &accelRMS512X, NULL, NULL,
                                     false, true);
SectionSumComputer accel512ComputerY(6, 1, &accelRMS512Y, NULL, NULL,
                                     false, true);
SectionSumComputer accel512ComputerZ(7, 1, &accelRMS512Z, NULL, NULL,
                                     false, true);
SectionSumComputer accel512TotalComputer(8, 1, &accelRMS512Total, NULL, NULL,
                                         false, true);


// Computers for FFT feature from 512 sample long accel data
Q15FFTComputer accelFFTComputerX(9,
                                 &accelReducedFFTX,
                                 &accelMainFreqX,
                                 &velRMS512X,
                                 &dispRMS512X,
                                 allocatedFFTSpace,
                                 DEFAULT_LOW_CUT_FREQUENCY,
                                 DEFAULT_HIGH_CUT_FREQUENCY,
                                 DEFAULT_MIN_AGITATION,
                                 VELOCITY_RMS_SCALING,
                                 DISPLACEMENT_RMS_SCALING);
Q15FFTComputer accelFFTComputerY(10,
                                 &accelReducedFFTY,
                                 &accelMainFreqY,
                                 &velRMS512Y,
                                 &dispRMS512Y,
                                 allocatedFFTSpace,
                                 DEFAULT_LOW_CUT_FREQUENCY,
                                 DEFAULT_HIGH_CUT_FREQUENCY,
                                 DEFAULT_MIN_AGITATION,
                                 VELOCITY_RMS_SCALING,
                                 DISPLACEMENT_RMS_SCALING);
Q15FFTComputer accelFFTComputerZ(11,
                                 &accelReducedFFTZ,
                                 &accelMainFreqZ,
                                 &velRMS512Z,
                                 &dispRMS512Z,
                                 allocatedFFTSpace,
                                 DEFAULT_LOW_CUT_FREQUENCY,
                                 DEFAULT_HIGH_CUT_FREQUENCY,
                                 DEFAULT_MIN_AGITATION,
                                 VELOCITY_RMS_SCALING,
                                 DISPLACEMENT_RMS_SCALING);


/***** Audio Features *****/

AudioDBComputer audioDB2048Computer(12, &audioDB2048, AUDIO_DB_SCALING);
AudioDBComputer audioDB4096Computer(13, &audioDB4096, AUDIO_DB_SCALING);


/***** Set up sources *****/

/**
 * Add sources to computer instances (must be called during main setup)
 */
void setUpComputerSources()
{
    // From acceleration sensor data
    accel128ComputerX.addSource(&accelerationX, 1);
    accel128ComputerY.addSource(&accelerationY, 1);
    accel128ComputerZ.addSource(&accelerationZ, 1);
    accelRMS128TotalComputer.addSource(&accelRMS128X, 1);
    accelRMS128TotalComputer.addSource(&accelRMS128Y, 1);
    accelRMS128TotalComputer.addSource(&accelRMS128Z, 1);
    // Aggregate acceleration RMS
    accel512ComputerX.addSource(&accelRMS128X, 1);
    accel512ComputerY.addSource(&accelRMS128Y, 1);
    accel512ComputerZ.addSource(&accelRMS128Z, 1);
    accel512TotalComputer.addSource(&accelRMS128Total, 1);
    // Acceleration FFTs
    accelFFTComputerX.addSource(&accelerationX, 4);
    accelFFTComputerY.addSource(&accelerationY, 4);
    accelFFTComputerZ.addSource(&accelerationZ, 4);
    // Audio DB
    audioDB2048Computer.addSource(&audio, 1);
    audioDB4096Computer.addSource(&audio, 2);
}
