#ifndef IUDEBUGGER_H
#define IUDEBUGGER_H

#include <Arduino.h>

/* =============================================================================
    Flag reading
============================================================================= */

const bool setupDebugMode = (bool) IUDEBUG_SETUP;
const bool loopDebugMode = (bool) IUDEBUG_MAIN;
const bool asyncDebugMode = (bool) IUDEBUG_ASYNC;
const bool featureDebugMode = (bool) IUDEBUG_FEATURE;
const bool debugMode = (bool) IUDEBUG_ANY;


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
