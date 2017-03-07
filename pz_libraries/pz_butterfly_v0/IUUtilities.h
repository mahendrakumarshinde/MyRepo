#ifndef IUUTILITIES_H
#define IUUTILITIES_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>


/* ============================= User defined Enums ============================= */
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


/* ============================= RFFT and CFFT instances to use for all FFTs ============================= */
arm_rfft_instance_q15 rfftInstance;
//arm_cfft_radix4_instance_q15 cfftInstance;


/* ============================= Single Source Feature Computation functions ============================= */
float computeRMS(uint16_t arrSize, q15_t *arrValues);


/* ============================= Multi Source Feature Computation functions ============================= */
float computeSignalEnergy(uint8_t sourceCount, uint16_t* sourceSize, q15_t* source);


#endif // IUUTILITIES_H
