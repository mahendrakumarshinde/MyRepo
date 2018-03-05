#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

/* =============================================================================
    Debugging
============================================================================= */

// Define DEBUGMODE to enable debug level messages from the whole firmware
#define DEBUGMODE

#ifdef DEBUGMODE
    const bool setupDebugMode = false;
    const bool loopDebugMode = true;
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
inline void raiseException(T msg, bool endline = true)
{
    #ifdef DEBUGMODE
    debugPrint(F("Error: "), false);
    debugPrint(msg, endline);
    #endif
}

#endif // LOGGER_H
