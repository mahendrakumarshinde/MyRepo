#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <ArduinoUnit.h>
#include <MemoryFree.h>

/* ============================= Debugging ============================= */

// Define DEBUGMODE to enable special notifications from the whole firmware
#define DEBUGMODE

#ifdef DEBUGMODE
    const bool setupDebugMode = false;
    const bool loopDebugMode = true;
    const bool highVerbosity = false;
    const bool showIntermediaryResults = true;
    const bool callbackDebugMode = false;
#else
    const bool setupDebugMode = false;
    const bool loopDebugMode = false;
    const bool highVerbosity = false;
    const bool showIntermediaryResults = false;
    const bool callbackDebugMode = false;
#endif

const bool debugMode = true;
//    (setupDebugMode || loopDebugMode || callbackDebugMode);

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
