#include "Utilities.h"

/*==============================================================================
    Conversion
============================================================================= */

float q15ToFloat(q15_t value)
{
    return ((float) value) / 32768.0;
}

q15_t floatToq15(float value)
{
    return (q15_t) (32768 * value);
}


/*==============================================================================
    Arrays
============================================================================= */

/**
 * Return the K max values from given source array
 *
 * Implement a QuickSelect algorithm
 */
void getMaxCoefficients(float *source, uint16_t sourceCount,
                        float *destination, uint16_t destinationCount)
{
    // TODO Implement
//    if (sourceCount <= destinationCount)
//    {
//        // If source is smaller than number of requested coeffs, just return the
//        // full source.
//        copyArray(source, destination, sourceCount);
//        return;
//    }

}


/*==============================================================================
    FFTs
============================================================================= */


/**
 * Compute (inverse) RFFT and put it in destination
 *
 * The freq domain array size should be twice the time domain array size, so:
 * - if inverse is false, source size = FFTLength and
 *  destination size = 2 * FFTLength
 * - if inverse is true, source size = 2 * FFTLength and
 *  destination size = FFTLength
 * NB: FFT computation mofidy the source array.
 * @param source Source data array
 * @param destination Destination data array
 * @param FFTLength The length of the FFT (= length of time domain sample)
 * @param inverse False to compute forward FFT, true to compute inverse FFT
 */
void FFT::computeRFFT(q15_t *source, q15_t *destination,
                      const uint16_t FFTLength, bool inverse)
{
    arm_rfft_instance_q15 rfftInstance;
    arm_status armStatus = arm_rfft_init_q15(
        &rfftInstance,  // RFFT instance
        FFTLength,      // FFT length
        (int) inverse,  // 0: forward FFT, 1: inverse FFT
        1);             // 1: enable bit reversal output
    if (armStatus != ARM_MATH_SUCCESS)
    {
        if (loopDebugMode)
        {
            debugPrint("FFT / Inverse FFT computation failed");
        }
    }
    arm_rfft_q15(&rfftInstance, source, destination);
}

/**
 * Compute (inverse) RFFT while applying given window function
 *
 * see computeRFFT(q15_t *source, q15_t *destination, const uint16_t FFTlength,
 *                 bool inverse)
 * @param window The window function as an array of FFTLength coefficients, to
 *  apply to the time domain samples
 */
void FFT::computeRFFT(q15_t *source, q15_t *destination,
                    const uint16_t FFTLength, bool inverse, q15_t *window)
{
    if (!inverse) // Apply window
    {
        for (uint16_t i = 0; i < FFTLength; ++i)
        {
            source[i] = (q15_t) ((float) source[i] * (float) window[i] / 32768.);
        }
    }
    computeRFFT(source, destination, FFTLength, inverse);
    if (inverse) // "un-" window the iFFT
    {
        for (uint16_t i = 0; i < FFTLength; ++i)
        {
            destination[i] = (q15_t) round(
            (float) destination[i] * 32768. / (float) window[i]);
        }
    }
}

/**
 * Apply a bandpass filtering and integrate 1 or 2 times a FFT
 *
 * NB: The input FFT is be modified in place
 * @param values The FFT (as an array of size 2 * sampleCount as outputed by
 *  arm_rfft)
 * @param sampleCount The sample count
 * @param samplingRate The sampling rate or frequency
 * @param FreqLowerBound The frequency lower bound for bandpass filtering
 * @param FreqHigherBound The frequency higher bound for bandpass filtering
 * @param scalingFactor A scaling factor to stay in bound of q15 precision.
 *  WARNING: if twice is true, the scalingFactor will be applied twice.
 * @param twice False (default) to integrate once, true to integrate twice
 */
void FFT::filterAndIntegrate(q15_t *values, uint16_t sampleCount,
                             uint16_t samplingRate, uint16_t FreqLowerBound,
                             uint16_t FreqHigherBound, uint16_t scalingFactor,
                             bool twice)
{
    float df = (float) samplingRate / (float) sampleCount;
    uint16_t minIdx = (uint16_t) max(((float) FreqLowerBound / df), 1);
    uint16_t maxIdx = (uint16_t) min((float) FreqHigherBound / df, sampleCount);
    float omega = 2. * PI * df / (float) scalingFactor;
    // Apply high pass filter
    for (uint16_t i = 0; i < minIdx; ++i)
    {
        values[2 * i] = 0;
        values[2 * i + 1] = 0;
    }
    // Integrate accel FFT (divide by j * idx * omega)
    if (twice)
    {
        float factor = - 1. / sq(omega);
        for (uint16_t i = minIdx; i < maxIdx; ++i)
        {
            values[2 * i] = (q15_t) round(
                (float) values[2 * i] * factor / (float) sq(i));
            values[2 * i + 1] = (q15_t) round(
                (float) values[2 * i + 1] * factor / (float) sq(i));
        }
    }
    else
    {
        q15_t real(0), comp(0);
        for (uint16_t i = minIdx; i < maxIdx; ++i)
        {
            real = values[2 * i];
            comp = values[2 * i + 1];
            values[2 * i] = (q15_t) round(
                (float) comp / ((float) i * omega));
            values[2 * i + 1] = (q15_t) round(
                - (float) real / ((float) i * omega));
        }
    }
    // Apply low pass filter
    for (uint16_t i = maxIdx; i < sampleCount; ++i)
    {
        values[2 * i] = 0;
        values[2 * i + 1] = 0;
    }
}


/**
 * Return the squared amplitudes from the given FFT complex values
 *
 * @param fftValues Complexes fft coefficients as outputed by arm_rfft_q15
 * @param sampleCount The number of samples in time domain (fftValues size =
 *  2 * sampleCount)
 * @param destination Output array of size sampleCount to receive the squarred
 *  amplitude at each frequency.
 */
void FFTSquaredAmplitudes::getAmplitudes(q15_t *fftValues, uint16_t sampleCount,
                                         q15_t *destination)
{
    int32_t temp;
    for (uint16_t i = 0; i < sampleCount; ++i)
    {
        temp = sq((int32_t) (fftValues[2 * i])) +
            sq((int32_t) (fftValues[2 * i + 1]));
        destination[i] = (q15_t) (temp / 32768);
    }
    // FIXME Divide by sampleCount ?
}


/**
 * Return the RMS from squared FFT amplitudes
 */
q15_t FFTSquaredAmplitudes::getRMS(q15_t *amplitudes, uint16_t sampleCount)
{
    int32_t rms = 0;
    for (uint16_t i = 0; i < sampleCount; ++i)
    {
        rms += amplitudes[i];
    }
    return (q15_t) sqrt(16384 * rms);
    // NB: 16384 = 2**15 (because of q15_t) / 2 (because of RMS)
}


/**
 * Apply a bandpass filtering and integrate 1 or 2 times a FFT AMPLITUDES
 *
 * NB: The input array is modified in place.
 * @param amplitudes The FFT squared amplitudes (real^2 + complex^2)
 * @param sampleCount The sample count
 * @param samplingRate The sampling rate or frequency
 * @param FreqLowerBound The frequency lower bound for bandpass filtering
 * @param FreqHigherBound The frequency higher bound for bandpass filtering
 * @param scalingFactor A scaling factor to stay in bound of q15 precision.
 *  WARNING: if twice is true, the scalingFactor will be applied twice.
 * @param twice False (default) to integrate once, true to integrate twice
 *
 */
void FFTSquaredAmplitudes::filterAndIntegrate(
    q15_t *values, uint16_t sampleCount, uint16_t samplingRate,
    uint16_t FreqLowerBound, uint16_t FreqHigherBound, uint16_t scalingFactor,
    bool twice)
{
    float df = (float) samplingRate / (float) sampleCount;
    uint16_t minIdx = (uint16_t) max(((float) FreqLowerBound / df), 1);
    uint16_t maxIdx = (uint16_t) min((float) FreqHigherBound / df, sampleCount);
    float omega = 2. * PI * df;
    // Apply high pass filter
    for (uint16_t i = 0; i < minIdx; ++i)
    {
        values[i] = 0;
    }
    // Integrate accel FFT (divide by j * idx * omega)
    float factor = (float) scalingFactor / omega;
    if (twice)
    {
        factor = sq(factor);
        for (uint16_t i = minIdx; i < maxIdx; ++i)
        {
            values[i] = (q15_t) round(
                (float) values[i] * factor / (float) sq(i));
        }
    }
    else
    {
        for (uint16_t i = minIdx; i < maxIdx; ++i)
        {
            values[i] = (q15_t) round((float) values[i] * factor / (float) i);
        }
    }
    // Apply high pass filter
    for (uint16_t i = maxIdx; i < sampleCount; ++i)
    {
        values[i] = 0;
    }
}


/**
 * Return the main frequency, ie the frequency of the highest peak
 *
 * @param amplitudes The FFT (squarred) amplitudes for each frequency
 * @param sampleCount The number of sample in time domain (= size of amplitudes)
 * @param samplingRate The sampling rate, or frequency
 */
float FFTSquaredAmplitudes::getMainFrequency(q15_t *amplitudes,
                                             uint16_t sampleCount,
                                             uint16_t samplingRate)
{
    float df = (float) samplingRate / (float) sampleCount;
    uint16_t maxIdx = 0;
    q15_t maxVal = 0;
    // Ignore 1 first coeff (0Hz = DC component)
    for (uint16_t i = 1; i < sampleCount; ++i)
    {
        if (amplitudes[i] > maxVal)
        {
            maxVal = amplitudes[i];
            maxIdx = i;
        }
    }
    return (float) maxIdx * df;
}


/*==============================================================================
    Analytics
============================================================================= */

/**
 * Find the max asscent of a curve over maxCount consecutive points.
 */
q15_t findMaxAscent(q15_t *batch, uint16_t batchSize, uint16_t maxCount)
{
  q15_t buff[maxCount];
  for (uint16_t j = 0; j < maxCount; ++j)
  {
    buff[j] = 0;
  }
  uint16_t pos = 0;
  q15_t diff = 0;
  q15_t max_ascent = 0;
  q15_t max_ascent_candidate = 0;
  for (uint16_t i = 1; i < batchSize; ++i)
  {
    diff = batch[i] - batch[i - 1];
    if (diff >= 0)
    {
      buff[pos] = diff;
      ++pos;
      if (pos >= maxCount)
      {
        pos = 0;
      }
      max_ascent_candidate = 0;
      for (uint16_t j = 0; j < maxCount; ++j)
      {
        max_ascent_candidate += buff[j];
      }
      max_ascent = max(max_ascent, max_ascent_candidate);
    }
    else
    {
      pos = 0;
      for (uint16_t j = 0; j < maxCount; ++j)
      {
        buff[j] = 0;
      }
    }
  }
  return max_ascent;
}

