#ifndef TEST_UTILITIES_H
#define TEST_UTILITIES_H

#include <ArduinoUnit.h>
#include "../Utilities.h"
#include "UnitTestData.h"


/**
 * Test getMax function
 */
test(getMax)
{
    q15_t a1[5] = {1, -2, -3, 2, 1};
    assertEqual(getMax(a1, 5), 2);
    assertEqual(getMax(a1, 3), 1);

    float a2[3] = {-12.46, 0.001, 0};
    assertEqual(getMax(a2, 3), 0.001);
    assertEqual(getMax(a2, 0), 0);
}


/**
 * Test sumArray function
 */
test(sumArray)
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
test(copyArray)
{
    q15_t src[5] = {1, -2, -3, 2, 1};
    q15_t dest[3];
    copyArray(src, dest, 3);
    assertEqual(dest[0], src[0]);
    assertEqual(dest[1], src[1]);
    assertEqual(dest[2], src[2]);
}


/**
 * Test FFT-related functions
 *
 * - FFT::computeRFFT
 * - FFT::filterAndIntegrateFFT
 */
test(FFT_computation)
{
    q15_t rfftValues[1024];
    copyArray(q15TestData, sharedTestArray, testSampleCount);
    // Store RMS value of the series for future reference
    float orgRMS = 0;
    for (uint16_t i = 0; i < testSampleCount; ++i)
    {
        orgRMS += sq((float) sharedTestArray[i]);
    }
    orgRMS = sqrt(orgRMS / testSampleCount);
//    assertEqual(round(orgRMS), 8060);
    Serial.println("orgRMS");
    Serial.println(orgRMS);

    /***** Test computeRFFT *****/
    // Forward
    FFT::computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);
    assertEqual(rfftValues[0], 7895);    // Found 7897 in Python
    assertEqual(rfftValues[1], 0);       // Found 0 in Python
    assertEqual(rfftValues[2], -1);      // Found -0.9 in Python
    assertEqual(rfftValues[3], 1);       // Found 1.6 in Python
    assertEqual(rfftValues[82], 329);    // Found 329.9 in Python
    assertEqual(rfftValues[83], -1085);  // Found -1084.8 in Python
    // Inverse fft
    FFT::computeRFFT(rfftValues, sharedTestArray, testSampleCount, true);
    float newRMS = 0;
    for (uint16_t i = 0; i < testSampleCount; ++i)
    {
        newRMS += sq((float) sharedTestArray[i]);
    }
    newRMS = sqrt(newRMS / testSampleCount);
//    // Illustrate loss of precision
//    assertEqual(round(orgRMS), 8060);
//    assertEqual(round(newRMS), 7660);
    Serial.println("newRMS");
    Serial.println(newRMS);


    /***** Test filterAndIntegrateFFT *****/
    copyArray(q15TestData, sharedTestArray, testSampleCount);
    FFT::computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);
    FFT::filterAndIntegrate(rfftValues, testSampleCount, testSamplingRate, 0,
                            1000, 512, false);
    assertEqual(rfftValues[0], 0);
    assertEqual(rfftValues[1], 0);
    assertEqual(rfftValues[2], 42);
    assertEqual(rfftValues[3], 42);
    assertEqual(rfftValues[4], 0);
    assertEqual(rfftValues[5], 42);
    assertEqual(rfftValues[82], -1104);
    assertEqual(rfftValues[83], -335);

    float integralRMS1 = 0;
    for (uint16_t i = 0; i < testSampleCount; ++i)
    {
        integralRMS1 += sq((float) sharedTestArray[i]);
    }
    integralRMS1 = sqrt(integralRMS1 / testSampleCount);
    // New RMS must match the original
//    assertEqual(round(integralRMS1 * 1000), 3497); // Python 3378
    Serial.println("integralRMS1");
    Serial.println(integralRMS1);


    copyArray(q15TestData, sharedTestArray, testSampleCount);
    FFT::computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);
    FFT::filterAndIntegrate(rfftValues, testSampleCount, testSamplingRate, 5,
                            1000, 512, false);
    assertEqual(rfftValues[0], 0);
    assertEqual(rfftValues[1], 0);
    assertEqual(rfftValues[2], 0);
    assertEqual(rfftValues[3], 0);
    assertEqual(rfftValues[4], 0);
    assertEqual(rfftValues[5], 42);
    assertEqual(rfftValues[82], -1104);
    assertEqual(rfftValues[83], -335);

    float integralRMS2 = 0;
    for (uint16_t i = 0; i < testSampleCount; ++i)
    {
        integralRMS2 += sq((float) sharedTestArray[i]);
    }
    integralRMS2 = sqrt(integralRMS2 / testSampleCount);
    // New RMS must match the original
//    assertEqual(round(integralRMS2 * 1000), 3476); // Python 3375
    Serial.println("integralRMS2");
    Serial.println(integralRMS2);

}


/**
 * Test FFTSquaredAmplitudes-related functions
 *
 * - FFTSquaredAmplitudes::getAmplitudes
 * - FFTSquaredAmplitudes::getRMS
 * - FFTSquaredAmplitudes::filterAndIntegrate
 * - FFTSquaredAmplitudes::getMainFrequency
 */
test(FFTSquaredAmplitudes_computation)
{

}


/**
 * Test FFT-related functions
 *
 */
test(fft_computation)
{
    copyArray(q15TestData, sharedTestArray, testSampleCount);
    q15_t maxAscent = findMaxAscent(sharedTestArray, testSampleCount, 4);
    assertEqual(maxAscent, 4144);

//    copyArray(q15TestData, sharedTestArray, testSampleCount);
//    float energy1 = computeSignalEnergy(sharedTestArray, testSampleCount, testSamplingRate,
//                                        1, false);
//    float energy2 = computeSignalEnergy(sharedTestArray, testSampleCount, testSamplingRate,
//                                        2, false);
//    float energy3 = computeSignalEnergy(sharedTestArray, testSampleCount, testSamplingRate,
//                                        1, true);
//    assertEqual(round(energy1), 33258676);
//    assertEqual(round(energy2), 133034704);
//    assertEqual(round(energy3), 1325821);
//
//    float power1 = computeSignalPower(sharedTestArray, testSampleCount, testSamplingRate, 1,
//                                      false);
//    float power2 = computeSignalPower(sharedTestArray, testSampleCount, testSamplingRate, 2,
//                                      false);
//    float power3 = computeSignalPower(sharedTestArray, testSampleCount, testSamplingRate, 1,
//                                      true);
//    assertEqual(round(power1), 64958348);
//    assertEqual(round(power2), 259833392);
//    assertEqual(round(power3), 2589493);
//
//    float rms1 = computeSignalRMS(sharedTestArray, testSampleCount, testSamplingRate, 1,
//                                  false);
//    float rms2 = computeSignalRMS(sharedTestArray, testSampleCount, testSamplingRate, 10,
//                                  false);
//    float rms3 = computeSignalRMS(sharedTestArray, testSampleCount, testSamplingRate, 1,
//                                  true);
//    assertEqual(round(rms1 * 100), 805967); // Round to 2 decimal place
//    assertEqual(round(rms2 * 100), 8059685); // Round to 2 decimal place
//    assertEqual(round(rms3 * 100), 160919); // Round to 2 decimal place

    q15_t rfftValues[1024];

    copyArray(q15TestData, sharedTestArray, testSampleCount);
    FFT::computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);
    assertEqual(rfftValues[0], 7895);   // found 7897 in Python
    assertEqual(rfftValues[1], 0);      // found 0 in Python
    assertEqual(rfftValues[2], -1);      // found -0.9 in Python
    assertEqual(rfftValues[3], 1);      // found 1.6 in Python
    assertEqual(rfftValues[82], 329);    // found 329.9 in Python
    assertEqual(rfftValues[83], -1085);  //found -1084.8 in Python

    // Inverse fft
    copyArray(q15TestData, sharedTestArray, testSampleCount);
    FFT::computeRFFT(rfftValues, sharedTestArray, testSampleCount, true);
//    float orgRMS = computeSignalRMS(sharedTestArray, testSampleCount, testSamplingRate, 1,
//                                    false);
//    float newRMS = computeSignalRMS(sharedTestArray, testSampleCount, testSamplingRate, 1,
//                                    false) * 512;
    //    // Illustrate loss of precision
    //    assertEqual(round(orgRMS), 8060);
    //    assertEqual(round(newRMS), 7381);

//    float mainFreq = getMainFrequency(rfftValues, testSampleCount, testSamplingRate);
//    assertEqual(round(mainFreq * 100), 8008); // Around 80 Hz

    copyArray(q15TestData, sharedTestArray, testSampleCount);
    FFT::computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);
    FFT::filterAndIntegrate(rfftValues, testSampleCount, testSamplingRate, 0, 1000, 512,
                            false);
    assertEqual(rfftValues[0], 0);
    assertEqual(rfftValues[1], 0);
    assertEqual(rfftValues[2], 42);
    assertEqual(rfftValues[3], 42);
    assertEqual(rfftValues[4], 0);
    assertEqual(rfftValues[5], 42);
    assertEqual(rfftValues[82], -1104);
    assertEqual(rfftValues[83], -335);


    copyArray(q15TestData, sharedTestArray, testSampleCount);
    FFT::computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);
    FFT::filterAndIntegrate(rfftValues, testSampleCount, testSamplingRate, 5, 1000, 512,
                            false);
    assertEqual(rfftValues[0], 0);
    assertEqual(rfftValues[1], 0);
    assertEqual(rfftValues[2], 0);
    assertEqual(rfftValues[3], 0);
    assertEqual(rfftValues[4], 0);
    assertEqual(rfftValues[5], 42);
    assertEqual(rfftValues[82], -1104);
    assertEqual(rfftValues[83], -335);

//    copyArray(q15TestData, sharedTestArray, testSampleCount);
//    computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);
//    float velRMS1 = computeFullVelocity(rfftValues, testSampleCount, testSamplingRate,
//                                        0, 1000, 1);
//
//    copyArray(q15TestData, sharedTestArray, testSampleCount);
//    computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);
//    float velRMS2 = computeFullVelocity(rfftValues, testSampleCount, testSamplingRate,
//                                        0, 1000, 9806.5 * 4. / 32768.);
//
//    copyArray(q15TestData, sharedTestArray, testSampleCount);
//    computeRFFT(sharedTestArray, rfftValues, testSampleCount, false);
//    float velRMS3 = computeFullVelocity(rfftValues, testSampleCount, testSamplingRate,
//                                        5, 1000, 9806.5 * 4. / 32768.);
//
//    assertEqual(round(velRMS1 * 1000), 3497); // Python 3378
//    assertEqual(round(velRMS2 * 1000), 4186); // Python 4052
//    assertEqual(round(velRMS3 * 1000), 4160); // Python 4044
}


#endif // IUTESTUTILITIES_H
