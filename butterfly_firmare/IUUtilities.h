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
                    record           = 4,
                    sleep            = 5,
                    modeCount        = 6}; // The number of different operation modes

// Operation state enum
enum operationState {notCutting     = 0,
                     normalCutting  = 1,
                     warningCutting = 2,
                     badCutting     = 3,
                     stateCount     = 4}; // The number of different operation states

/* ============================= RFFT and CFFT instances to use for all FFTs ============================= */
//arm_rfft_instance_q15 rfftInstance;
//arm_cfft_radix4_instance_q15 cfftInstance;

/*====================== General utility function ============================ */

// String processing
uint8_t splitString(String str, const char separator, String destination[], uint8_t destinationSize);

uint8_t splitStringToInt(String str, const char separator, int destination[], const uint8_t destinationSize);

bool checkCharsAtPosition(char *charBuffer, int *positions, char character);

// Conversion
float q15ToFloat(q15_t value);

q15_t floatToQ15(float value);

inline float g_to_ms2(float value) { return 9.8 * value; }

inline q15_t g_to_ms2(q15_t value) { return 9.8 * value; }

inline float ms2_to_g(float value) { return value / 9.8; }

inline q15_t ms2_to_g(q15_t value) { return value / 9.8; }

float computeRMS(uint16_t sourceSize, q15_t *source);


/* ============================= Feature Computation functions ============================= */
// Default compute functions
inline float computeDefaultQ15(uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[]) { return q15ToFloat(source[0][0]); }

inline float computeDefaultFloat(uint8_t sourceCount, const uint16_t *sourceSize, float *source[]) { return (source[0][0]); }

inline float computeDefaultDataCollectionQ15 (uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[], float *m_destination[]);

float computeSignalEnergy(uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[]);

float computeSumOf(uint8_t sourceCount, const uint16_t* sourceSize, float* source[]);

float computeRFFTMaxIndex(uint8_t sourceCount, const uint16_t* sourceSize, q15_t* source[]);

float computeVelocity(uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[]);

float computeAcousticDB(uint8_t sourceCount, const uint16_t* sourceSize, q15_t* source[]);

float computeAudioRFFT(uint8_t sourceCount, const uint16_t* sourceSize, q15_t* source[]);


//==============================================================================
//========================== Hamming Window Presets ============================
//==============================================================================
// Hamming window definition: w(n) = 0.54 - 0.46 * cos(2 * Pi * n / N), 0 <= n <= N
// Since we are using q15 values, we then have to do round(w(n) * 2^15) to convert from float to q15 format.

const int magsize_512 = 257;
const float hamming_K_512 = 1.8519;         // 1/0.5400
const float inverse_wlen_512 = 1/512.0;
extern q15_t hamming_window_512 [512];

const int magsize_2048 = 1025;
const float hamming_K_2048 = 1.8519;        // 1/0.5400
const float inverse_wlen_2048 = 1/2048.0;
extern q15_t hamming_window_2048 [2048];


#endif // IUUTILITIES_H
