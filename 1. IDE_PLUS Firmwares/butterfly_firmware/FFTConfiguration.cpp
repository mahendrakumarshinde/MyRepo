#include "FFTConfiguration.h"

namespace FFTConfiguration {
    int currentSamplingRate = DEFAULT_SAMPLING_RATE;
    int currentBlockSize = DEFAULT_BLOCK_SIZE;
    int currentLowCutOffFrequency = DEFALUT_LOW_CUT_OFF_FREQUENCY;
    int currentHighCutOffFrequency = DEFAULT_HIGH_CUT_OFF_FREQUENCY;
    float currentMinAgitation = DEFAULT_MIN_AGITATION;
    bool currentSensor = lsmSensor;
    // RPM Parameters
    int lowRPMFrequency = DEFALUT_LOW_CUT_OFF_FREQUENCY;
    int highRPMFrequency = DEFAULT_HIGH_CUT_OFF_FREQUENCY;
    float defaultRPMThreshold = DEFAULT_RPM_THRESHOLD; 
    
}