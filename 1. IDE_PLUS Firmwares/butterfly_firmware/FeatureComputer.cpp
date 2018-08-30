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
    m_destinationCount(destinationCount),
    m_computeLast(false)
{
    m_destinations[0] = destination0;
    m_destinations[1] = destination1;
    m_destinations[2] = destination2;
    m_destinations[3] = destination3;
    m_destinations[4] = destination4;
    for (uint8_t i = 0; i < m_destinationCount; ++i) {
        m_destinations[i]->setComputer(this);
    }
    // Instance registration
    m_instanceIdx = instanceCount;
    instances[m_instanceIdx] = this;
    instanceCount++;
}

FeatureComputer::~FeatureComputer()
{
    // Remove pointer to this in sources and destinations
    deleteAllSources();
    for (uint8_t i = 0; i < m_destinationCount; ++i) {
        m_destinations[i]->removeComputer();
    }
    // Remove from class registry
    instances[m_instanceIdx] = NULL;
    for (uint8_t i = m_instanceIdx + 1; i < instanceCount; ++i) {
        instances[i]->m_instanceIdx--;
        instances[i -1] = instances[i];
    }
    instances[instanceCount] = NULL;
    instanceCount--;
}

FeatureComputer *FeatureComputer::getInstanceById(const uint8_t id)
{
    for (uint8_t i = 0; i < instanceCount; ++i) {
        if (instances[i] != NULL) {
            if(instances[i]->getId() == id) {
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
 * @return True if the source was added, else false (eg: if the source buffer
 *  is full).
 */
bool FeatureComputer::addSource(Feature *source, uint8_t sectionCount)
{
    if (m_sourceCount >= maxSourceCount) {
        if (debugMode) {
            debugPrint(F("Computer can't add new source (source array overflow)"));
        }
        return false;
    }
    m_sources[m_sourceCount] = source;
    m_sectionCount[m_sourceCount] = sectionCount;
    m_sourceCount++;
    source->addReceiver(this);
    return true;
}

void FeatureComputer::deleteAllSources()
{
    uint8_t srcCount = m_sourceCount;
    m_sourceCount = 0;
    for (uint8_t i = 0; i < srcCount; i++)
    {
        m_sources[i]->removeReceiver(this);
        m_sources[i] = NULL;
    }
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
    // Check if sources are ready (and check if there was an error in the
    // source for the data sections that this computer uses).
    bool sourceError = false;
    for (uint8_t i = 0; i < m_sourceCount; ++i) {
        if(!m_sources[i]->isReadyToCompute(this, m_sectionCount[i],
                                           m_computeLast))
        {
            return false;
        }
        sourceError |= m_sources[i]->sectionsToComputeHaveDataError(
            this, m_sectionCount[i]);
    }
    // Check if destinations are ready
    for (uint8_t i = 0; i < m_destinationCount; ++i) {
        if(!m_destinations[i]->isReadyToRecord()) {
            return false;
        }
    }
    if (m_active)
    {
        // If there was an error in the source,  transmit it to the destinations
        // BEFORE sending them new values (see Feature.flagDataError).
        if (sourceError) {
            for (uint8_t i = 0; i < m_destinationCount; ++i) {
                m_destinations[i]->flagDataError();
            }
        }
        // Compute and send new values to the destinations.
        m_specializedCompute();
    }
    // Acknowledge the computed sections for each source
    for (uint8_t i = 0; i < m_sourceCount; ++i ) {
        m_sources[i]->acknowledge(this, m_sectionCount[i]);
    }
    return true;
}


/***** Debugging *****/

/**
 * Print the feature name and configuration (active, computer & receivers)
 */
void FeatureComputer::exposeConfig()
{
    #ifdef IUDEBUG_ANY
    debugPrint(F("Computer #"), false);
    debugPrint(m_id);
    debugPrint(F("  active: "), false);
    debugPrint(m_active);
    debugPrint(F("  "), false);
    debugPrint(m_sourceCount, false);
    debugPrint(F(" source(s): "));
    for (uint8_t i = 0; i < m_sourceCount; ++i)
    {
        debugPrint(F("    "), false);
        debugPrint(m_sources[i]->getName(), false);
        debugPrint(F(", index as receiver "), false);
        debugPrint(m_sources[i]->getReceiverIndex(this), false);
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
    Feature State Computer
============================================================================= */

bool FeatureStateComputer::addSource(Feature *source, uint8_t sectionCount,
                                     bool active)
{
    if (FeatureComputer::addSource(source, sectionCount)) {
        m_activeOpStateFeatures[m_sourceCount - 1] = active;
        source->reset();  // WHY?
        return true;
    } else {
        return false;
    }
}

/**
 *
 */
bool FeatureStateComputer::addOpStateFeature(
    Feature *feature, float lowThreshold, float medThreshold,
    float highThreshold, uint8_t sectionCount, bool active)
{
    if (addSource(feature, sectionCount, active)) {
        m_activeOpStateFeatures[m_sourceCount - 1] = active;
        setThresholds(m_sourceCount - 1, lowThreshold, medThreshold,
                      highThreshold);
        if (debugMode) {
            debugPrint(F("Added OP State source: "), false);
            debugPrint(feature->getName(), false);
            debugPrint(F(", active="), false);
            debugPrint(active, false);
            debugPrint(F(", th=("), false);
            debugPrint(lowThreshold, false);
            debugPrint(F(", "), false);
            debugPrint(medThreshold, false);
            debugPrint(F(", "), false);
            debugPrint(highThreshold, false);
            debugPrint(F(")"));
        }
        return true;
    } else {
        return false;
    }
}

void FeatureStateComputer::setThresholds(uint8_t idx, float low, float med,
                                         float high)
{
    if (idx >= m_sourceCount) {
        if (debugMode) { debugPrint(F("Index out of range")); }
        return;
    }
    m_lowThresholds[idx] = low;
    m_medThresholds[idx] = med;
    m_highThresholds[idx] = high;
    if (loopDebugMode) {
        debugPrint(m_sources[idx]->getName(), false);
        debugPrint(F(": "), false);
        debugPrint(m_lowThresholds[idx], false);
        debugPrint(F(" - "), false);
        debugPrint(m_medThresholds[idx], false);
        debugPrint(F(" - "), false);
        debugPrint(m_highThresholds[idx]);
    }
}



/**
 * Read the given JSON to select the features to use for the Operation State.
 */
void FeatureStateComputer::configure(JsonVariant &config)
{
    deleteAllSources();
    JsonVariant myConfig;
    Feature *feature;
    float th0, th1, th2;
    for (uint8_t i = 0; i < Feature::instanceCount; ++i) {
        feature = Feature::instances[i];
        myConfig = config[feature->getName()];
        if (myConfig.success()) {
            JsonVariant active = config["OPS"];
            th0 = config["TRH"][0];
            th1 = config["TRH"][1];
            th2 = config["TRH"][2];
            addOpStateFeature(feature, th0, th1, th2, 1,
                              !(active.success() && active.as<int>() == 0));
        }
    }
}

/**
 *
 */
void FeatureStateComputer::m_specializedCompute()
{
    q15_t newState = 0;
    q15_t featureState;
    float avg;
    if (featureDebugMode) {
        debugPrint(F("OP state from "), false);
        debugPrint(m_sourceCount, false);
        debugPrint(F(" source(s): "), false);
        for (uint8_t i = 0; i < m_sourceCount; i++) {
            if (i > 0) {
                debugPrint(F(", "), false);
            }
            debugPrint(m_sources[i]->getName(), false);
            if (m_activeOpStateFeatures[i]) {
                debugPrint(F(" (active)"), false);
            } else {
                debugPrint(F(" (inactive)"), false);
            }
        }
        debugPrint(F(""));
    }
    for (uint8_t i = 0; i < m_sourceCount; i++) {
        if (!m_activeOpStateFeatures[i]) {
            continue;  // Skip features not used for OP State
        }
        // Feature is active: we can compute the OP state
        avg = m_getSectionAverage(m_sources[i]);
        if (avg > m_highThresholds[i]) {
            featureState = 3;
        } else if (avg > m_medThresholds[i]) {
            featureState = 2;
        } else if (avg > m_lowThresholds[i]) {
            featureState = 1;
        } else {
            featureState = 0;
        }
        if (newState < featureState) {
            newState = featureState;
        }
    }
    if (featureDebugMode) {
        debugPrint(millis(), false);
        debugPrint(" -> ", false);
        debugPrint(m_destinations[0]->getName(), false);
        debugPrint(": ", false);
        debugPrint(newState);
    }
    m_destinations[0]->addValue(newState);
}


/**
 * Returns the average of the last recorded section values * resolution.
 */
float FeatureStateComputer::m_getSectionAverage(Feature *feature)
{
    float *floatValues = feature->getNextFloatValuesToCompute(this);
    q15_t *q15Values = feature->getNextQ15ValuesToCompute(this);
    q31_t *q31Values = feature->getNextQ31ValuesToCompute(this);
    float total = 0;
    uint16_t sectionSize = feature->getSectionSize();
    float resolution = feature->getResolution();
    if (floatValues != NULL) {
        for (uint16_t i = 0; i < sectionSize; ++i) {
            total += floatValues[i];
        }
    } else if (q15Values != NULL) {
        for (uint16_t i = 0; i < sectionSize; ++i) {
            total += float(q15Values[i]);
        }
    } else if (q31Values != NULL) {
        for (uint16_t i = 0; i < sectionSize; ++i) {
            total += float(q31Values[i]);
        }
    }
    return total * resolution / float(sectionSize);
}


/* =============================================================================
    Averaging
============================================================================= */

AverageComputer::AverageComputer(uint8_t id, FeatureTemplate<float> *input) :
    FeatureComputer(id, 1, input)
{
    // Constructor
}

void AverageComputer::m_specializedCompute()
{
    float resolution = m_sources[0]->getResolution();
    float *values = m_sources[0]->getNextFloatValuesToCompute(this);
    uint16_t totalSize = m_sectionCount[0] * m_sources[0]->getSectionSize();
    float avg = 0;
    arm_mean_f32(values, totalSize, &avg);
    m_destinations[0]->setResolution(resolution);
    m_destinations[0]->addValue(avg);
    if (featureDebugMode) {
        debugPrint(millis(), false);
        debugPrint(" -> ", false);
        debugPrint(m_destinations[0]->getName(), false);
        debugPrint(": ", false);
        debugPrint(avg * resolution);
    }
}


/* =============================================================================
    Signal Energy, Power and RMS
============================================================================= */


SignalRMSComputer::SignalRMSComputer(uint8_t id, FeatureTemplate<float> *rms,
                                     bool removeMean, bool normalize,
                                     bool squared, float calibrationScaling) :
    FeatureComputer(id, 1, rms),
    m_removeMean(removeMean),
    m_normalize(normalize),
    m_squared(squared),
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
    if (my_config.success()) {
        setRemoveMean(my_config.as<int>() > 0);
    }
    my_config = config["NORM"];
    if (my_config.success()) {
        setNormalize(my_config.as<int>() > 0);
    }
    my_config = config["SQR"];
    if (my_config.success()) {
        setSquaredOutput(my_config.as<int>() > 0);
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
    q15_t *values = m_sources[0]->getNextQ15ValuesToCompute(this);
    uint16_t sectionSize = m_sources[0]->getSectionSize();
    uint16_t totalSize = m_sectionCount[0] * sectionSize;
    float total = 0;
    q15_t avg = 0;
    if (m_removeMean) {
        arm_mean_q15(values, totalSize, &avg);
    }
    for (int i = 0; i < totalSize; ++i) {
        total += sq((values[i] - avg));
    }
    if (m_normalize) {
        total = total / (float) totalSize;
    }
    if (!m_squared) {
        total = sqrt(total);
    }
    m_destinations[0]->setSamplingRate(m_sources[0]->getSamplingRate());
    if (m_squared) {
        m_destinations[0]->setResolution(sq(resolution));
    } else {
        m_destinations[0]->setResolution(resolution);
    }
    m_destinations[0]->addValue(total * m_calibrationScaling);
    if (featureDebugMode)
    {
        debugPrint(millis(), false);
        debugPrint(" -> ", false);
        debugPrint(m_destinations[0]->getName(), false);
        debugPrint(": ", false);
        if (m_squared) {
            debugPrint(total * sq(resolution));
        } else {
            debugPrint(total * resolution);
        }
    }
}


/* =============================================================================
    Sum of several sections of the same source
============================================================================= */

SectionSumComputer::SectionSumComputer(uint8_t id, uint8_t destinationCount,
                                       FeatureTemplate<float> *destination0,
                                       FeatureTemplate<float> *destination1,
                                       FeatureTemplate<float> *destination2,
                                       bool normalize, bool rmsInput) :
    FeatureComputer(id, destinationCount, destination0, destination1,
                    destination2),
    m_normalize(normalize),
    m_rmsInput(rmsInput)
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
    if (my_config.success()) {
        setNormalize(my_config.as<int>() > 0);
    }
    my_config = config["RMS"];
    if (my_config.success()) {
        setRMSInput(my_config.as<int>() > 0);
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
    for (uint8_t i = 0; i < m_sourceCount; ++i) {
        m_destinations[i]->setSamplingRate(m_sources[i]->getSamplingRate());
        m_destinations[i]->setResolution(m_sources[i]->getResolution());
        length = m_sources[i]->getSectionSize() * m_sectionCount[i];
        values = m_sources[i]->getNextFloatValuesToCompute(this);
        total = 0;
        if (m_rmsInput) {
            for (uint16_t j = 0; j < length; ++j) {
                total += sq(values[j]);
            }
        } else {
            for (uint16_t j = 0; j < length; ++j) {
                total += values[j];
            }
        }
        if (m_normalize || m_rmsInput) {
            total = total / (float) length;
        }
        if (m_rmsInput) {
            total = sqrt(total);
        }
        m_destinations[i]->addValue(total);
        if (featureDebugMode) {
            debugPrint(millis(), false);
            debugPrint(" -> ", false);
            debugPrint(m_destinations[i]->getName(), false);
            debugPrint(": ", false);
            debugPrint(total * m_sources[i]->getResolution());
        }
    }
}


/* =============================================================================
    Sum of the same section on different source
============================================================================= */

MultiSourceSumComputer::MultiSourceSumComputer(
        uint8_t id, FeatureTemplate<float> *destination0, bool normalize,
        bool rmsInput) :
    FeatureComputer(id, 1, destination0),
    m_normalize(normalize),
    m_rmsInput(rmsInput)
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
    if (my_config.success()) {
        setNormalize(my_config.as<int>() > 0);
    }
    my_config = config["RMS"];
    if (my_config.success()) {
        setRMSInput(my_config.as<int>() > 0);
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
    float *valuePointers[m_sourceCount];
    for (uint8_t i = 0; i < m_sourceCount; ++i) {
        valuePointers[i] = m_sources[i]->getNextFloatValuesToCompute(this);
    }
    for (uint16_t k = 0; k < length; ++k) {
        total = 0;
        if (m_rmsInput) {
            for (uint8_t i = 0; i < m_sourceCount; ++i) {
                total += sq(valuePointers[i][k]);
            }
        } else {
            for (uint8_t i = 0; i < m_sourceCount; ++i) {
                total += valuePointers[i][k];
            }
        }
        if (m_normalize || m_rmsInput) {
            total = total / (float) m_sourceCount;
        }
        if (m_rmsInput) {
            total = sqrt(total);
        }
        m_destinations[0]->addValue(total);
        if (featureDebugMode) {
            debugPrint(millis(), false);
            debugPrint(" -> ", false);
            debugPrint(m_destinations[0]->getName(), false);
            debugPrint(": ", false);
            debugPrint(total * m_sources[0]->getResolution());
        }
    }
}


/* =============================================================================
    FFT computations
============================================================================= */

Q15FFTComputer::Q15FFTComputer(uint8_t id,
                               FeatureTemplate<q15_t> *reducedFFT,
                               FeatureTemplate<float> *mainFrequency,
                               FeatureTemplate<float> *integralRMS,
                               FeatureTemplate<float> *doubleIntegralRMS,
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
    m_computeLast = true;
}

/**
 * Configure the computer parameters from given json
 */
void Q15FFTComputer::configure(JsonVariant &config)
{
    FeatureComputer::configure(config);
    JsonVariant freq = config["FREQ"][0];
    if (freq.success()) {
        setLowCutFrequency((uint16_t) (freq.as<int>()));
    }
    freq = config["FREQ"][1];
    if (freq.success()) {
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
    q15_t *values = m_sources[0]->getNextQ15ValuesToCompute(this);
    for (uint8_t i = 0; i < m_destinationCount; ++i) {
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
    if (!isInMotion && featureDebugMode) {
        debugPrint(F("Device is still - Freq, vel & disp defaulted to 0."));
    }
    // 2. Keep the K max coefficients (K = reducedLength)
    uint16_t reducedLength = m_destinations[0]->getSectionSize() / 3;
    q15_t maxVal;
    uint32_t maxIdx;
    // If board is still, freq is defaulted to 0.
    bool mainFreqSaved = false;
    float freq(0);
    if (!isInMotion) {
        m_destinations[1]->addValue(freq);
        mainFreqSaved = true;
    }
    q15_t amplitudesCopy[amplitudeCount];
    copyArray(amplitudes, amplitudesCopy, amplitudeCount);
    for (uint16_t i = 0; i < reducedLength; ++i) {
        arm_max_q15(amplitudesCopy, amplitudeCount, &maxVal, &maxIdx);
        amplitudesCopy[maxIdx] = 0;
        m_destinations[0]->addValue((q15_t) maxIdx);
        m_destinations[0]->addValue(m_allocatedFFTSpace[2 * maxIdx]);
        m_destinations[0]->addValue(m_allocatedFFTSpace[2 * maxIdx + 1]);
        if (!mainFreqSaved) {
            freq = df * (float) maxIdx;
            if (freq > m_lowCutFrequency && freq < m_highCutFrequency) {
                m_destinations[1]->addValue(freq);
                mainFreqSaved = true;
            }
        }
    }
    if (featureDebugMode) {
        debugPrint(millis(), false);
        debugPrint(F(" -> "), false);
        debugPrint(m_destinations[0]->getName(), false);
        debugPrint(F(": computed"));
        debugPrint(millis(), false);
        debugPrint(F(" -> "), false);
        debugPrint(m_destinations[1]->getName(), false);
        debugPrint(F(": "), false);
        debugPrint(freq);
    }
    if (isInMotion) {
        // 3. 1st integration in frequency domain
        q15_t scaling1 = RFFTAmplitudes::getRescalingFactorForIntegral(
            amplitudes, sampleCount, samplingRate);
        RFFTAmplitudes::filterAndIntegrate(
            amplitudes, sampleCount, samplingRate, m_lowCutFrequency,
            m_highCutFrequency, scaling1, false);
        float integratedRMS1 = RFFTAmplitudes::getRMS(amplitudes, sampleCount);
        integratedRMS1 *= 1000 / ((float) scaling1) * m_calibrationScaling1;
        m_destinations[2]->addValue(integratedRMS1);
        if (featureDebugMode) {
            debugPrint(millis(), false);
            debugPrint(F(" -> "), false);
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
        m_destinations[3]->addValue(integratedRMS2);
        if (featureDebugMode) {
            debugPrint(millis(), false);
            debugPrint(F(" -> "), false);
            debugPrint(m_destinations[3]->getName(), false);
            debugPrint(": ", false);
            debugPrint(integratedRMS2 * resolution);
        }
    } else {
        m_destinations[2]->addValue(0.0);
        m_destinations[3]->addValue(0.0);
        if (featureDebugMode) {
            debugPrint(millis(), false);
            debugPrint(F(" -> "), false);
            debugPrint(m_destinations[2]->getName(), false);
            debugPrint(F(": 0"));
            debugPrint(millis(), false);
            debugPrint(F(" -> "), false);
            debugPrint(m_destinations[3]->getName(), false);
            debugPrint(F(": 0"));
        }
    }
}


/* =============================================================================
    Audio DB
============================================================================= */

AudioDBComputer::AudioDBComputer(uint8_t id, FeatureTemplate<float> *audioDB,
                                 float calibrationScaling,
                                 float calibrationOffset) :
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
    q15_t* values = m_sources[0]->getNextQ15ValuesToCompute(this);
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
    for (uint16_t j = 0; j < length; ++j) {
        data = abs(values[j]);
        if (data > 0) {
            accu *= data;
        }
        if (accu >= limit) {
            audioDB += log10(accu);
            accu = 1;
        }
    }
    audioDB += log10(accu);
    float result = 20.0 * audioDB / (float) length;
    //result = 2.8 * result - 10;  // Empirical formula
    result = 1.3076 * result + 21.41;  // Empirical formula
    result = result * m_calibrationScaling + m_calibrationOffset;
    m_destinations[0]->addValue(result);
    if (featureDebugMode) {
        debugPrint(millis(), false);
        debugPrint(" -> ", false);
        debugPrint(m_destinations[0]->getName(), false);
        debugPrint(": ", false);
        debugPrint(result * m_sources[0]->getResolution());
    }
}
