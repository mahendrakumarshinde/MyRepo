#include <arm_math.h>
#include "IUUtilities.h"

/*====================== General utility function ============================ */

float q15ToFloat(q15_t value)
{
    return ((float) value) / 32768.;
}

q15_t floatToQ15(float value)
{
    ((q15_t) (32768 * value))
}

/**
 * Compute and return the normalized root min square
 * @param sourceSize the length of the array
 * @param source a q15_t array (elements are int16_t0
 */
float computeRMS(uint16_t sourceSize, q15_t *source)
{
  float avg(0), avg2(0);
  for (int i = 0; i < sourceSize; i++)
  {
    avg += (float)source[i];
    avg2 += sq((float)source[i]);

  return sqrt(avg2 / sourceSize - sq(avg / sourceSize));
}


/*====================== Feature Computation functions ============================ */


float  computeDefaultDataCollectionQ15 (uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[], float *m_destination[])
{
  for (int i = 0; i < sourceCount; i++)
  {
    arm_q15_to_float(source[i], m_destination[i], (uint32_t) sourceSize[i]);
  }
  return 0;
}



/**
 * Compute and return the total signal energy along one or several axis
 * @param sourceCount the number of sources (= the number of axis)
 * @param sourceSize  pointer to an array of size sourceCount
 * @param source      pointer to an array q15_t[sourceCount][sourceSize[i for i in sourceCount]]
 */
float computeSignalEnergy(uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[])
{
  float energy = 0;
  float totalEnergy = 0;
  for (uint8_t i = 0; i < sourceCount; i++)
  {
    for (uint16_t j = 0; j < sourceSize[i]; j++)
    {
      energy += sq(q15ToFloat(source[i][j]));
    }
    totalEnergy += energy / (float) sourceSize[i];
    energy = 0
  }
  totalEnergy /= (float) sourceCount;
  return totalEnergy;
}

/**
 * Compute and return the normalized sum of all sources
 * @param sourceCount the number of sources
 * @param sourceSize  pointer to an array of size sourceCount
 * @param source      pointer to an array q15_t[sourceCount][sourceSize[i for i in sourceCount]]
 */
float computeSumOf(uint8_t sourceCount, const uint16_t* sourceSize, float* source[])
{
  float total = 0;
  float grandTotal = 0;
  for (uint8_t i = 0; i < sourceCount; i++)
  {
    for (uint16_t j = 0; j < sourceSize[i]; j++)
    { 
      total += q15ToFloat(source[i][j])
    }
    grandTotal += total / (float) sourceSize[i];
    total = 0
  }
  grandTotal /= (float) sourceCount;
  return grandTotal;
}


//TODO: Check what windowing function work best + maybe test some other pitch detection method
/**
 * Compute RFFT and return the index of the max factor - WORK FOR A SINGLE SOURCE
 * Since a single index is returned, only work for a single source (sourceCount = 1)
 * Check out this example: https://www.keil.com/pack/doc/CMSIS/DSP/html/arm_fft_bin_example_f32_8c-example.html
 * What we try to do is basically pitch detection: https://en.wikipedia.org/wiki/Pitch_detection_algorithm
 * However, have a look here: http://dsp.stackexchange.com/questions/11312/why-should-one-use-windowing-functions-for-fft
 * @param sourceCount the number of sources - must be 1
 * @param sourceSize  pointer to an array of size 1 - must be {512} or {2048} in current implementation
 * @param source      pointer to an array q15_t[sourceCount][sourceSize[i for i in sourceCount]]
 */
float computeRFFTMaxIndex(uint8_t sourceCount, const uint16_t* sourceSize, q15_t* source[])
{
  if (sourceCount != 1)
  {
    return -1; // sourceCount expected to be 1
  }
  uint16_t ss = sourceSize[0];
  q15_t compBuffer[ss];
  switch(ss)
  {
    case 512:
      arm_mult_q15(source[0], hamming_window_512, compBuffer, ss);
      break;
    case 2048:
      arm_mult_q15(source[0], hamming_window_2048, compBuffer, ss);
      break;
    default:
      return -1; //Fail if sourceSize is not supported
  }
  // RFFT
  q15_t rfftBuffer[ss * 2];
  arm_status status = arm_rfft_init_q15(&rfftInstance,  // RFFT instance
                                        ss        ,     // FFT length
                                        0,              // 0: forward FFT, 1: inverse FFT
                                        1);             // 0: disable bit reversal output, 1: enable it
  if (status != ARM_MATH_SUCCESS)
  {
      return -1; // failure
  }
  arm_rfft_q15(&rfftInstance, compBuffer, rfftBuffer);
  // TODO check what is better between block 1 and block 2
  // block 1 ===========================================
  // Find index of max factor
  float flatness(0), index(0);
  // TODO: check why we start at index 2
  for (int i = 2; i < ss; i++) {
    if (rfftBuffer[i] > flatness) {
      flatness = rfftBuffer[i];
      index = i;
    }
  }
  // ===================================================
  // block 2 ===========================================
  /*
  q15_t flatness = 0;
  uint32_t index = 0;
  // TODO: check why we start at index 2
  arm_max_q15(&rfftBuffer[2],   // array
              ss - 2,   // array size
              flatness,
              index);
  */
  // ===================================================
  return ((float) index);
}

/**
 * Return the average sound level in DB over an array of sound pressure data
 * Acoustic decibel formula: 20 * log10(p / p_ref) where
 * p is the sound pressure
 * p_ref is the reference pressure for sound in air = 20 mPa (or 1 mPa in water)
 * @param sourceCount the number of sources
 * @param sourceSize  pointer to an array of size sourceCount
 * @param source      pointer to an array q15_t[sourceCount][sourceSize[i for i in sourceCount]]
 */
float computeAcousticDB(uint8_t sourceCount, const uint16_t* sourceSize, q15_t* source[])
{
  // Strangely, log10 is very slow. It's better to use it the least possible, so we
  // multiply data points as much as possible and then log10 the results when we have to.
  // We can multiply data points until we reach uint64_t limit (so 2^64)
  // First, compute the limit for our accumulation
  q15_t maxVal = 0;
  uint32_t maxIdx = 0;
  uint64_t limit = 0;
  int data = 0;
  uint64_t accu = 1;
  float audioDB = 0.;
  float meanAudioDB = 0.;
  for (uint8_t i = 0; i < sourceCount; i++)
  {
    arm_max_q15(source[i], sourceSize[i], maxVal, maxIdx); // Get max data value
    limit = pow(2, 63 - round(log(maxVal) / log(2)));
    // Then, use it to compute our data
    for (uint16_t j = 0; j < sourceSize[i]; j++)
    {
      data = abs(source[i][j]);
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
    accu = 1;
    meanAudioDB += 20.0 * audioDB / (float) sourceSize[i];
    audioDB = 0;
  }
  meanAudioDB /= (float) sourceCount;
  return audioDB;
}

/**
 * Compute and return single axis velocity using acceleration RMS and pitch
 * @param sourceCount the number of sources - should be 2
 * @param sourceSize  pointer to an array of size sourceCount - eg: {512, 1}
 * @param source      should be {acceleration values,
 *                               Acceleration energy operationState}
 */
float computeVelocity(uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[])
{
  if (source[1][0] == operationState::notCutting)
  {
    return 0;
  }
  else
  {
    maxIndex = computeRFFTMaxIndex(1, sourceSize, source);
    return computeRMS(sourceSize[0], source[0]) * 1000 / (2 * PI * maxIndex);
  }
}

