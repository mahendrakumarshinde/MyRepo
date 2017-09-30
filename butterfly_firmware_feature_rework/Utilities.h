#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>

#include "Logger.h"
#include "Keywords.h"


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


/*==============================================================================
    String processing
============================================================================= */

uint8_t splitString(String str, const char separator, String *destination,
                    uint8_t destinationSize);

uint8_t splitStringToInt(String str, const char separator, int *destination,
                         const uint8_t destinationSize);

bool checkCharsAtPosition(char *charBuffer, int *positions, char character);

void strCopyWithAutocompletion(char *destination, char *source, uint8_t destLen,
                               uint8_t srcLen);


/*==============================================================================
    Conversion
============================================================================= */

// Q15 <-> Float
float q15ToFloat(q15_t value);

q15_t floatToq15(float value);


// Acceleration units
float toG(q15_t value, uint16_t accelResolution);

float toMS2(q15_t value, uint16_t accelResolution);

float getFactorToMS2(uint16_t accelResolution);


/*==============================================================================
    FFTs
============================================================================= */

bool computeRFFT(q15_t *source, q15_t *destination, const uint16_t FFTlength,
                 bool inverse);

bool computeRFFT(q15_t *source, q15_t *destination, const uint16_t FFTlength,
                 bool inverse, q15_t *window);

void filterAndIntegrateFFT(q15_t *values, uint16_t sampleCount,
                           uint16_t samplingRate, uint16_t FreqLowerBound,
                           uint16_t FreqHigherBound, uint16_t scalingFactor,
                           bool twice = false);

float getMainFrequency(q15_t *fftValues, uint16_t sampleCount,
                       uint16_t samplingRate);

q15_t findMaxAscent(q15_t *batch, uint16_t batchSize, uint16_t max_count);


/*==============================================================================
    Hamming Window Presets
============================================================================= */

/* Hamming window definition: w(n) = 0.54 - 0.46 * cos(2 * Pi * n / N),
0 <= n <= N. Since we are using q15 values, we then have to do
round(w(n) * 2^15) to convert from float to q15 format.*/

__attribute__((section(".noinit2"))) const int magsize_512 = 257;
// 1/0.5400 = 1.8519
__attribute__((section(".noinit2"))) const float hamming_K_512 = 1.8519;
__attribute__((section(".noinit2"))) const float inverse_wlen_512 = 1/512.0;
//extern __attribute__((section(".noinit2"))) q15_t hamming_window_512 [512];
extern q15_t hamming_window_512 [512];

__attribute__((section(".noinit2"))) const int magsize_2048 = 1025;
// 1/0.5400 = 1.8519
__attribute__((section(".noinit2"))) const float hamming_K_2048 = 1.8519;
__attribute__((section(".noinit2"))) const float inverse_wlen_2048 = 1/2048.0;
//extern __attribute__((section(".noinit2"))) q15_t hamming_window_2048 [2048];
extern q15_t hamming_window_2048 [2048];


#endif // UTILITIES_H
