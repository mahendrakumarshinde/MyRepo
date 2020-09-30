#ifndef FEATUREUTILITIES_H
#define FEATUREUTILITIES_H

#include <Arduino.h>
#include <arm_math.h>  // CMSIS-DSP library for RFFT

#include <IUDebugger.h>


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
inline void getMax(T *values, uint32_t length, T *maxVal, uint32_t *maxIdx)
{
    if (length == 0) {
        return;
    }
    maxVal = values[0];
    maxIdx = 0;
    for (uint32_t i = 0; i < length; ++i) {
        if (maxVal < values[i]) {
            maxVal = values[i];
            maxIdx = i;
        }
    }
}

template <>
inline void getMax(q15_t *values, uint32_t length, q15_t *maxVal, uint32_t *maxIdx)
{
    arm_max_q15(values, length, maxVal, maxIdx);
}

template <>
inline void getMax(q31_t *values, uint32_t length, q31_t *maxVal, uint32_t *maxIdx)
{
    arm_max_q31(values, length, maxVal, maxIdx);
}

template <>
inline void getMax(float *values, uint32_t length, float *maxVal, uint32_t *maxIdx)
{
    arm_max_f32(values, length, maxVal, maxIdx);
}

/**
 * Return the mean value of the array (using operator <)
 */
template <typename T>
inline void getMean(T *values, uint32_t length, T *result)
{
    // Not defined
}

template <>
inline void getMean(q15_t *values, uint32_t length, q15_t *result)
{
    arm_mean_q15(values, length, result);
}

template <>
inline void getMean(q31_t *values, uint32_t length, q31_t *result)
{
    arm_mean_q31(values, length, result);
}

template <>
inline void getMean(float *values, uint32_t length, float *result)
{
    arm_mean_f32(values, length, result);
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


/*==============================================================================
    FFTs
============================================================================= */

/**
 * Collection of function to compute RFFTs and use their complex coefficients.
 *
 * To compute RFFT, we use arm_rfft functions: these functions output an array
 * of 2 * FFT length (number of sample points in temporal domain), where:
 *  - even index are real part of FFT coef
 *  - odd index are complex part of FFT coef
 * Because of that:
 *  - coefs[0] + j*coefs[1] is the DC component, with coefs[1] = 0 since RFFT
 *  - coefs[2*k] + j*coefs[2*k + 1] for k in [1, 255] are the positive frequency
 *  coefficients.
 *  - coefs[512] + j*coefs[513] is the Nyquist freq coefficient (same for both
 *  positive and negative frequencies)
 *  - coefs[2*k] + j*coefs[2*k + 1] for k in [257, 511] are the negative
 *  frequency coefficients. Since it's an RFFT, the negative freq coefs are the
 *  complex conjugate of the positive freq coefs.
 *
 * NB: In fact, it looks like arm_rfft function DOESN'T USE these coefficients
 * to compute the inverse RFFT (although it still needs an array with enough
 * room to contain them, and that the forward RFFT provide these coefs).
 */
namespace RFFT
{
    void computeRFFT(q15_t *source, q15_t *destination,
                     const uint16_t FFTlength, bool inverse,
                     q15_t *window=NULL);

    void computeRFFT(q31_t *source, q31_t *destination,
                     const uint32_t FFTlength, bool inverse);

    uint16_t getRescalingFactorForIntegral(q15_t *values, uint16_t sampleCount,
                                           uint16_t samplingRate);

    uint32_t getRescalingFactorForIntegral(q31_t *values, uint16_t sampleCount,
                                           uint16_t samplingRate);

    void filterAndIntegrate(q15_t *values, uint16_t sampleCount,
                            uint16_t samplingRate, uint16_t FreqLowerBound,
                            uint16_t FreqHigherBound, uint16_t scalingFactor,
                            bool twice = false);

    void filterAndIntegrate(q31_t *values, uint16_t sampleCount,
                            uint16_t samplingRate, uint16_t FreqLowerBound,
                            uint16_t FreqHigherBound, uint32_t scalingFactor,
                            bool twice = false);
}

/**
 * Collection of function that uses RFFT amplitudes.
 *
 * amplitude = sqrt(real^2 + complex^2) (ie absolute value of complex coef)
 * IMPORTANT: Since dealing with RFFT, negative frequency coefficients are
 * complex conjugate of the positive frequency coefficients, so their amplitudes
 * are equal. As a consequence, length of amplitude array is
 * SampleCount / 2 + 1 (since sampleCount is always even)
 * where SampleCount is the number of data point in time domain.
 */
namespace RFFTAmplitudes
{
    void getAmplitudes(q15_t *rfftValues, uint16_t sampleCount,
                       q15_t *destination);

    void getAmplitudes(q31_t *rfftValues, uint16_t sampleCount,
                       q31_t *destination);

    float getRMS(q15_t *amplitudes, uint16_t sampleCount, bool removeDC=true);

    float getRMS(q31_t *amplitudes, uint16_t sampleCount, bool removeDC=true);

    // function to get accl. mean
    float getAccelerationMean(q15_t *values,uint16_t sampleCount);

    // removeAccelAmplitudesMean 
    void removeNewAccelMean(q15_t *values, uint16_t sampleCount,float newAccelMean,q15_t *newAccelAmplitudes);
    //get the new RMS
    float getNewAccelRMS(q15_t *newAccelAmplitudes,uint16_t sampleCount);
    
    uint16_t getRescalingFactorForIntegral(q15_t *amplitudes,
                                           uint16_t sampleCount,
                                           uint16_t samplingRate);

    uint32_t getRescalingFactorForIntegral(q31_t *amplitudes,
                                           uint16_t sampleCount,
                                           uint16_t samplingRate);

    void filterAndIntegrate(q15_t *amplitudes, uint16_t sampleCount,
                            uint16_t samplingRate, uint16_t FreqLowerBound,
                            uint16_t FreqHigherBound, uint16_t scalingFactor,
                            bool twice);

    void filterAndIntegrate(q31_t *amplitudes, uint16_t sampleCount,
                            uint16_t samplingRate, uint16_t FreqLowerBound,
                            uint16_t FreqHigherBound, uint16_t scalingFactor,
                            bool twice);

    float getMainFrequency(q15_t *amplitudes, uint16_t sampleCount,
                           uint16_t samplingRate);

    float getMainFrequency(q31_t *amplitudes, uint16_t sampleCount,
                           uint16_t samplingRate);
}

namespace RFFTFeatures {

    float computeRPM(q15_t *amplitudes, int m_lowRPMFrequency,int m_highRPMFrequency,float rpm_threshold,
    float df,float resolution,float scalingFactor);
}

/*==============================================================================
    Analytics
============================================================================= */

q15_t findMaxAscent(q15_t *batch, uint16_t batchSize, uint16_t maxCount);

#endif // FEATUREUTILITIES_H
