#ifndef TEST_IUUTILITIES_H
#define TEST_IUUTILITIES_H

#include <ArduinoUnit.h>
#include "../IUUtilities.h"

test(splitString)
{
  uint8_t destsize = 3;
  String dest[3];
  uint8_t foundCount = splitString("ABC-D-EFGH", '-', dest, destSize);
  assertEqual(foundCount, 3);
  assertEqual(dest[0], "ABC");
  assertEqual(dest[1], "D");
  assertEqual(dest[2], "EFGH");
}

test(splitStringToInt)
{
  uint8_t destsize = 3;
  int dest[3];
  uint8_t foundCount = splitStringToInt("12:0.12", ':', dest, destSize);
  assertEqual(foundCount, 2);
  assertEqual(dest[0], 12);
  assertEqual(dest[0], 0);
}

test(checkCharsAtPosition)
{
  //TODO implement
}

test(strCopyWithAutocompletion)
{
  char dest[12];
  uint8_t destLen = 12;
  char src[8] = "abcdefg"
  uint8_t srcLen = 8;
  strCopyWithAutocompletion(dest, src, destLen, srcLen);
  assertEqual(src, dest);
  for (uint8_t i = 8; i < 12; ++i)
  {
    assertEqual(dest[i], 0);
  }
}

test(getMax)
{
  q15_t a1[5] = {1, 2, -3, -2, 1};
  assertEqual(getMax(a1, 5), 2);
}

test(computeRMS)
{
  float computeRMS(uint16_t sourceSize, q15_t *source, float (*transform)(q15_t));
}

test(multiArrayMean)
{
  float multiArrayMean(uint8_t arrCount, const uint16_t* arrSizes, float **arrays);
}

test(computeRFFT)
{
  bool computeRFFT(q15_t *source, q15_t *destination, const uint16_t FFTlength, bool inverse);
}

test(computeRFFT)
{
  bool computeRFFT(q15_t *source, q15_t *destination, const uint16_t FFTlength, bool inverse, q15_t *window, float windowGain);
}

test(filterAndIntegrateFFT)
{
  void filterAndIntegrateFFT(q15_t *values, uint16_t sampleCount, uint16_t samplingRate, uint16_t FreqLowerBound, uint16_t FreqHigherBound, uint16_t rescale, bool twice = false);
}


#endif // IUTESTUTILITIES_H
