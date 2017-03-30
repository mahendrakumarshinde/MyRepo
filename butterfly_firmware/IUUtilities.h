#ifndef IUUTILITIES_H
#define IUUTILITIES_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>
#include <MemoryFree.h>

/* ============================= Debugging ============================= */

// Define DEBUGMODE to enable special notifications from the whole firmware
#define DEBUGMODE

#ifdef DEBUGMODE
__attribute__((section(".noinit2"))) const bool setupDebugMode = false;
__attribute__((section(".noinit2"))) const bool loopDebugMode = true;
#else
__attribute__((section(".noinit2"))) const bool setupDebugMode = false;
__attribute__((section(".noinit2"))) const bool loopDebugMode = false;
#endif

__attribute__((section(".noinit2"))) const bool debugMode = (setupDebugMode || loopDebugMode);

void debugPrint(String msg, bool endline = true);
void debugPrint(int msg, bool endline = true);
void debugPrint(uint32_t msg, bool endline = true);
void debugPrint(float msg, bool endline = true);
void debugPrint(char msg, bool endline = true);
void debugPrint(char *msg, bool endline = true);


/* ============================= Operation Enums ============================= */
/**
 * Operation modes are mostly user controlled, with some automatic mode switching
 */
enum operationMode : uint8_t
{
  run              = 0,
  charging         = 1,
  dataCollection   = 2,
  configuration    = 3,
  record           = 4,
  sleep            = 5,
  opModeCount      = 6
}; // The number of different operation modes

/**
 * Operation states describe the production status, inferred from calculated features
 * and user-defined thresholds
 */
enum operationState : uint8_t {idle      = 0,
                               normalCutting   = 1,
                               warningCutting  = 2,
                               badCutting      = 3,
                               opStateCount    = 4}; // The number of different operation states

/**
 * Power modes are programmatically controlled and managed by the conductor depending
 * on the operation modes, states, and desired sampling rate
 */
enum powerMode : uint8_t {deepSuspend          = 0,
                          suspend              = 1,
                          standBy              = 2,
                          low                  = 3,
                          normal               = 4,
                          powerModeCount       = 5};


/*====================== General utility function ============================ */

// String processing
uint8_t splitString(String str, const char separator, String *destination, uint8_t destinationSize);

uint8_t splitStringToInt(String str, const char separator, int *destination, const uint8_t destinationSize);

bool checkCharsAtPosition(char *charBuffer, int *positions, char character);

// Conversion
float q15ToFloat(q15_t value);

q15_t floatToQ15(float value);

inline float g_to_ms2(float value) { return 9.8 * value; }

inline q15_t g_to_ms2(q15_t value) { return 9.8 * value; }

inline float ms2_to_g(float value) { return value / 9.8; }

inline q15_t ms2_to_g(q15_t value) { return value / 9.8; }

float computeRMS(uint16_t sourceSize, q15_t *source);



//==============================================================================
//========================== Hamming Window Presets ============================
//==============================================================================
// Hamming window definition: w(n) = 0.54 - 0.46 * cos(2 * Pi * n / N), 0 <= n <= N
// Since we are using q15 values, we then have to do round(w(n) * 2^15) to convert from float to q15 format.

__attribute__((section(".noinit2"))) const int magsize_512 = 257;
__attribute__((section(".noinit2"))) const float hamming_K_512 = 1.8519;         // 1/0.5400
__attribute__((section(".noinit2"))) const float inverse_wlen_512 = 1/512.0;
extern __attribute__((section(".noinit2"))) q15_t hamming_window_512 [512];

__attribute__((section(".noinit2"))) const int magsize_2048 = 1025;
__attribute__((section(".noinit2"))) const float hamming_K_2048 = 1.8519;        // 1/0.5400
__attribute__((section(".noinit2"))) const float inverse_wlen_2048 = 1/2048.0;
extern __attribute__((section(".noinit2"))) q15_t hamming_window_2048 [2048];


#endif // IUUTILITIES_H
