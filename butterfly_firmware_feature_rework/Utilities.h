#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>
#include <arm_math.h>  // CMSIS-DSP library for RFFT

#include "Logger.h"


/*==============================================================================
    Conversion
============================================================================= */

// Q15 <-> Float
float q15ToFloat(q15_t value);

q15_t floatToq15(float value);


/*==============================================================================
    Arrays
============================================================================= */

/**
 * Return the max value of the array (using operator <)
 */
template <typename T>
T getMax(T *values, uint16_t length)
{
    if (length == 0)
    {
        return 0;
    }
    T maxVal = values[0];
    for (uint16_t i = 0; i < length; ++i)
    {
        if (maxVal < values[i])
        {
            maxVal = values[i];
        }
    }
    return maxVal;
}

/**
 * Return the max value of the array (using operator <)
 */
template <typename T>
T sumArray(T *values, uint16_t length)
{
    T total = 0;
    for (uint16_t i = 0; i < length; ++i)
    {
        total += values[i];
    }
    return total;
}

/**
 * Copy source values into destination
 */
template <typename T>
void copyArray(T *source, T *dest, int arrSize)
{
  for (int i = 0; i < arrSize; ++i)
  {
    dest[i] = source[i];
  }
}

/**
 * Return the K max values from given source array
 *
 * Implement a QuickSelect algorithm
 */
void getMaxCoefficients(float *source, uint16_t sourceCount, float *destination,
                        uint16_t destinationCount);


/*==============================================================================
    FFTs
============================================================================= */

/**
 * Collection of function to compute FFTs and use their complex coefficients.
 */
namespace FFT
{
    void computeRFFT(q15_t *source, q15_t *destination,
                     const uint16_t FFTlength, bool inverse);

    void computeRFFT(q15_t *source, q15_t *destination,
                     const uint16_t FFTlength, bool inverse, q15_t *window);

    void filterAndIntegrate(q15_t *values, uint16_t sampleCount,
                            uint16_t samplingRate, uint16_t FreqLowerBound,
                            uint16_t FreqHigherBound, uint16_t scalingFactor,
                            bool twice = false);
}

/**
 * Collection of function that uses FFT Squared amplitudes.
 */
namespace FFTSquaredAmplitudes
{
    void getAmplitudes(q15_t *fftValues, uint16_t sampleCount,
                       q15_t *destination);

    q15_t getRMS(q15_t *amplitudes, uint16_t sampleCount);

    void filterAndIntegrate(q15_t *values, uint16_t sampleCount,
                            uint16_t samplingRate, uint16_t FreqLowerBound,
                            uint16_t FreqHigherBound, uint16_t scalingFactor,
                            bool twice);

    float getMainFrequency(q15_t *amplitudes, uint16_t sampleCount,
                           uint16_t samplingRate);
}


/*==============================================================================
    Analytics
============================================================================= */

q15_t findMaxAscent(q15_t *batch, uint16_t batchSize, uint16_t maxCount);

#endif // UTILITIES_H
