#ifndef IUDEBUGGER_H
#define IUDEBUGGER_H

#include <Arduino.h>

/* =============================================================================
    Flag reading
============================================================================= */
#ifdef IUDEBUG_SETUP
    const bool setupDebugMode = (bool) IUDEBUG_SETUP;
#else
    const bool setupDebugMode = false;
#endif
#ifdef IUDEBUG_MAIN
    const bool loopDebugMode = (bool) IUDEBUG_MAIN;
#else
    const bool loopDebugMode = false;
#endif
#ifdef IUDEBUG_ASYNC
    const bool asyncDebugMode = (bool) IUDEBUG_ASYNC;
#else
    const bool asyncDebugMode = false;
#endif
#ifdef IUDEBUG_FEATURE
    const bool featureDebugMode = (bool) IUDEBUG_FEATURE;
#else
    const bool featureDebugMode = false;
#endif
#ifdef IUDEBUG_ANY
    const bool debugMode = (bool) IUDEBUG_ANY;
#else
    const bool debugMode = false;
#endif

/* =============================================================================
    Debugging
============================================================================= */

template <typename T>
inline void debugPrint(T msg, bool endline = true)
{
    #if IUDEBUG_ANY == 1
    if (Serial.availableForWrite()) {
        if (endline) {
            Serial.println(msg);
        } else {
            Serial.print(msg);
        }
    }
    #endif
}

//Specialization for float printing with fixed decimal count
template <>
inline void debugPrint(float msg, bool endline)
{
    #if IUDEBUG_ANY == 1
    if (Serial.availableForWrite()) {
        if (endline) {
            Serial.println(msg, 6);
        } else {
            Serial.print(msg, 6);
        }
    }
    #endif
}

#endif // IUDEBUGGER_H
