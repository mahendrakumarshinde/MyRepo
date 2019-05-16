#ifndef TEST_UTILITIES_H
#define TEST_UTILITIES_H

#include <ArduinoUnit.h>
#include "../Utilities.h"
#include "UnitTestData.h"


/**
 * Test getMax function
 */
test(Utilities__getMax)
{
    q15_t maxVal;
    uint32_t maxIdx;
    q15_t a1[5] = {1, -2, -3, 2, 1};
    getMax(a1, 5, &maxVal, &maxIdx)
    assertEqual(maxVal, 2);
    assertEqual(maxIdx, 3);
    getMax(a1, 3, &maxVal, &maxIdx)
    assertEqual(maxVal, 1);
    assertEqual(maxIdx, 0);

    float maxValF;
    float a2[3] = {-12.46, 0.001, 0};
    getMax(a2, 3, &maxValF, &maxIdx)
    assertEqual(maxValF,  0.001);
    assertEqual(maxIdx, 1);
}


/**
 * Test sumArray function
 */
test(Utilities__sumArray)
{
    q15_t a1[5] = {1, -2, -3, 2, 1};
    assertEqual(sumArray(a1, 5), -1);
    assertEqual(sumArray(a1, 3), -4);

    float a2[3] = {-12.46, 0.001, 0};
    assertEqual(sumArray(a2, 3), -12.459);
    assertEqual(sumArray(a2, 0), 0);
}


/**
 * Test copyArray function
 */
test(Utilities__copyArray)
{
    q15_t src[5] = {1, -2, -3, 2, 1};
    q15_t dest[3];
    copyArray(src, dest, 3);
    assertEqual(dest[0], src[0]);
    assertEqual(dest[1], src[1]);
    assertEqual(dest[2], src[2]);
}


/**
 * Test RFFT-related functions
 *
 * These tests not only aim at testing the function themselves, but also at
 * illustrating the loss of precision in the q15_t RFFT and in the FFT
 * integration. In the FFT integration itself, the loss of precision can be
 * countered by using an rescaling factor of k bits (see getRescalingFactor).
 *
 * - RFFT::computeRFFT
 * - RFFT::getRescalingFactor
 * - RFFT::filterAndIntegrateFFT
 */
test(Utilities__RFFT)
{
    q15_t rfftValues[2 * testSampleCount];
    copyArray(q15TestData, sharedTestArray, testSampleCount);
    // Store RMS value of the series for future reference
    float orgRMS = 0;
    for (uint16_t i = 0; i < testSampleCount; ++i)
    {
        orgRMS += sq((float) sharedTestArray[i]);
    }
    orgRMS = sqrt(orgRMS / testSampleCount);
    assertEqual(round(orgRMS), 8060);

    /***** Test computeRFFT *****/
    // Forward
    RFFT::computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);

    assertEqual(rfftValues[0], 7895);    // Found 7897 in Python
    assertEqual(rfftValues[1], 0);       // Found 0 in Python
    assertEqual(rfftValues[2], -1);      // Found -0.9 in Python
    assertEqual(rfftValues[3], 1);       // Found 1.6 in Python
    assertEqual(rfftValues[80], -2);
    assertEqual(rfftValues[81], 8);
    assertEqual(rfftValues[82], 329);    // Found 329.9 in Python
    assertEqual(rfftValues[83], -1085);  // Found -1084.8 in Python
    assertEqual(rfftValues[84], -3);
    assertEqual(rfftValues[85], -9);
    // Inverse fft
    RFFT::computeRFFT(rfftValues, sharedTestArray, testSampleCount, true);
    float newRMS = 0;
    for (uint16_t i = 0; i < testSampleCount; ++i)
    {
        // Scaling factor of 512=2**9 because of FFT internal rescaling
        newRMS += sq((float) sharedTestArray[i] * 512.);
    }
    newRMS = sqrt(newRMS / testSampleCount);
    // Illustrate loss of precision
    assertEqual(round(orgRMS), 8060);
    assertEqual(round(newRMS), 7381);


    /***** Test getRescalingFactorForIntegral *****/
    copyArray(q15TestData, sharedTestArray, testSampleCount);
    RFFT::computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);

    uint16_t scaling = RFFT::getRescalingFactorForIntegral(
        rfftValues, testSampleCount, testSamplingRate);
    assertEqual(scaling, 8192);


    /***** Test filterAndIntegrate *****/
    /* Also show difference of precision with different rescaling factor */

    // Inapdated rescaling => imprecise result
    RFFT::filterAndIntegrate(rfftValues, testSampleCount, testSamplingRate, 0,
                            1000, 256, false);
    assertEqual(rfftValues[0], 0);
    assertEqual(rfftValues[1], 0);
    assertEqual(rfftValues[2], 21);
    assertEqual(rfftValues[3], 21);
    assertEqual(rfftValues[4], 0);
    assertEqual(rfftValues[5], 21);
    assertEqual(rfftValues[82], -552);
    assertEqual(rfftValues[83], -167);
    RFFT::computeRFFT(rfftValues, sharedTestArray, testSampleCount, true);
    float unpreciseRMS = 0;
    for (uint16_t i = 0; i < testSampleCount; ++i)
    {
        // factor of 2 = 512 / 256 (internal FFT rescaling / scaling factor)
        unpreciseRMS += sq((float) sharedTestArray[i] * 2.0);
    }
    unpreciseRMS = sqrt(unpreciseRMS / testSampleCount);
    assertEqual(round(unpreciseRMS * 1000), 4776); // Python 3378

    // Apdated rescaling, as outputed by RFFT::getRescalingFactorForIntegral
    // => precise result
    copyArray(q15TestData, sharedTestArray, testSampleCount);
    RFFT::computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);
    RFFT::filterAndIntegrate(rfftValues, testSampleCount, testSamplingRate, 0,
                             1000, 8192, false);
    assertEqual(rfftValues[0], 0);
    assertEqual(rfftValues[1], 0);
    assertEqual(rfftValues[2], 668);
    assertEqual(rfftValues[3], 668);
    assertEqual(rfftValues[4], 0);
    assertEqual(rfftValues[5], 668);
    assertEqual(rfftValues[80], 134);
    assertEqual(rfftValues[81], 33);
    assertEqual(rfftValues[82], -17665);
    assertEqual(rfftValues[83], -5357);
    assertEqual(rfftValues[84], -143);
    assertEqual(rfftValues[85], 48);
    RFFT::computeRFFT(rfftValues, sharedTestArray, testSampleCount, true);
    float preciseRMS = 0;
    for (uint16_t i = 0; i < testSampleCount; ++i)
    {
        // factor of 1/16 = 512 / 8192 (internal FFT rescaling / scaling factor)
        preciseRMS += sq((float) sharedTestArray[i] / 16.0);
    }
    preciseRMS = sqrt(preciseRMS / testSampleCount);
    assertEqual(round(preciseRMS * 1000), 3369); // Python 3378

    // Test the passband filtering
    copyArray(q15TestData, sharedTestArray, testSampleCount);
    RFFT::computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);
    RFFT::filterAndIntegrate(rfftValues, testSampleCount, testSamplingRate, 5,
                             1000, 8192, false);
    assertEqual(rfftValues[0], 0);
    assertEqual(rfftValues[1], 0);
    assertEqual(rfftValues[2], 0);
    assertEqual(rfftValues[3], 0);
    assertEqual(rfftValues[4], 0);
    assertEqual(rfftValues[5], 668);
    assertEqual(rfftValues[82], -17665);
    assertEqual(rfftValues[83], -5357);
    RFFT::computeRFFT(rfftValues, sharedTestArray, testSampleCount, true);
    float filteredRMS = 0;
    for (uint16_t i = 0; i < testSampleCount; ++i)
    {
        // factor of 1/16 = 512 / 8192 (internal FFT rescaling / scaling factor)
        filteredRMS += sq((float) sharedTestArray[i] / 16);
    }
    filteredRMS = sqrt(filteredRMS / testSampleCount);
    assertEqual(round(filteredRMS * 1000), 3366); // Python 3375


    // 2nd Order integration
    copyArray(q15TestData, sharedTestArray, testSampleCount);
    RFFT::computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);
    RFFT::filterAndIntegrate(rfftValues, testSampleCount, testSamplingRate, 0,
                             1000, 8192, false);
    scaling = RFFT::getRescalingFactorForIntegral(
        rfftValues, testSampleCount, testSamplingRate);
    assertEqual(scaling, 256);

    RFFT::filterAndIntegrate(rfftValues, testSampleCount, testSamplingRate, 0,
                             1000, 256, false);
    assertEqual(rfftValues[0], 0);
    assertEqual(rfftValues[1], 0);
    assertEqual(rfftValues[2], 13935);
    assertEqual(rfftValues[3], -13935);
    assertEqual(rfftValues[4], 6967);
    assertEqual(rfftValues[5], 0);
    assertEqual(rfftValues[80], 17);
    assertEqual(rfftValues[81], -70);
    assertEqual(rfftValues[82], -2726);
    assertEqual(rfftValues[83], 8988);
    assertEqual(rfftValues[84], 24);
    assertEqual(rfftValues[85], 71);
    RFFT::computeRFFT(rfftValues, sharedTestArray, testSampleCount, true);
    float displacementRMS = 0;
    for (uint16_t i = 0; i < testSampleCount; ++i)
    {
        // factor of 1/4.096 = 1000 * 512 / (8192 * 256)
        // (1000 * internal FFT rescaling / scaling factor)
        displacementRMS += sq((float) sharedTestArray[i] / 4.0960);
    }
    displacementRMS = sqrt(displacementRMS / testSampleCount);
    assertEqual(round(displacementRMS * 100), 2423); // Python 2686
}


/**
 * Test RFFTAmplitudes-related functions
 *
 * - RFFTAmplitudes::getAmplitudes
 * - RFFTAmplitudes::getRMS
 * - RFFTAmplitudes::getRescalingFactorForIntegral
 * - RFFTAmplitudes::filterAndIntegrate
 * - RFFTAmplitudes::getMainFrequency
 */
test(Utilities__RFFTAmplitudes)
{
    q15_t rfftValues[2 * testSampleCount];
    q15_t amplitudes[testSampleCount / 2 + 1];
    copyArray(q15TestData, sharedTestArray, testSampleCount);
    // Store RMS value of the series for future reference
    q15_t avg = 0;
    arm_mean_q15(sharedTestArray, testSampleCount, &avg);
    float orgRMSNoDC(0), orgRMSDC(0);
    for (uint16_t i = 0; i < testSampleCount; ++i)
    {
        orgRMSDC += sq((float) sharedTestArray[i]);
        orgRMSNoDC += sq((float) (sharedTestArray[i] - avg));
    }
    orgRMSDC = sqrt(orgRMSDC / testSampleCount);
    orgRMSNoDC = sqrt(orgRMSNoDC / testSampleCount);
    assertEqual(round(orgRMSDC), 8060);
    assertEqual(round(orgRMSNoDC), 1609);

    /***** Test getAmplitudes *****/
    RFFT::computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);

    RFFTAmplitudes::getAmplitudes(rfftValues, testSampleCount, amplitudes);
    assertEqual(amplitudes[0], 7895);
    assertEqual(amplitudes[1], 1);
    assertEqual(amplitudes[2], 2);
    assertEqual(amplitudes[40], 8);
    assertEqual(amplitudes[41], 1133);
    assertEqual(amplitudes[42], 9);

    /***** Test getRMS *****/
    float amplitudeRMS = RFFTAmplitudes::getRMS(amplitudes, testSampleCount);
    assertEqual(round(amplitudeRMS), 1608);

    float amplitudeRMSWithDC = RFFTAmplitudes::getRMS(amplitudes,
                                                      testSampleCount, false);
    assertEqual(round(amplitudeRMSWithDC), 8057);


    /***** Test getRescalingFactorForIntegral *****/
    q15_t scaling = RFFTAmplitudes::getRescalingFactorForIntegral(
        amplitudes, testSampleCount, testSamplingRate);
    assertEqual(scaling, 8192);


    /***** Test filterAndIntegrate *****/
    // 1st Integration
    RFFTAmplitudes::filterAndIntegrate(amplitudes, testSampleCount,
                                       testSamplingRate, 0, 1000, 8192, false);
    assertEqual(amplitudes[0], 0);
    assertEqual(amplitudes[1], 668);
    assertEqual(amplitudes[2], 668);
    assertEqual(amplitudes[3], 445);
    assertEqual(amplitudes[40], 134);
    assertEqual(amplitudes[41], 18447);
    assertEqual(amplitudes[42], 143);
    amplitudeRMS = RFFTAmplitudes::getRMS(amplitudes, testSampleCount);
    assertEqual(round(amplitudeRMS), 27554);
    // Rescaled, but round to 3 decimal precision
    assertEqual(round(amplitudeRMS * 1000.0 / 8192.0), 3364);
    // RMS without DC should be the same than with DC, since integration
    // remove DC component
    amplitudeRMSWithDC = RFFTAmplitudes::getRMS(amplitudes, testSampleCount,
                                                false);
    assertEqual(amplitudeRMSWithDC, amplitudeRMS);


    // 2nd Integration
    scaling = RFFTAmplitudes::getRescalingFactorForIntegral(
        amplitudes, testSampleCount, testSamplingRate);
    assertEqual(scaling, 256);

    RFFTAmplitudes::filterAndIntegrate(amplitudes, testSampleCount,
                                       testSamplingRate, 0, 1000, 256, false);
    assertEqual(amplitudes[0], 0);
    assertEqual(amplitudes[1], 13935);
    assertEqual(amplitudes[2], 6967);
    assertEqual(amplitudes[40], 70);
    assertEqual(amplitudes[41], 9386);
    assertEqual(amplitudes[42], 71);

    amplitudeRMS = RFFTAmplitudes::getRMS(amplitudes, testSampleCount);
    assertEqual(round(amplitudeRMS), 46822);
    // RMS without DC should be the same than with DC, since integration
    // remove DC component
    amplitudeRMSWithDC = RFFTAmplitudes::getRMS(amplitudes, testSampleCount,
                                                false);
    assertEqual(amplitudeRMSWithDC, amplitudeRMS);
}


#endif // IUTESTUTILITIES_H
