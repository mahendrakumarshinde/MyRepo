#ifndef IUUTILITIES_H
#define IUUTILITIES_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>

float computeRMS(uint16_t arrSize, q15_t *arrValues);


//============================= User defined Enums =============================
// Operation mode enum
enum OpMode {
  RUN              = 0,
  CHARGING         = 1,
  DATA_COLLECTION  = 2
};

// Operation state enum
enum OpState {
  NOT_CUTTING     = 0,
  NORMAL_CUTTING  = 1,
  WARNING_CUTTING = 2,
  BAD_CUTTING     = 3
};

#endif // IUUTILITIES_H
