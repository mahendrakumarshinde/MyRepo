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


/*====================== Single Source Feature Computation functions ============================ */
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

/**
 * Compute and return the total signal energy along single axis
 */
float computeSignalEnergy(uint16_t sourceSize, q15_t* source)
{
  float energy = 0;
  for (uint16_t j = 0; j < sourceSize; j++)
  {
    energy += sq(compBuffer[j]);
  }
  energy /= sourceSize;
  return energy;
}

//TODO: Check what windowing function work best + maybe test some other pitch detection method
/**
 * Compute RFFT and return the index of the max factor
 * Check out this example: https://www.keil.com/pack/doc/CMSIS/DSP/html/arm_fft_bin_example_f32_8c-example.html
 * What we try to do is basically pitch detection: https://en.wikipedia.org/wiki/Pitch_detection_algorithm
 * However, have a look here: http://dsp.stackexchange.com/questions/11312/why-should-one-use-windowing-functions-for-fft
 * @param sourceSize the number of samples - must be 512 or 2048
 */
float computeRFFTMaxIndex(uint16_t sourceSize, q15_t *source); //uint8_t buffer_compute_index, IUBMX055::Axis axis, q15_t *hamming_window)
{
  q15_t compBuffer[sourceSize];
  q15_t rfftBuffer[sourceSize * 2];
  switch(sourceSize)
  {
    case 512:
      arm_mult_q15(source, hamming_window_512, compBuffer, sourceSize);
      break;
    case 2048:
      arm_mult_q15(source, hamming_window_2048, compBuffer, sourceSize);
      break;
    default:
      return -1; //Fail if sourceSize is not supported
  }

  // RFFT
  arm_status status = arm_rfft_init_q15(&rfftInstance,  // RFFT instance
                                        sourceSize,     // FFT length
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
  for (int i = 2; i < sourceSize; i++) {
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
              sourceSize - 2,   // array size
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
 */
float computeAcousticDB(uint16_t sourceSize, q15_t *source)
{
  // Strangely, log10 is very slow. It's better to use it the least possible, so we
  // multiply data points as much as possible and then log10 the results when we have to.
  // We can multiply data points until we reach uint64_t limit (so 2^64)
  // First, compute the limit for our accumulation
  q15_t maxVal = 0;
  uint32_t maxIdx = 0;
  arm_max_q15(source, sourceSize, maxVal, maxIdx); // Get max data value
  uint64_t limit = pow(2, 63 - round(log(maxVal) / log(2)));
  // Then, use it to compute our data
  uint64_t accu = 1;
  float audioDB_val = 0.0;
  int data = 0;
  for (int i = 0; i < sourceSize; i++)
  {
    data = abs(source[i]);
    if (data > 0)
    {
      accu *= data;
    }
    if (accu >= limit)
    {
      audioDB_val += log10(accu);
      accu = 1;
    }
  }
  audioDB_val += log10(accu);
  audioDB_val *= 20. / sourceSize;
  return audioDB_val;
}

/*====================== Multi Source Feature Computation functions ============================ */
/**
 * Compute and return the total signal energy along several axis
 */
float computeSignalEnergy(uint8_t sourceCount, uint16_t* sourceSize, q15_t* source)
{
  float totalEnergy = 0;
  for (int i = 0; i < sourceCount; i++)
  {
    totalEnergy += computeSignalEnergy(sourceSize[i], source[i])
  }
  return totalEnergy;
}

/**
 * Compute and return single axis velocity using acceleration RMS and pitch
 * @param source should be {value to compare, threshold, acceleration data}
 * @return velocity approximation from FFT and pitch if source[0] >= source[1], else 0
    eg: In initial implementation, we had:
    source = {total acceleration energy,
              total acceleration energy normal threshold,
              acceleration values}
 */
float computeVelocity(uint8_t sourceCount, uint16_t* sourceSize, q15_t* source)
{
  if (source[0][0] < source[1][0]) {
    return 0; // level 0: not cutting
  }
  else {
    maxIndex = computeRFFTMaxIndex(sourceSize[2], source[2]);
    return computeRMS(sourceSize[2], source[2]) * 1000 / (2 * PI * maxIndex);
  }
}

