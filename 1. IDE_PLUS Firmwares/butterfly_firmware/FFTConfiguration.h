/*
    This file includes:
    - All supported configurations for FFT parameters
    - The current FFT configuration

    The file tracks all the available configurations for FFT computation: 
    - samplingRate as dictated by the sensor sampling frequency
    - blockSize as dictated by available memory    

    NOTE: Update the new samplingRates and blockSizes in the following variables: 
    samplingRateConfigurations, blockSizeConfigurations,
    samplingRates[samplingRateConfigurations], blockSizes[blockSizeConfigurations]
    These values are being used for validation in IUFlash::validateConfig() { case:CFG_FFT }
 */

#ifndef FFT_CONFIGURATION_H
#define FFT_CONFIGURATION_H

#define FMAX_FACTOR 2.56

namespace FFTConfiguration {
    // The number of possible configurations for samplingRate and blockSize
    // const int samplingRateConfigurations = 4;
    //const int samplingRateConfigurations = 6;
    const int LSMsamplingRateOption = 4 ;
    const int KNXsamplingRateOption = 6 ;
    const int blockSizeConfigurations = 6;
    // Arrays which keep track of available configurations
    const int samplingRates[LSMsamplingRateOption] = { 416, 833, 1660, 3330 };
    const int samplingRates2[KNXsamplingRateOption] = { 800, 1600, 3200, 6400, 12800, 25600};
    const int blockSizes[blockSizeConfigurations] = { 256, 512, 1024, 2048, 4096, 8192 };

    // Default parameter values
    const int DEFAULT_SAMPLING_RATE = 25600;
    const int DEFALUT_LOW_CUT_OFF_FREQUENCY_KNX = 10;
    const int DEFALUT_LOW_CUT_OFF_FREQUENCY_LSM = 5;
    const int DEFAULT_BLOCK_SIZE = 8192;
    const int DEFALUT_LOW_CUT_OFF_FREQUENCY = 5;
    const int DEFAULT_HIGH_CUT_OFF_FREQUENCY = DEFALUT_LOW_CUT_OFF_FREQUENCY_KNX / FMAX_FACTOR;  
    const float DEFAULT_MIN_AGITATION = 0.03;
    
    // Current configurations
    extern int currentSamplingRate;
    extern int currentBlockSize;
    extern int currentLowCutOffFrequency;
    extern int currentHighCutOffFrequency;
    extern float currentMinAgitation;
    extern bool currentSensor;   //if currenSensor=0 LSM is selected else kionix is selected
    const bool lsmSensor = 0; 
    const bool kionixSensor = 1;
}

#endif // FFT_CONFIGURATION_H
