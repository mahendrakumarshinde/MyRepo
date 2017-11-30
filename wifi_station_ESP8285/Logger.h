#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

/* ============================= Debugging ============================= */

// Define DEBUGMODE to enable special notifications from the whole firmware
#define DEBUGMODE

#ifdef DEBUGMODE
    const bool debugMode = false;
#else
    const bool debugMode = true;
#endif



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

#endif // LOGGER_H
