#ifndef TEST_IUUTILITIES_H
#define TEST_IUUTILITIES_H

#include <ArduinoUnit.h>
#include "../IUUtilities.h"
#include "../IUFeature.h"

test(splitString)
{
  uint8_t destSize = 3;
  String dest[3];
  uint8_t foundCount = splitString("ABC-D-EFGH", '-', dest, destSize);
  assertEqual(foundCount, 3);
  assertEqual(dest[0], "ABC");
  assertEqual(dest[1], "D");
  assertEqual(dest[2], "EFGH");
}

test(splitStringToInt)
{
  uint8_t destSize = 3;
  int dest[3];
  uint8_t foundCount = splitStringToInt("12:0.12", ':', dest, destSize);
  assertEqual(foundCount, 2);
  assertEqual(dest[0], 12);
  assertEqual(dest[1], 0);
}

test(checkCharsAtPosition)
{
  //TODO implement
}

test(strCopyWithAutocompletion)
{
  char dest[12];
  uint8_t destLen = 12;
  char src[8] = "abcdefg";
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

test(accelAndVelocityComputation)
{
  int sampleCount = 512;
  int samplingRate = 1000;
  q15_t dataCopy[512];

  q15_t data[512] =
    {
      8576, 9536, 10128, 10176, 9568, 8736, 7632, 6544, 5872, 5776, 5952, 6864, 7840, 8960, 10016,
      10240, 9824, 8992, 7856, 6800, 5968, 5648, 5760, 6496, 7552, 8544, 9472, 10016, 10000, 9472,
      8496, 7392, 6352, 5680, 5504, 5840, 6784, 7872, 8832, 9712, 10112, 9680, 8720, 7776, 6688,
      5824, 5552, 5792, 6464, 7648, 8752, 9648, 10080, 10000, 9568, 8560, 7472, 6416, 5824, 5584,
      6064, 6784, 7968, 9040, 9808, 10176, 10016, 9280, 8224, 6624, 5936, 5568, 5936, 6672, 7680,
      8880, 9680, 10160, 10112, 9456, 8528, 7440, 6384, 5776, 5600, 6096, 6976, 8112, 9072, 9936,
      10192, 9984, 9216, 8256, 7152, 6080, 5648, 6000, 6704, 7856, 8944, 9792, 10176, 10096, 9408,
      8400, 7376, 6416, 5824, 5728, 6208, 7056, 8096, 9280, 9840, 10208, 9920, 9168, 8080, 7040,
      6064, 5680, 5648, 6368, 7792, 8944, 9744, 10064, 9856, 9232, 8192, 7104, 6160, 5568, 5536,
      6032, 6960, 8176, 9152, 9872, 10144, 9840, 9056, 7920, 6880, 5984, 5616, 5712, 6320, 7344,
      8464, 9696, 10160, 9952, 9280, 8256, 7168, 6160, 5728, 5696, 6240, 7168, 8224, 9296, 10016,
      10208, 9808, 9088, 7936, 6912, 6032, 5648, 5760, 6400, 7568, 8576, 9568, 10224, 10144, 9232,
      8272, 7168, 6240, 5664, 5680, 6272, 7184, 8384, 9440, 10016, 10240, 9872, 9024, 7904, 6912,
      6032, 5648, 5824, 6560, 7600, 8720, 9616, 10112, 10192, 9680, 8688, 7088, 6208, 5712, 5792,
      6336, 7312, 8448, 9488, 10048, 10208, 9808, 8896, 7856, 6672, 5904, 5600, 5712, 6528, 7440,
      8624, 9504, 9968, 9920, 9312, 8400, 7376, 6304, 5680, 5648, 6224, 7248, 8320, 9296, 9952,
      10080, 9632, 8784, 7712, 6624, 5968, 5568, 5984, 6560, 7648, 8800, 9712, 10064, 10096, 9504,
      8384, 7408, 6432, 5728, 5664, 6064, 7456, 8576, 9568, 10096, 10112, 9632, 8768, 7696, 6672,
      5920, 5680, 5984, 6752, 7728, 8832, 9808, 10208, 9920, 9520, 8512, 7392, 6432, 5696, 5696,
      6096, 6960, 8032, 9184, 10144, 10144, 9632, 8672, 7584, 6624, 5920, 5680, 6048, 6800, 7808,
      8992, 9840, 10176, 10032, 9408, 8496, 7344, 6432, 5696, 5712, 6224, 7024, 8080, 9264, 10016,
      10128, 9904, 8576, 7504, 6496, 5840, 5584, 5920, 6688, 7808, 8880, 9728, 10080, 9824, 9216,
      8112, 7008, 6064, 5520, 5568, 6160, 7024, 8080, 9232, 9888, 10080, 9776, 8960, 7920, 6432,
      5808, 5680, 6032, 6896, 7952, 9072, 9920, 10112, 9904, 9264, 8256, 7152, 6256, 5776, 5776,
      6304, 7232, 8272, 9264, 9904, 10160, 9888, 9024, 7936, 6848, 6112, 5696, 6144, 6944, 8112,
      9088, 9936, 10224, 10016, 9248, 8224, 7056, 6192, 5744, 5792, 6384, 7280, 8432, 9328, 10128,
      10208, 9760, 8896, 7952, 6736, 6112, 5648, 5808, 6624, 7680, 9248, 9952, 10208, 9936, 9120,
      8016, 7088, 6096, 5776, 5792, 6400, 7408, 8480, 9392, 10080, 10160, 9648, 8816, 7712, 6624,
      5936, 5632, 5808, 6496, 7536, 8656, 9472, 10128, 9776, 8960, 7872, 6768, 6016, 5552, 5680,
      6416, 7392, 8432, 9424, 10048, 10016, 9648, 8832, 7600, 6608, 5792, 5568, 5840, 6704, 7776,
      8832, 9696, 10176, 10016, 9456, 8000, 6800, 6032, 5632, 5856, 6496, 7568, 8608, 9504, 10064,
      10208, 9600, 8736, 7632, 6608, 5856, 5632, 5904, 6816, 7888, 8864, 9760, 10112, 10096, 9392,
      8400, 7344, 5920, 5680, 5856, 6496, 7680, 8720, 9632, 10096, 10192, 9632, 8704, 7584, 6560,
      5888, 5680, 6048, 6800, 7904, 9040, 9776, 10160, 10048, 9376, 8320, 7264, 6368, 5808, 5728,
      6656, 7696
    };

  float energy1 = computeSignalEnergy(data, sampleCount, samplingRate, 1, false);
  float energy2 = computeSignalEnergy(data, sampleCount, samplingRate, 2, false);
  float energy3 = computeSignalEnergy(data, sampleCount, samplingRate, 1, true);
  assertEqual(round(energy1), 33258676);
  assertEqual(round(energy2), 133034704);
  assertEqual(round(energy3), 1325821);

  float power1 = computeSignalPower(data, sampleCount, samplingRate, 1, false);
  float power2 = computeSignalPower(data, sampleCount, samplingRate, 2, false);
  float power3 = computeSignalPower(data, sampleCount, samplingRate, 1, true);
  assertEqual(round(power1), 64958348);
  assertEqual(round(power2), 259833392);
  assertEqual(round(power3), 2589493);

  float rms1 = computeSignalRMS(data, sampleCount, samplingRate, 1, false);
  float rms2 = computeSignalRMS(data, sampleCount, samplingRate, 10, false);
  float rms3 = computeSignalRMS(data, sampleCount, samplingRate, 1, true);
  assertEqual(round(rms1 * 100), 805967); // Round to 2 decimal place
  assertEqual(round(rms2 * 100), 8059685); // Round to 2 decimal place
  assertEqual(round(rms3 * 100), 160919); // Round to 2 decimal place

  q15_t rfftValues[1024];

  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  assertEqual(rfftValues[0], 7895);   // found 7897 in Python
  assertEqual(rfftValues[1], 0);      // found 0 in Python
  assertEqual(rfftValues[2], -1);      // found -0.9 in Python
  assertEqual(rfftValues[3], 1);      // found 1.6 in Python
  assertEqual(rfftValues[82], 329);    // found 329.9 in Python
  assertEqual(rfftValues[83], -1085);  //found -1084.8 in Python

  // Inverse fft
  computeRFFT(rfftValues, dataCopy, sampleCount, true);
  float orgRMS = computeSignalRMS(data, sampleCount, samplingRate, 1, false);
  float newRMS = computeSignalRMS(dataCopy, sampleCount, samplingRate, 1, false) * 512;
  // Illustrate loss of precision
  assertEqual(round(orgRMS), 8060);
  assertEqual(round(newRMS), 7381);

  float mainFreq = getMainFrequency(rfftValues, sampleCount, samplingRate);
  assertEqual(round(mainFreq * 100), 8008); // Around 80 Hz

  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  filterAndIntegrateFFT(rfftValues, sampleCount, samplingRate, 0, 1000, 512, false);
  assertEqual(rfftValues[0], 0);
  assertEqual(rfftValues[1], 0);
  assertEqual(rfftValues[2], 42);
  assertEqual(rfftValues[3], 42);
  assertEqual(rfftValues[4], 0);
  assertEqual(rfftValues[5], 42);
  assertEqual(rfftValues[82], -1104);
  assertEqual(rfftValues[83], -335);


  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  filterAndIntegrateFFT(rfftValues, sampleCount, samplingRate, 5, 1000, 512, false);
  assertEqual(rfftValues[0], 0);
  assertEqual(rfftValues[1], 0);
  assertEqual(rfftValues[2], 0);
  assertEqual(rfftValues[3], 0);
  assertEqual(rfftValues[4], 0);
  assertEqual(rfftValues[5], 42);
  assertEqual(rfftValues[82], -1104);
  assertEqual(rfftValues[83], -335);

  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  float velRMS1 = computeFullVelocity(rfftValues, sampleCount, samplingRate, 0, 1000, 1);

  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  float velRMS2 = computeFullVelocity(rfftValues, sampleCount, samplingRate, 0, 1000, 9806.5 * 4. / 32768.);

  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  float velRMS3 = computeFullVelocity(rfftValues, sampleCount, samplingRate, 5, 1000, 9806.5 * 4. / 32768.);

  assertEqual(round(velRMS1 * 1000), 3497); // Python 3378
  assertEqual(round(velRMS2 * 1000), 4186); // Python 4052
  assertEqual(round(velRMS3 * 1000), 4160); // Python 4044
}


#endif // IUTESTUTILITIES_H
