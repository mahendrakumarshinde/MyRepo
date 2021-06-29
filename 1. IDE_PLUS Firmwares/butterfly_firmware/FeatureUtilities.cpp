#include "FeatureUtilities.h"
#include "RawDataState.h"


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
    arm_status armStatus = arm_rfft_init_q15(
        &rfftInstance,  // RFFT instance
        FFTLength,      // FFT length
        (int) inverse,  // 0: forward RFFT, 1: inverse RFFT
        1);             // 1: enable bit reversal output
    if (armStatus != ARM_MATH_SUCCESS)
    {
        if (loopDebugMode)
        {
            debugPrint("RFFT / Inverse RFFT computation failed");
        }
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

void RFFT::computeRFFT(q31_t *source, q31_t *destination,
                       const uint32_t FFTLength, bool inverse)
{
    arm_rfft_instance_q31 rfftInstance;
    arm_status armStatus = arm_rfft_init_q31(
        &rfftInstance,  // RFFT instance
        FFTLength,      // FFT length
        (uint32_t) inverse,  // 0: forward RFFT, 1: inverse RFFT
        1);             // 1: enable bit reversal output
    if (armStatus != ARM_MATH_SUCCESS)
    {
        if (loopDebugMode)
        {
            debugPrint("RFFT / Inverse RFFT computation failed");
        }
    }
    arm_rfft_q31(&rfftInstance, source, destination);
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
uint16_t RFFT::getRescalingFactorForIntegral(
        q15_t *values, uint16_t sampleCount, uint16_t samplingRate)
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
    uint8_t rescaleBit = max(13 - ceil(log(maxVal) / log(2)), 0);
    return (uint16_t) pow(2, rescaleBit);
}

uint32_t RFFT::getRescalingFactorForIntegral(
        q31_t *values, uint16_t sampleCount, uint16_t samplingRate)
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
    uint8_t rescaleBit = max(13 - ceil(log(maxVal) / log(2)), 0);
    return (uint32_t) pow(2, rescaleBit);

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

void RFFT::filterAndIntegrate(q31_t *values, uint16_t sampleCount,
                              uint16_t samplingRate, uint16_t FreqLowerBound,
                              uint16_t FreqHigherBound, uint32_t scalingFactor,
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
            values[2 * i] = (q31_t) ((float) values[2 * i] * factor /
                                     (float) sq(i));
            values[2 * i + 1] = (q31_t) ((float) values[2 * i + 1] * factor /
                                         (float) sq(i));
        }
    }
    else
    {
        q31_t real(0), comp(0);
        for (uint16_t i = lowIdx; i < highIdx; ++i)
        {
            real = values[2 * i];
            comp = values[2 * i + 1];
            values[2 * i] = (q31_t) ((float) comp / ((float) i * omega));
            values[2 * i + 1] = (q31_t) (- (float) real / ((float) i * omega));
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

void RFFTAmplitudes::getAmplitudes(q31_t *rfftValues, uint16_t sampleCount,
                                   q31_t *destination)
{
    for (uint16_t i = 0; i < sampleCount / 2 + 1; ++i)
    {
        destination[i] = sqrt(sq(rfftValues[2 * i]) +
                              sq(rfftValues[2 * i + 1]));
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

float RFFTAmplitudes::getRMS(q31_t *amplitudes, uint16_t sampleCount,
                             bool removeDC)
{
    uint32_t rms = 0;
    if (!removeDC) {
        rms += (uint32_t) sq((amplitudes[0]));
    }
    for (uint16_t i = 1; i < sampleCount / 2 + 1; ++i) {
        /* factor 2 because RFFT and we use only half (positive part) of freq
        spectrum */
        rms += 2 * (uint32_t) sq((amplitudes[i]));
    }
    return (float) sqrt(rms);
}

/* getAccelerationMean from raw acceleration values

*/

float RFFTAmplitudes::getAccelerationMean(q15_t *values,uint16_t sampleCount)

{
        // get the New RFFT acceleration Amplitudes from Raw values
        float newAccelAmplitudes[sampleCount];
        float accelAmplitudesSum=0;    
        //Serial.print("[");
        for(uint16_t i = 2 ; i < sampleCount; i++){
               
           newAccelAmplitudes[i] = values[i];//*resolution;     // q15
           accelAmplitudesSum += newAccelAmplitudes[i];
           //Serial.print(values[i]);Serial.print(","); 
         }
         //Serial.println("]");
        float accelAmplitudesMean = accelAmplitudesSum/(sampleCount -2) ;

        return (accelAmplitudesMean);    
}

/* Remove newAcceleration Mean from newAccelAmplitudes
 *  
 */
void RFFTAmplitudes::removeNewAccelMean(q15_t *values, uint16_t sampleCount,float newAccelMean,q15_t *newAccelAmplitudes)

{   
    for(uint16_t i= 2; i < sampleCount; i++){
                newAccelAmplitudes[i] = values[i] - (q15_t) newAccelMean;
           }

}

/* get the new accelerationRMS values from newAccelerationAmplitudes 
 *  
 */
float RFFTAmplitudes::getNewAccelRMS(q15_t *newAccelAmplitudes,uint16_t sampleCount)

{
    uint32_t rms = 0;
//    if (!removeDC) {
//        rms += (uint32_t) sq((newAccelAmplitudes[0]));
//    }
    for (uint16_t i = 2; i < sampleCount ; ++i) {
        /* factor 2 because RFFT and we use only half (positive part) of freq
        spectrum */
        //rms += 2 * (uint32_t) sq((newAccelAmplitudes[i]));
        rms +=  (uint32_t) sq((newAccelAmplitudes[i]));
    }
    return (float) abs(sqrt(rms/sampleCount));
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
    uint8_t rescaleBit = max(13 - ceil(log(maxVal) / log(2)), 0);
    return (uint16_t) pow(2, rescaleBit);
}

uint32_t RFFTAmplitudes::getRescalingFactorForIntegral(
    q31_t *amplitudes, uint16_t sampleCount, uint16_t samplingRate)
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
    uint8_t rescaleBit = max(13 - ceil(log(maxVal) / log(2)), 0);
    return (uint32_t) pow(2, rescaleBit);
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

void RFFTAmplitudes::filterAndIntegrate(
    q31_t *amplitudes, uint16_t sampleCount, uint16_t samplingRate,
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
                (q31_t) ((float) amplitudes[i] * factor / (float) sq(i));
        }
    }
    else
    {
        for (uint16_t i = lowIdx; i < highIdx; ++i)
        {
            amplitudes[i] =
                (q31_t) ((float) amplitudes[i] / ((float) i * omega));
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
    uint32_t maxIdx = 0;
    q15_t maxVal = 0;
    // Ignore 1 first coeff (0Hz = DC component)
    getMax(&amplitudes[1], (uint32_t) sampleCount, &maxVal, &maxIdx);
    return (float) (maxIdx + 1) * df;
}

float RFFTAmplitudes::getMainFrequency(q31_t *amplitudes, uint16_t sampleCount,
                                       uint16_t samplingRate)
{
    float df = (float) samplingRate / (float) sampleCount;
    uint32_t maxIdx = 0;
    q31_t maxVal = 0;
    // Ignore 1 first coeff (0Hz = DC component)
    getMax(&amplitudes[1], (uint32_t) sampleCount, &maxVal, &maxIdx);
    return (float) (maxIdx + 1) * df;
}

// Compute rpm
float RFFTFeatures::computeRPM(q15_t *amplitudes,int m_lowRPMFrequency,int m_highRPMFrequency,float rpm_threshold,float df,
                                float resolution,float scalingFactor,uint8_t bufferType)
{   
    // debugPrint("\n\nlRPM : ",false);debugPrint(m_lowRPMFrequency);
    // debugPrint("hRPM : ",false);debugPrint(m_highRPMFrequency);
    // debugPrint("RPM_TRH : ",false);debugPrint(rpm_threshold);
    if(FeatureStates::opStateStatusFlag == 0){ 
        return 0;           //return 0 for device is in ideal condition
    }
    uint8_t MAX_PEAK_COUNT = 200 ;
    float peakfreq[MAX_PEAK_COUNT];                 // store the first top 200 Peaks 
    float val,freq(0); 
    float maxVal;
    uint32_t maxIdx=0; 
    uint8_t count = 0;
    int lower_index = (int)(m_lowRPMFrequency/df ); // ceiling(lower_bound/df)
    int upper_index = (int)(m_highRPMFrequency/df - 1 ); // floor(upper_bound/df)
    
    switch (bufferType)
    {
    case 0: // acceleration
        {
           // debugPrint(" ACCEL : [ ",false);
            for (uint16_t i = lower_index; i <= upper_index ; i++)
            {
                val = 2 * (uint32_t) sq((int32_t) (amplitudes[i]) ) ;
                val = val*resolution;
               // debugPrint(val,false);debugPrint(",",false);
                if((val > 0) && (count < MAX_PEAK_COUNT) )
                {
                    peakfreq[count] = val;
                    count++;
                }   
            }
            //debugPrint( " ] ",true);
        }
        break;
    case 1: // Velocity 
        {
            //debugPrint("VELL FFT : ");
            float factor = (1000*resolution/scalingFactor);
            // debugPrint("lower_bound : ",false);debugPrint(lower_index);
            // debugPrint("upper_bound : ",false);debugPrint(upper_index);
            // debugPrint("scalingFactor : ",false);debugPrint(scalingFactor);
            // debugPrint("Factor : ",false);debugPrint(factor);
            // debugPrint("df : ",false);debugPrint(df);
            // debugPrint("resolution : ",false);debugPrint(resolution);
            // debugPrint("\n Vel Peaks [ ",false);
            for (uint16_t i = lower_index; i <= upper_index ; i++)
            {
                val = 2 * (uint32_t) sq((int32_t) (amplitudes[i]) ) ;
                val = val*factor;
                //debugPrint(val,false);debugPrint(",",false);
                if((val > rpm_threshold) && (count < MAX_PEAK_COUNT) )
                {
                    peakfreq[count] = val;
                    count++;
                }   
            }
            //debugPrint(" ] ",true);
        } 
        break;
    case 2: // Displacement
        {
            debugPrint("Displacement");
        }
        break;
    
    default:
        break;
    }
    if(count != 0) {
        // debugPrint("Top Peaks : [ ",false);
        // for (size_t i = 0; i < count; i++)
        // {
        //     debugPrint(peakfreq[i],false);debugPrint(",",false);
        // }
        // debugPrint(" ] ");
        getMax(peakfreq,(uint32_t)count, &maxVal, &maxIdx);
        freq = (maxIdx + lower_index)*df;
        return (float) (freq*60) ;
    }else{
        return 0;
    }
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

//New RPM Algo

float RFFTFeatures::computeRPM(q15_t *amplitudes,int m_lowRPMFrequency,int m_highRPMFrequency,float rpm_threshold,float df,
                                float resolution,float scalingFactor,uint8_t bufferType, uint32_t amplitudeCount)
{   
    uint32_t MAX_PEAK_COUNT = 128 ;
    float amplitudesCopy[MAX_PEAK_COUNT];
    float peakAmp[64];
    int peakFreqIndex[64];
    uint16_t peakIndex = 0;
    float tempVal=0;
    float finalPeak = 0;
    uint32_t finalIdx=0;    
    uint32_t tempIdx=0;
    int pCount=0;
    float val,freq(0); 
    uint32_t buffCount = 0;
    int lower_index = (int)(m_lowRPMFrequency/df ); // ceiling(lower_bound/df)
    int upper_index = (int)(m_highRPMFrequency/df - 1 ); // floor(upper_bound/df)
    
    int sectionCount1;
    int sectionCount2;
    int sectionIdx = 0;
    int dCount = 0;

    if(FeatureStates::opStateStatusFlag == 0){ 
        return 0;           //return 0 for device is in ideal condition
    }

    if(upper_index < lower_index) {
        return 0;           //return 0 invalid condition
    }
    if(upper_index > amplitudeCount){
        upper_index = amplitudeCount;
        if(debugMode){debugPrint("upper_index Out of bound");}
    }
    dCount = (upper_index - lower_index)+1;
    sectionCount1 = dCount/MAX_PEAK_COUNT;     //for devision
    sectionCount2 = dCount%MAX_PEAK_COUNT;      //for less than 128 count

    switch (bufferType)
    {
   /* case 0: // acceleration
        {
           // debugPrint(" ACCEL : [ ",false);
            for (uint16_t i = lower_index; i <= upper_index ; i++)
            {
                val = 2 * (uint32_t) sq((int32_t) (amplitudes[i]) ) ;
                val = val*resolution;
               // debugPrint(val,false);debugPrint(",",false);
                if((val > 0) && (count < MAX_PEAK_COUNT) )
                {
                    peakfreq[count] = val;
                    count++;
                }   
            }
            //debugPrint( " ] ",true);
        }
        break;
    */    
    case 1: // Velocity 
        {            
            float factor = (1000*resolution/scalingFactor);
            if(loopDebugMode) {
                // debugPrint("sectionCount1 : ",false);
                // debugPrint(sectionCount1);
                // debugPrint("sectionCount2 : ",false);
                // debugPrint(sectionCount2);
                // debugPrint("amplitudeCount : ",false);debugPrint(amplitudeCount);
                // debugPrint("lower_bound : ",false);debugPrint(lower_index);
                // debugPrint("upper_bound : ",false);debugPrint(upper_index);
                // debugPrint("scalingFactor : ",false);debugPrint(scalingFactor);
                // debugPrint("Factor : ",false);debugPrint(factor);
                // debugPrint("df : ",false);debugPrint(df);
                // debugPrint("resolution : ",false);debugPrint(resolution);
                // debugPrint("dCount : ",false);
                // debugPrint(dCount);                
                // debugPrint("Block[",false);
                // debugPrint(pCount,false);
                // debugPrint("] Vel Peaks {",false);
            }
            int iCnt = 0;
            if(dCount >= MAX_PEAK_COUNT) {
                for (uint16_t i = lower_index; i <= upper_index ; i++) {
                    val = 2 * (uint32_t) sq((int32_t) (amplitudes[i]) ) ;
                    val = val*factor;                
                    amplitudesCopy[iCnt] = val;
                    if(loopDebugMode) {                    
                        //debugPrint(val,false);debugPrint(",",false);
                    }
                    buffCount++;
                    iCnt++;
                    if(iCnt >= MAX_PEAK_COUNT && sectionIdx < sectionCount1) {
                        getMax(amplitudesCopy,(uint32_t)iCnt, &tempVal, &tempIdx);
                        peakAmp[pCount] = tempVal;
                        peakFreqIndex[pCount] = tempIdx;
                        if(loopDebugMode) {
                            // debugPrint(" } ",true);
                            // debugPrint("peakFreqIndex_1 : ",false);
                            // debugPrint(peakFreqIndex[pCount]);
                            // debugPrint("PeakFreq : ",false);
                            // debugPrint(peakAmp[pCount]);
                            // debugPrint("Block[",false);
                            // debugPrint(pCount+1,false);
                            // debugPrint("] Vel Peaks {",false);                        
                        }                    
                        iCnt = 0;
                        tempVal = 0;
                        tempIdx = 0;
                        pCount++;
                        sectionIdx++;
                    }
                    else if(iCnt == sectionCount2 && sectionIdx >= sectionCount1) {
                        getMax(amplitudesCopy,(uint32_t)iCnt, &tempVal, &tempIdx);
                        peakAmp[pCount] = tempVal;               //memset tempArray need to clear temparray
                        peakFreqIndex[pCount] = tempIdx;
                        if(loopDebugMode) {
                            // debugPrint("\nbuffCount : ",false);
                            // debugPrint(buffCount);
                            // debugPrint("peakFreqIndex_1 : ",false);
                            // debugPrint(peakFreqIndex[pCount]);
                            // debugPrint("PeakFreq : ",false);
                            // debugPrint(peakAmp[pCount]);
                        }
                        tempVal = 0;
                        tempIdx = 0;
                        pCount++;
                    }
                }
            }
            else if(dCount >= 0 && dCount < MAX_PEAK_COUNT) {
                for (uint16_t i = lower_index; i <= upper_index ; i++) {
                    val = 2 * (uint32_t) sq((int32_t) (amplitudes[i]) ) ;
                    val = val*factor;                
                    amplitudesCopy[iCnt] = val;
                    if(loopDebugMode) {                    
                        //debugPrint(val,false);debugPrint(",",false);
                    }
                    buffCount++;
                    iCnt++;
                    if(iCnt >= dCount) {
                        getMax(amplitudesCopy,(uint32_t)iCnt, &tempVal, &tempIdx);
                        peakAmp[pCount] = tempVal;
                        peakFreqIndex[pCount] = tempIdx;
                        if(loopDebugMode) {
                            // debugPrint(" } ",true);
                            // debugPrint("peakFreqIndex_1 : ",false);
                            // debugPrint(peakFreqIndex[pCount]);
                            // debugPrint("PeakFreq : ",false);
                            // debugPrint(peakAmp[pCount]);
                        }
                    }
                }            
            }
        } 
        break;
    case 2: // Displacement
        {
            //debugPrint("Displacement");
        }
        break;
    
    default:
        break;
    }
    
    if(buffCount != 0) {
        if(pCount > 0) {
            if(loopDebugMode) {
                // debugPrint("Top Peaks : [ ",false);
                // for (size_t i = 0; i < pCount-1; i++) {
                //     debugPrint(peakAmp[i],false);debugPrint(",",false);
                // }
                // debugPrint(" ] ");
            }
            getMax(peakAmp,(uint32_t)pCount,&finalPeak,&finalIdx);
            //peakIndex = peakFreqIndex[finalIdx];
            if(finalIdx == 0)
                peakIndex = peakFreqIndex[finalIdx];
            else
                peakIndex = (MAX_PEAK_COUNT*finalIdx) + peakFreqIndex[finalIdx];
        }
        else {
            if(loopDebugMode) {
                // debugPrint("Top Peak : [ ",false);
                // debugPrint(peakAmp[0],false);debugPrint(",",false);
                // debugPrint(" ] ");
            }
            finalPeak = peakAmp[0];
            peakIndex = peakFreqIndex[0];
        }
        freq = (peakIndex + lower_index)*df;
        if(loopDebugMode) {
            // debugPrint("finalPeak : ",false);
            // debugPrint(finalPeak);
            // debugPrint("finalIdx : ",false);
            // debugPrint(finalIdx);
            // debugPrint("peakFreqIndex[finalIdx] : ",false);            
            // debugPrint(peakFreqIndex[finalIdx]);
            // debugPrint("peakIndex : ",false);
            // debugPrint(peakIndex);
            // debugPrint("Peak freq : ",false);
            // debugPrint(freq);
            // debugPrint("(round((float) (freq*60))): ",false);
            // debugPrint((round((float) (freq*60))));
        }
        return (round((float) (freq*60))) ;
    }
    else {
        return 0;
    }
}
