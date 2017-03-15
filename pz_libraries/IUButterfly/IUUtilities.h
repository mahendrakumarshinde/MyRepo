#ifndef IUUTILITIES_H
#define IUUTILITIES_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>


/* ============================= Operation Enums ============================= */
// Operation mode enum
enum operationMode {run              = 0,
                    charging         = 1,
                    dataCollection   = 2,
                    configuration    = 3,
                    modeCount        = 4}; // The number of different operation modes

// Operation state enum
enum operationState : uint8_t {notCutting     = 0,
                               normalCutting  = 1,
                               warningCutting = 2,
                               badCutting     = 3,
                               stateCount     = 4}; // The number of different operation states

struct operation_t {
  operationMode mode;
  operationState state;
};

/* ============================= RFFT and CFFT instances to use for all FFTs ============================= */
arm_rfft_instance_q15 rfftInstance;
//arm_cfft_radix4_instance_q15 cfftInstance;

/*====================== General utility function ============================ */
float q15ToFloat(q15_t value);
q15_t floatToQ15(float value);
float g_to_ms2(float value) { return 9.8 * value; }
q15_t g_to_ms2(q15_t value) { return 9.8 * value; }
float ms2_to_g(float value) { return value / 9.8; }
q15_t ms2_to_g(q15_t value) { return value / 9.8; }
float computeRMS(uint16_t sourceSize, q15_t *source);


/* ============================= Feature Computation functions ============================= */
// Default compute functions
float computeDefaultQ15(uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[]) { return q15ToFloat(source[0][0]); }
float computeDefaultFloat(uint8_t sourceCount, const uint16_t *sourceSize, float *source[]) { return (source[0][0]); }
float computeDefaultDataCollectionQ15 (uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[], float *m_destination[]);
float computeSignalEnergy(uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[]);
float computeSumOf(uint8_t sourceCount, const uint16_t* sourceSize, float* source[]);
float computeRFFTMaxIndex(uint8_t sourceCount, const uint16_t* sourceSize, q15_t* source[]);
float computeAcousticDB(uint8_t sourceCount, const uint16_t* sourceSize, q15_t* source[]);
float computeVelocity(uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[]);


#endif // IUUTILITIES_H
