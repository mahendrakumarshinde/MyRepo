#ifndef IUUTILITIES_H
#define IUUTILITIES_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>

#include "IULogger.h"

/* ============================= Operation Modes ============================= */

/**
 * Power modes are enforced at component level and are controlled by the either
 * automatically or by the user
 */
class powerMode
{
  public:
    enum  option : uint8_t
    {
      ACTIVE  = 0,
      SLEEP   = 1,
      SUSPEND = 2,
      COUNT   = 3,
    };
};

/**
 * Usage modes are user controlled, they describe how the device is being used
 */
class usageMode
{
  public:
    enum  option : uint8_t
    {
      CALIBRATION   = 0,
      CONFIGURATION = 1,
      OPERATION     = 2,
      COUNT         = 3,
    };
};

/**
 * Acquisition modes are mostly user controlled, with some automatic mode switching
 */
class aquisitionMode
{
  public:
    enum  option : uint8_t
    {
      FEATURE   = 0,
      EXTENSIVE = 1,
      RECORD    = 2,
      NONE      = 3,
      COUNT     = 4,
    };
};

/**
 * Operation states describe the production status, inferred from calculated features
 * and user-defined thresholds
 */
class operationState
{
  public:
    enum  option : uint8_t
    {
      IDLE    = 0,
      NORMAL  = 1,
      WARNING = 2,
      DANGER  = 3,
      COUNT   = 4,
    };
};


/*====================== General utility function ============================ */

// String processing
uint8_t splitString(String str, const char separator, String *destination, uint8_t destinationSize);

uint8_t splitStringToInt(String str, const char separator, int *destination, const uint8_t destinationSize);

bool checkCharsAtPosition(char *charBuffer, int *positions, char character);

void strCopyWithAutocompletion(char *destination, char *source, uint8_t destLen, uint8_t srcLen);

// Conversion
inline float q15ToFloat(q15_t value) { return ((float) value) / 32768.0; }

inline q15_t floatToq15(float value) { return (q15_t) (32768 * value); }

inline float toG(q15_t value, q15_t accelResolution) { return (float) value * (float) accelResolution / 32768.0; }

inline float toMS2(q15_t value, q15_t accelResolution) { return (float) value * (float) accelResolution * 9.8065 / 32768.0; }

inline float getFactorToMS2(q15_t accelResolution) { return (float) accelResolution * 9.8065 / 32768.0; }

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
