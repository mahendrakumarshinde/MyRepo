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

test(siganl_energy_power_RMS)
{
  int sampleCount = 512;
  int samplingRate = 1000;
  q15_t dataCopy[512];
  /*
  q15_t data[512] =
    {
      2608, 2592, 2544, 2480, 2364, 2224, 2108, 1968, 1808, 1672, 1560, 1472, 1420, 1388, 1416,
      1444, 1552, 1672, 1760, 1924, 2072, 2172, 2332, 2456, 2532, 2528, 2536, 2472, 2404, 2328,
      2184, 2064, 1932, 1760, 1612, 1536, 1448, 1380, 1352, 1368, 1420, 1532, 1640, 1768, 1920,
      2056, 2208, 2316, 2416, 2480, 2520, 2560, 2528, 2484, 2316, 2188, 2056, 1924, 1772, 1668,
      1540, 1468, 1388, 1396, 1416, 1464, 1552, 1676, 1800, 1928, 2104, 2228, 2324, 2432, 2520,
      2560, 2588, 2524, 2480, 2392, 2272, 2152, 1996, 1756, 1652, 1528, 1436, 1416, 1408, 1404,
      1484, 1576, 1664, 1796, 1936, 2092, 2240, 2356, 2504, 2544, 2548, 2548, 2524, 2448, 2372,
      2280, 2124, 2004, 1848, 1680, 1588, 1456, 1424, 1420, 1436, 1476, 1572, 1676, 1828, 1948,
      2120, 2252, 2360, 2440, 2512, 2532, 2544, 2520, 2420, 2328, 2216, 2064, 1932, 1796, 1664,
      1528, 1456, 1404, 1344, 1392, 1452, 1540, 1664, 1800, 1936, 2080, 2208, 2372, 2484, 2512,
      2568, 2588, 2520, 2452, 2352, 2236, 2088, 1992, 1800, 1668, 1556, 1464, 1416, 1388, 1388,
      1448, 1520, 1640, 1824, 1992, 2116, 2260, 2384, 2480, 2544, 2552, 2552, 2520, 2428, 2368,
      2232, 2096, 1952, 1808, 1684, 1560, 1472, 1440, 1404, 1440, 1460, 1544, 1656, 1764, 1924,
      2108, 2276, 2396, 2476, 2540, 2556, 2560, 2516, 2452, 2348, 2216, 2068, 1912, 1780, 1668,
      1564, 1472, 1412, 1400, 1416, 1492, 1548, 1672, 1792, 1956, 2096, 2220, 2344, 2428, 2548,
      2584, 2520, 2488, 2408, 2280, 2164, 2040, 1904, 1736, 1612, 1504, 1400, 1360, 1364, 1380,
      1416, 1528, 1648, 1768, 1924, 2068, 2216, 2352, 2448, 2532, 2520, 2556, 2504, 2408, 2316,
      2208, 2068, 1900, 1780, 1620, 1516, 1460, 1416, 1388, 1404, 1472, 1564, 1680, 1828, 1964,
      2132, 2256, 2356, 2428, 2504, 2528, 2544, 2540, 2492, 2352, 2252, 2116, 1884, 1748, 1640,
      1528, 1456, 1404, 1388, 1416, 1484, 1576, 1704, 1824, 1956, 2120, 2244, 2356, 2472, 2544,
      2600, 2572, 2524, 2492, 2336, 2252, 2140, 1980, 1840, 1660, 1504, 1444, 1404, 1404, 1444,
      1472, 1600, 1716, 1832, 1984, 2132, 2252, 2388, 2472, 2512, 2540, 2516, 2488, 2404, 2324,
      2200, 2072, 1924, 1760, 1628, 1512, 1432, 1376, 1352, 1404, 1464, 1568, 1676, 1820, 1960,
      2108, 2256, 2372, 2452, 2492, 2552, 2552, 2508, 2440, 2348, 2212, 2080, 1924, 1780, 1668,
      1520, 1476, 1412, 1388, 1420, 1468, 1556, 1716, 1872, 2000, 2144, 2284, 2404, 2496, 2552,
      2580, 2560, 2504, 2448, 2336, 2208, 2100, 1944, 1764, 1656, 1536, 1452, 1408, 1408, 1428,
      1476, 1572, 1668, 1808, 1936, 2164, 2284, 2424, 2496, 2548, 2576, 2552, 2488, 2432, 2348,
      2220, 2048, 1936, 1772, 1632, 1552, 1456, 1420, 1396, 1444, 1516, 1584, 1704, 1808, 1936,
      2116, 2264, 2336, 2460, 2528, 2532, 2536, 2492, 2404, 2288, 2160, 2008, 1856, 1716, 1600,
      1476, 1412, 1380, 1360, 1372, 1452, 1556, 1636, 1796, 1936, 2088, 2252, 2336, 2484, 2524,
      2584, 2568, 2496, 2424, 2300, 2184, 2056, 1888, 1732, 1632, 1504, 1420, 1388, 1384, 1424,
      1488, 1564, 1700, 1828, 1956, 2100, 2276, 2356, 2472, 2524, 2540, 2552, 2484, 2440, 2368,
      2244, 2028, 1884, 1752, 1628, 1536, 1444, 1408, 1416, 1452, 1516, 1596, 1708, 1860, 1996,
      2132, 2272, 2372, 2476, 2528, 2544, 2536, 2500, 2440, 2352, 2224, 2084, 1916, 1796, 1612,
      1504, 1428
    };
    */

  q15_t data[512] =
    {
      1692, 1668, 1676, 1684, 1700, 1704, 1700, 1724, 1724, 1748,
    1764, 1796, 1800, 1856, 1864, 1884, 1920, 1948, 1996, 2020,
    2032, 2056, 2080, 2132, 2160, 2164, 2192, 2216, 2220, 2228,
    2232, 2268, 2276, 2276, 2272, 2260, 2264, 2244, 2248, 2196,
    2200, 2200, 2172, 2164, 2140, 2100, 2068, 2044, 2028, 2004,
    1964, 1916, 1924, 1880, 1860, 1816, 1788, 1756, 1780, 1744,
    1732, 1728, 1696, 1724, 1712, 1700, 1716, 1700, 1696, 1728,
    1732, 1756, 1760, 1772, 1784, 1800, 1792, 1824, 1876, 1884,
    1924, 1936, 1980, 2004, 2024, 2068, 2076, 2080, 2124, 2160,
    2192, 2208, 2212, 2220, 2208, 2252, 2232, 2224, 2244, 2244,
    2228, 2212, 2208, 2204, 2192, 2184, 2140, 2128, 2092, 2060,
    2044, 2048, 2028, 1960, 1920, 1896, 1872, 1852, 1816, 1804,
    1796, 1748, 1744, 1724, 1736, 1716, 1712, 1696, 1716, 1696,
    1696, 1716, 1712, 1732, 1736, 1760, 1792, 1804, 1840, 1824,
    1888, 1908, 1928, 1968, 1984, 2012, 2056, 2072, 2120, 2136,
    2144, 2192, 2224, 2244, 2228, 2252, 2260, 2272, 2268, 2268,
    2280, 2252, 2260, 2252, 2256, 2232, 2232, 2212, 2176, 2156,
    2120, 2100, 2072, 2012, 1976, 1948, 1924, 1904, 1860, 1820,
    1816, 1776, 1764, 1724, 1708, 1704, 1700, 1680, 1684, 1692,
    1680, 1672, 1668, 1680, 1692, 1696, 1708, 1740, 1752, 1760,
    1804, 1800, 1820, 1856, 1864, 1944, 1964, 1996, 2036, 2064,
    2076, 2100, 2108, 2156, 2168, 2184, 2184, 2232, 2224, 2244,
    2252, 2252, 2292, 2276, 2264, 2264, 2264, 2220, 2220, 2220,
    2192, 2176, 2148, 2116, 2092, 2024, 2020, 2004, 1968, 1944,
    1920, 1892, 1852, 1852, 1792, 1816, 1784, 1764, 1740, 1732,
    1724, 1704, 1704, 1696, 1708, 1724, 1676, 1720, 1716, 1732,
    1744, 1784, 1816, 1832, 1856, 1864, 1908, 1920, 1952, 1988,
    1996, 2044, 2052, 2036, 2076, 2108, 2128, 2164, 2176, 2180,
    2200, 2216, 2212, 2224, 2236, 2240, 2252, 2236, 2260, 2228,
    2216, 2180, 2176, 2148, 2136, 2096, 2076, 2056, 2032, 2024,
    1984, 1956, 1936, 1888, 1880, 1856, 1840, 1808, 1808, 1788,
    1736, 1712, 1708, 1688, 1692, 1704, 1704, 1672, 1700, 1700,
    1716, 1740, 1748, 1772, 1776, 1812, 1816, 1848, 1864, 1900,
    1916, 1956, 1972, 1984, 2024, 2052, 2080, 2120, 2144, 2160,
    2196, 2200, 2228, 2252, 2276, 2252, 2268, 2248, 2268, 2288,
    2268, 2260, 2256, 2224, 2216, 2196, 2176, 2164, 2148, 2096,
    2092, 2048, 2032, 1988, 1992, 1936, 1920, 1888, 1852, 1828,
    1788, 1776, 1740, 1732, 1712, 1660, 1680, 1660, 1660, 1644,
    1668, 1656, 1664, 1676, 1688, 1700, 1728, 1764, 1756, 1804,
    1816, 1856, 1860, 1916, 1944, 1936, 1988, 2028, 2024, 2072,
    2124, 2140, 2148, 2188, 2204, 2236, 2248, 2260, 2272, 2264,
    2264, 2268, 2264, 2256, 2232, 2216, 2232, 2216, 2188, 2168,
    2136, 2144, 2108, 2044, 2056, 2020, 1992, 1976, 1964, 1908,
    1892, 1848, 1808, 1792, 1772, 1740, 1716, 1712, 1692, 1708,
    1696, 1676, 1716, 1700, 1700, 1716, 1732, 1736, 1760, 1760,
    1784, 1780, 1824, 1848, 1872, 1908, 1916, 1944, 1960, 2016,
    2076, 2084, 2120, 2164, 2164, 2184, 2208, 2236, 2248, 2236,
    2264, 2248, 2248, 2224, 2232, 2236, 2232, 2220, 2192, 2176,
    2156, 2148, 2128, 2140, 2108, 2084, 2060, 2000, 1972, 1944,
    1936, 1892, 1876, 1828, 1816, 1772, 1764, 1736, 1740, 1712,
    1740, 1688, 1704, 1716, 1680, 1680, 1696, 1684, 1692, 1712,
    1728, 1748
    };

  float energy1 = computeSignalEnergy(data, sampleCount, samplingRate, 1, false);
  float energy2 = computeSignalEnergy(data, sampleCount, samplingRate, 10, false);
  float energy3 = computeSignalEnergy(data, sampleCount, samplingRate, 1, true);
  float power1 = computeSignalPower(data, sampleCount, samplingRate, 1, false);
  float power2 = computeSignalPower(data, sampleCount, samplingRate, 10, false);
  float power3 = computeSignalPower(data, sampleCount, samplingRate, 1, true);
  float rms1 = computeSignalRMS(data, sampleCount, samplingRate, 1, false);
  float rms2 = computeSignalRMS(data, sampleCount, samplingRate, 10, false);
  float rms3 = computeSignalRMS(data, sampleCount, samplingRate, 1, true);

  Serial.println("energy");
  Serial.println(energy1);
  Serial.println(energy2);
  Serial.println(energy3);
  Serial.println("Power");
  Serial.println(power1);
  Serial.println(power2);
  Serial.println(power3);
  Serial.println("RMS");
  Serial.println(rms1);
  Serial.println(rms2);
  Serial.println(rms3);

  /*
  assertEqual(round(energy3), 88695);
  assertEqual(round(energy1), 2093086);
  assertEqual(round(energy2), 209309168);
  assertEqual(round(power1), 4088059);
  assertEqual(round(power2), 408806944);
  assertEqual(round(power3), 173233);
  assertEqual(round(rms1 * 100), 202189); // Round to 2 decimal place
  assertEqual(round(rms2 * 100), 2021898); // Round to 2 decimal place
  assertEqual(round(rms3 * 100), 41621); // Round to 2 decimal place
  */

  q15_t rfftValues[1024];

  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  /*
  assertEqual(rfftValues[0], 1976);
  assertEqual(rfftValues[1], 0);
  */

  rfftValues[0] = 0;
  rfftValues[1] = 0;
  float mainFreq = getMainFrequency(rfftValues, sampleCount, samplingRate);
  Serial.print("Main Freq"); Serial.println(mainFreq);
  assertEqual(round(mainFreq * 100), 4102); // Around 40 Hz
  assertEqual(rfftValues[42], -30);  // in Python -28.158913732246681
  assertEqual(rfftValues[43], -185);    // in Python -184.8373755644219

  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  filterAndIntegrateFFT(rfftValues, sampleCount, samplingRate, 0, 1000, 512, false);
  assertEqual(rfftValues[0], 0);
  assertEqual(rfftValues[2], 0);
  assertEqual(rfftValues[3], 42);
  assertEqual(rfftValues[42], -368); // Python -367.22
  assertEqual(rfftValues[43], 60); // Python 55.94

  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  filterAndIntegrateFFT(rfftValues, sampleCount, samplingRate, 5, 950, 512, false);
  assertEqual(rfftValues[0], 0);
  assertEqual(rfftValues[1], 0);
  assertEqual(rfftValues[2], 0);
  assertEqual(rfftValues[3], 0);
  assertEqual(rfftValues[4], -21);
  assertEqual(rfftValues[5], 42);
  assertEqual(rfftValues[40], 384); // Python 384.84
  assertEqual(rfftValues[41], -65); // Python 66.59
  assertEqual(rfftValues[1022], 0);
  assertEqual(rfftValues[1023], 0);

  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  float velRMS1 = computeFullVelocity(rfftValues, sampleCount, samplingRate, 0, 1000, 1);

  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  float velRMS2 = computeFullVelocity(rfftValues, sampleCount, samplingRate, 0, 1000, 9810. / 2048.);

  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  float velRMS3 = computeFullVelocity(rfftValues, sampleCount, samplingRate, 5, 1000, 1);

  copyArray(data, dataCopy, sampleCount);
  computeRFFT(dataCopy, rfftValues, sampleCount, false);
  float velRMS4 = computeFullVelocity(rfftValues, sampleCount, samplingRate, 5, 1000, 9810. / 2048.);
  assertEqual(round(velRMS1 * 1000), 2404); // Python 1709
  assertEqual(round(velRMS2 * 1000), 11517); // Python 8191
  assertEqual(round(velRMS3 * 1000), 2368); // Python 1698
  assertEqual(round(velRMS4 * 1000), 11345); // Python 8136
}


#endif // IUTESTUTILITIES_H
