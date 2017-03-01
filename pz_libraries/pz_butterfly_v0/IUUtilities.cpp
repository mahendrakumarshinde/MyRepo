#include "IUUtilities.h"

/**
 * Compute and return the normalized root min square
 * @param arrSize the length of the array
 * @param a q15_t array (elements are int16_t0
 */
float computeRMS(uint16_t arrSize, q15_t *arrValues)
{
  float avg(0), avg2(0);
  for (int i = 0; i < arrSize; i++)
  {
    avg += (float)arrValues[i];
    avg2 += sq((float)arrValues[i]);
  }
  return sqrt(avg2 / arrSize - sq(avg / arrSize));
}



