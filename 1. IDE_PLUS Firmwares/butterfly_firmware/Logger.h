#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <MemoryFree.h>

/* ============================= Debugging ============================= */

// Define DEBUGMODE to enable special notifications from the whole firmware
#define DEBUGMODE

#ifdef DEBUGMODE
    const bool setupDebugMode = false;
    const bool loopDebugMode = false;
    const bool featureDebugMode = false;
    const bool highVerbosity = false;
    const bool callbackDebugMode = false;
#else
    const bool setupDebugMode = false;
    const bool loopDebugMode = false;
    const bool featureDebugMode = false;
    const bool highVerbosity = false;
    const bool callbackDebugMode = false;
#endif

const bool debugMode = (setupDebugMode || loopDebugMode ||
    featureDebugMode || callbackDebugMode);

template <typename T>
inline void debugPrint(T msg, bool endline = true)
{
    #ifdef DEBUGMODE
    if (endline) { Serial.println(msg); }
    else { Serial.print(msg); }
    #endif
}

//Specialization for float printing with fixed decimal count
template <>
inline void debugPrint(float msg, bool endline)
{
    #ifdef DEBUGMODE
    if (endline) { Serial.println(msg, 6); }
    else { Serial.print(msg, 6); }
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
