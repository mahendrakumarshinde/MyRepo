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

namespace FFTConfiguration {
    // The number of possible configurations for samplingRate and blockSize
    const int samplingRateConfigurations = 4;
    const int blockSizeConfigurations = 5;

    // Arrays which keep track of available configurations
    const int samplingRates[samplingRateConfigurations] = { 416, 833, 1660, 3330 };
    const int blockSizes[blockSizeConfigurations] = { 256, 512, 1024, 2048, 4096 };

    // Default parameter values
    const int DEFAULT_SAMPLING_RATE = 3330;
    const int DEFAULT_BLOCK_SIZE = 512;
    const int DEFALUT_LOW_CUT_OFF_FREQUENCY = 10;
    const int DEFAULT_HIGH_CUT_OFF_FREQUENCY = DEFAULT_SAMPLING_RATE / 2.56;  
    const float DEFAULT_MIN_AGITATION = 0.03;
    
    // Current configurations
    extern int currentSamplingRate;
    extern int currentBlockSize;
    extern int currentLowCutOffFrequency;
    extern int currentHighCutOffFrequency;
    extern float currentMinAgitation;
}

#endif // FFT_CONFIGURATION_H
