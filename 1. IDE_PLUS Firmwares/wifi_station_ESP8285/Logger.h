#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

/* =============================================================================
    Debugging
============================================================================= */

// Define DEBUGMODE to enable debug level messages from the whole firmware
//#define DEBUGMODE

#ifdef DEBUGMODE
    const bool debugMode = true;
    /***** Test connection *****/
//    const char testSSID[14] = "The Port WiFi";
//    const char testPSK[11] = "Th3P0rt317";
    const char testSSID[19] = "AndroidHotspot6994";
    const char testPSK[13] = "f1b94630f970";
#else
    const bool debugMode = false;
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
inline void raiseException(T msg, bool endline = true)
{
    #ifdef DEBUGMODE
    debugPrint(F("Error: "), false);
    debugPrint(msg, endline);
    #endif
}

#endif // LOGGER_H
