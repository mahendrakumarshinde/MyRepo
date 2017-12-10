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


/*==============================================================================
    RFFTs
============================================================================= */

/**
 * Compute (inverse) RFFT and put it in destination
 *
 * The freq domain array size should be twice the time domain array size, so:
 * - if inverse is false, source size = FFTLength and
 *  destination size = 2 * FFTLength
 * - if inverse is true, source size = 2 * FFTLength and
 *  destination size = FFTLength
 * NB: RFFT computation mofidy the source array.
 * @param source Source data array
 * @param destination Destination data array
 * @param FFTLength The length of the FFT (= length of time domain sample)
 * @param inverse False to compute forward FFT, true to compute inverse FFT
 * @param window Optionnal - The window function as an array of FFTLength
 *  coefficients, to apply to the time domain samples
 */
void RFFT::computeRFFT(q15_t *source, q15_t *destination,
                      const uint16_t FFTLength, bool inverse, q15_t *window)
{
    if (window != NULL && !inverse) // Apply window
    {
        for (uint16_t i = 0; i < FFTLength; ++i)
        {
            source[i] = (q15_t) ((float) source[i] *
                                 (float) window[i] / 32768.);
        }
    }
    arm_rfft_instance_q15 rfftInstance;
    arm_cfft_radix4_instance_q15 cfftInstance;
    arm_status armStatus = arm_rfft_init_q15(
        &rfftInstance,  // RFFT instance
        &cfftInstance,
        FFTLength,      // FFT length
        (int) inverse,  // 0: forward RFFT, 1: inverse RFFT
        1);             // 1: enable bit reversal output
    if (armStatus != ARM_MATH_SUCCESS)
    {
        return;
    }
    arm_rfft_q15(&rfftInstance, source, destination);
    if (window != NULL && inverse) // "un-" window the iFFT
    {
        for (uint16_t i = 0; i < FFTLength; ++i)
        {
            destination[i] = (q15_t) round(
            (float) destination[i] * 32768. / (float) window[i]);
        }
    }
}

/**
 * Get max rescaling factor usable (without overflowing) for given RFFT values
 *
 * When intergrating in the frequency domain, FFT coefficients are divided by
 * the frequency. Since FFT coeffs are stored on signed 16bits, loss of
 * precision can significantly impact results.
 * NB: DC / constant RFFT component (coeff at 0Hz) is ignored in this function
 * since RFFT integration assume that DC component is 0.
 * @param values RFFT coefficients as outputed by arm_rfft_q15
 * @param sampleCount The time domain sample count (values size = 2 * FFTLength)
 * @param samplingRate The sampling rate of the time domain data
 */
uint16_t RFFT::getRescalingFactorForIntegral(q15_t *values, uint16_t sampleCount,
                                            uint16_t samplingRate)
{
    float real(0), comp(0), maxVal(2);
    float df = (float) samplingRate / (float) sampleCount;
    float omega = 2. * PI * df;
    // skip i=0 (DC component)
    for (uint16_t i = 1; i < sampleCount / 2; ++i)  // Up to Nyquist Frequency
    {
        real = (float) abs(values[2 * i]) / ((float) i * omega);
        comp = (float) abs(values[2 * i + 1]) / ((float) i * omega);
        maxVal = max(maxVal, max(real, comp));
    }
    uint8_t rescaleBit = 15 - ceil(log(maxVal) / log(2));
    return (uint16_t) pow(2, rescaleBit);

}

/**
 * Apply a bandpass filtering and integrate 1 or 2 times a RFFT
 *
 * NB: The input RFFT is modified in place
 * @param values The RFFT (as an array of size 2 * sampleCount as outputed by
 *  arm_rfft)
 * @param sampleCount The sample count
 * @param samplingRate The sampling rate or frequency
 * @param FreqLowerBound The frequency lower bound for bandpass filtering
 * @param FreqHigherBound The frequency higher bound for bandpass filtering
 * @param scalingFactor A scaling factor to stay in bound of q15 precision.
 *  WARNING: if twice is true, the scalingFactor will be applied twice.
 * @param twice False (default) to integrate once, true to integrate twice
 */
void RFFT::filterAndIntegrate(q15_t *values, uint16_t sampleCount,
                             uint16_t samplingRate, uint16_t FreqLowerBound,
                             uint16_t FreqHigherBound, uint16_t scalingFactor,
                             bool twice)
{
    float df = (float) samplingRate / (float) sampleCount;
    uint16_t nyquistIdx = sampleCount / 2;
    uint16_t lowIdx = (uint16_t) max(((float) FreqLowerBound / df), 1);
    uint16_t highIdx = (uint16_t) min((float) FreqHigherBound / df,
                                      nyquistIdx + 1);
    float omega = 2. * PI * df / (float) scalingFactor;
    // Apply filtering
    for (uint16_t i = 0; i < lowIdx; ++i)  // Low cut
    {
        values[2 * i] = 0;
        values[2 * i + 1] = 0;
    }
    for (uint16_t i = highIdx; i < nyquistIdx + 1; ++i)  // High cut
    {
        values[2 * i] = 0;
        values[2 * i + 1] = 0;
    }
    // Integrate RFFT (divide by j * idx * omega)
    if (twice)
    {
        float factor = - 1. / sq(omega);
        for (uint16_t i = lowIdx; i < highIdx; ++i)
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
        for (uint16_t i = lowIdx; i < highIdx; ++i)
        {
            real = values[2 * i];
            comp = values[2 * i + 1];
            values[2 * i] = (q15_t) round(
                (float) comp / ((float) i * omega));
            values[2 * i + 1] = (q15_t) round(
                - (float) real / ((float) i * omega));
        }
    }
    /* Negative frequencies (in 2nd half of array) are complex conjugate of
    positive frequencies (in 1st half of array) */
    for (uint16_t i = 1; i < nyquistIdx; ++i)
    {
        values[2 * (nyquistIdx + i)] = values[2 * (nyquistIdx - i)];
        values[2 * (nyquistIdx + i) + 1] = - values[2 * (nyquistIdx - i) + 1];
    }
}


/*==============================================================================
    RFFT Amplitudes
============================================================================= */

/**
 * Return the amplitudes from the given RFFT complex values
 *
 * @param rfftValues Complexes fft coefficients as outputed by arm_rfft_q15
 * @param sampleCount The number of samples in time domain (rfftValues size is
 *  2 * sampleCount when outputed by arm_rfft, but we'll use only the 1st half)
 * @param destination Output array of size sampleCount / 2 (since dealing with
 *  RFFT) to receive the amplitude at each frequency.
 */
void RFFTAmplitudes::getAmplitudes(q15_t *rfftValues, uint16_t sampleCount,
                                   q15_t *destination)
{
    int32_t temp;
    for (uint16_t i = 0; i < sampleCount / 2 + 1; ++i)
    {
        temp = sq((int32_t) (rfftValues[2 * i])) +
               sq((int32_t) (rfftValues[2 * i + 1]));
        destination[i] = (q15_t) sqrt(temp);
    }
}


/**
 * Return the RMS from RFFT amplitudes
 */
float RFFTAmplitudes::getRMS(q15_t *amplitudes, uint16_t sampleCount,
                             bool removeDC)
{
    uint32_t rms = 0;
    if (!removeDC)
    {
        rms += (uint32_t) sq((int32_t) (amplitudes[0]));
    }
    for (uint16_t i = 1; i < sampleCount / 2 + 1; ++i)
    {
        /* factor 2 because RFFT and we use only half (positive part) of freq
        spectrum */
        rms += 2 * (uint32_t) sq((int32_t) (amplitudes[i]));
    }
    return (float) sqrt(rms);
}

/**
 * Get max rescaling factor usable (without overflowing) for given amplitudes
 *
 * When intergrating in the frequency domain, RFFT amplitudes are divided by
 * the frequency. Since RFFT amplitude are stored on signed 16bits, loss of
 * precision can significantly impact results.
 * NB: DC / constant RFFT component (amplitude at 0Hz) is ignored in this
 * function since FFT integration assume that DC component is 0.
 * @param amplitudes RFFT amplitudes as outputed by getAmplitudes
 * @param sampleCount The time domain sample count
 * @param samplingRate The sampling rate of the time domain data
 */
uint16_t RFFTAmplitudes::getRescalingFactorForIntegral(
    q15_t *amplitudes, uint16_t sampleCount, uint16_t samplingRate)
{
    float val(0), maxVal(2);
    float df = (float) samplingRate / (float) sampleCount;
    float omega = 2. * PI * df;
    // skip i=0 (DC component)
    for (uint16_t i = 1; i < sampleCount / 2 + 1; ++i)
    {
        val = (float) amplitudes[i] / ((float) i * omega);
        maxVal = max(maxVal, val);
    }
    uint8_t rescaleBit = 15 - ceil(log(maxVal) / log(2));
    return (uint16_t) pow(2, rescaleBit);
}

/**
 * Apply a bandpass filtering and integrate 1 or 2 times a RFFT amplitudes
 *
 * NB: The input array is modified in place.
 * @param amplitudes The RFFT amplitudes (real^2 + complex^2)
 * @param sampleCount The sample count
 * @param samplingRate The sampling rate or frequency
 * @param FreqLowerBound The frequency lower bound for bandpass filtering
 * @param FreqHigherBound The frequency higher bound for bandpass filtering
 * @param scalingFactor A scaling factor to stay in bound of q15 precision.
 *  WARNING: if twice is true, the scalingFactor will be applied twice.
 * @param twice False (default) to integrate once, true to integrate twice
 *
 */
void RFFTAmplitudes::filterAndIntegrate(
    q15_t *amplitudes, uint16_t sampleCount, uint16_t samplingRate,
    uint16_t FreqLowerBound, uint16_t FreqHigherBound, uint16_t scalingFactor,
    bool twice)
{
    float df = (float) samplingRate / (float) sampleCount;
    uint16_t nyquistIdx = sampleCount / 2;
    uint16_t lowIdx = (uint16_t) max(((float) FreqLowerBound / df), 1);
    uint16_t highIdx = (uint16_t) min((float) FreqHigherBound / df,
                                      nyquistIdx + 1);
    float omega = 2. * PI * df / (float) scalingFactor;
    // Apply filtering
    for (uint16_t i = 0; i < lowIdx; ++i)  // low cut
    {
        amplitudes[i] = 0;
    }
    for (uint16_t i = highIdx; i < nyquistIdx + 1; ++i)
    {
        amplitudes[i] = 0;
    }
    // Integrate RFFT (divide by j * idx * omega)
    if (twice)
    {
        float factor = 1. / sq(omega);
        for (uint16_t i = lowIdx; i < highIdx; ++i)
        {
            amplitudes[i] =
                (q15_t) round((float) amplitudes[i] * factor / (float) sq(i));
        }
    }
    else
    {
        for (uint16_t i = lowIdx; i < highIdx; ++i)
        {
            amplitudes[i] =
                (q15_t) round((float) amplitudes[i] / ((float) i * omega));
        }
    }
}


/**
 * Return the main frequency, ie the frequency of the highest peak
 *
 * @param amplitudes The RFFT amplitudes for each frequency
 * @param sampleCount The number of sample in time domain
 *  (size of amplitudes = sampleCount / 2 + 1)
 * @param samplingRate The sampling rate / frequency
 */
float RFFTAmplitudes::getMainFrequency(q15_t *amplitudes, uint16_t sampleCount,
                                       uint16_t samplingRate)
{
    float df = (float) samplingRate / (float) sampleCount;
    uint16_t maxIdx = 0;
    q15_t maxVal = 0;
    // Ignore 1 first coeff (0Hz = DC component)
    for (uint16_t i = 1; i < sampleCount / 2 + 1; ++i)
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

