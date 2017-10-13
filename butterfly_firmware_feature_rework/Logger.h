#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <ArduinoUnit.h>
#include <MemoryFree.h>

/* ============================= Debugging ============================= */

// Define DEBUGMODE to enable special notifications from the whole firmware
#define DEBUGMODE

#ifdef DEBUGMODE
__attribute__((section(".noinit2"))) const bool setupDebugMode = false;
__attribute__((section(".noinit2"))) const bool loopDebugMode = false;
__attribute__((section(".noinit2"))) const bool callbackDebugMode = false;
__attribute__((section(".noinit2"))) const bool highVerbosity = false;
__attribute__((section(".noinit2"))) const bool readableDataCollection = false;
#else
__attribute__((section(".noinit2"))) const bool setupDebugMode = false;
__attribute__((section(".noinit2"))) const bool loopDebugMode = false;
__attribute__((section(".noinit2"))) const bool callbackDebugMode = false;
__attribute__((section(".noinit2"))) const bool highVerbosity = false;
__attribute__((section(".noinit2"))) const bool readableDataCollection = false;
#endif

__attribute__((section(".noinit2"))) const bool debugMode =
    (setupDebugMode || loopDebugMode || callbackDebugMode);

template <typename T>
inline void debugPrint(T msg, bool endline = true)
{
    #ifdef DEBUGMODE
    if (endline) { Serial.println(msg); }
    else { Serial.print(msg); }
    Serial.flush();
    #endif
}

//Specialization for float printing with fixed decimal count
template <>
inline void debugPrint(float msg, bool endline)
{
    #ifdef DEBUGMODE
    if (endline) { Serial.println(msg, 6); }
    else { Serial.print(msg, 6); }
    Serial.flush();
    #endif
}

template <typename T>
inline void raiseException(T msg)
{
    #ifdef DEBUGMODE
    debugPrint(F("Error: "), false);
    debugPrint(msg);
    while(1) { }
    #endif
}

inline void memoryLog()
{
    #ifdef DEBUGMODE
    Serial.print(F("Available Memory: "));
    Serial.println(freeMemory(), DEC);
    Serial.flush();
    #endif
}

inline void memoryLog(String msg)
{
    #ifdef DEBUGMODE
    Serial.print(msg);
    Serial.print(F(" - "));
    memoryLog();
    #endif
}

#endif // LOGGER_H
