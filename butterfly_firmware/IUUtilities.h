#ifndef IUUTILITIES_H
#define IUUTILITIES_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>

#include "IULogger.h"

/* ============================= Operation Enums ============================= */
  /** 
 * Usage modes are user controlled, they describe how the device is being used
 */
enum usageMode : uint8_t
{
  calibration      = 0,
  configuration    = 1,
  operation       = 2,
  usageModeCount   = 3
};

/**
 * Operation modes are mostly user controlled, with some automatic mode switching
 */
enum operationMode : uint8_t
{
  run              = 0,
  charging         = 1,
  dataCollection   = 2,
  record           = 3,
  sleep            = 4,
  opModeCount      = 5
}; // The number of different operation modes

/**
 * Operation states describe the production status, inferred from calculated features
 * and user-defined thresholds
 */
enum operationState : uint8_t {idle            = 0,
                               normalCutting   = 1,
                               warningCutting  = 2,
                               badCutting      = 3,
                               opStateCount    = 4}; // The number of different operation states

/**
 * Power modes are programmatically controlled and managed by the conductor depending
 * on the operation modes, states, and desired sampling rate
 */
enum powerMode : uint8_t {deepSuspend          = 0,
                          suspend              = 1,
                          standBy              = 2,
                          low                  = 3,
                          normal               = 4,
                          powerModeCount       = 5};


/*====================== General utility function ============================ */

// String processing
uint8_t splitString(String str, const char separator, String *destination, uint8_t destinationSize);

uint8_t splitStringToInt(String str, const char separator, int *destination, const uint8_t destinationSize);

bool checkCharsAtPosition(char *charBuffer, int *positions, char character);

void strCopyWithAutocompletion(char *destination, char *source, uint8_t destLen, uint8_t srcLen);

// Conversion
inline float q15ToFloat(q15_t value) { return ((float) value) / 32768.0; }

inline q15_t floatToq15(float value) { return (q15_t) (32768 * value); }

inline float q4_11ToFloat(q15_t value) { return ((float) value) / 2048.0; }

inline q15_t floatToq4_11(float value) { return (q15_t) (2048 * value); }

inline float q13_2ToFloat(q15_t value) { return ((float) value) / 4.0; }

inline q15_t floatToq13_2(float value) { return (q15_t) (4 * value); }

inline float g_to_ms2(float value) { return 9.8 * value; }

inline q15_t g_to_ms2(q15_t value) { return 9.8 * value; }

inline float ms2_to_g(float value) { return value / 9.8; }

inline q15_t ms2_to_g(q15_t value) { return value / 9.8; }


inline q15_t getMax(q15_t *values, uint16_t count)
{
  if (count == 0)
  {
    return 0;
  }
  q15_t maxVal = values[0];
  for (uint16_t i = 0; i < count; ++i)
  {
    if (maxVal < values[i])
    {
      maxVal = values[i];
    }
  }
  return maxVal;
}

inline void copyArray(q15_t *source, q15_t *dest, int arrSize)
{
  for (int i = 0; i < arrSize; ++i)
  {
    dest[i] = source[i];
  }
}

/*=========================== Math functions ================================= */

float computeRMS(uint16_t sourceSize, q15_t *source, float (*transform)(q15_t));

float computeNormalizedRMS(uint16_t sourceSize, q15_t *source, float (*transform)(q15_t));

float computeSignalEnergy(q15_t *values, uint16_t sampleCount, uint16_t samplingFreq, float scalingFactor = 1., bool removeMean = false);

float computeSignalPower(q15_t *values, uint16_t sampleCount, uint16_t samplingFreq, float scalingFactor = 1., bool removeMean = false);

float computeSignalRMS(q15_t *values, uint16_t sampleCount, uint16_t samplingFreq, float scalingFactor = 1., bool removeMean = false);

float multiArrayMean(uint8_t arrCount, const uint16_t* arrSizes, float **arrays);

bool computeRFFT(q15_t *source, q15_t *destination, const uint16_t FFTlength, bool inverse);

bool computeRFFT(q15_t *source, q15_t *destination, const uint16_t FFTlength, bool inverse, q15_t *window);

void filterAndIntegrateFFT(q15_t *values, uint16_t sampleCount, uint16_t samplingRate, uint16_t FreqLowerBound, uint16_t FreqHigherBound, uint16_t scalingFactor, bool twice = false);

float getMainFrequency(q15_t *fftValues, uint16_t sampleCount, uint16_t samplingRate);


//==============================================================================
//========================== Hamming Window Presets ============================
//==============================================================================
// Hamming window definition: w(n) = 0.54 - 0.46 * cos(2 * Pi * n / N), 0 <= n <= N
// Since we are using q15 values, we then have to do round(w(n) * 2^15) to convert from float to q15 format.

__attribute__((section(".noinit2"))) const int magsize_512 = 257;
__attribute__((section(".noinit2"))) const float hamming_K_512 = 1.8519;         // 1/0.5400
__attribute__((section(".noinit2"))) const float inverse_wlen_512 = 1/512.0;
//extern __attribute__((section(".noinit2"))) q15_t hamming_window_512 [512];
extern q15_t hamming_window_512 [512];

__attribute__((section(".noinit2"))) const int magsize_2048 = 1025;
__attribute__((section(".noinit2"))) const float hamming_K_2048 = 1.8519;        // 1/0.5400
__attribute__((section(".noinit2"))) const float inverse_wlen_2048 = 1/2048.0;
//extern __attribute__((section(".noinit2"))) q15_t hamming_window_2048 [2048];
extern q15_t hamming_window_2048 [2048];


#endif // IUUTILITIES_H
