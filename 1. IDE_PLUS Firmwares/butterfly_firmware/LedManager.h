#ifndef LEDMANAGER_H
#define LEDMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "RGBLed.h"
#include "FeatureClass.h"


/* =============================================================================
    Operation State values
============================================================================= */

/**
 * Operation states describe the production status
 *
 * The state is determined by features values and user-defined thresholds.
 */
namespace OperationState
{
    const uint8_t IDLE    = 0;
    const uint8_t NORMAL  = 1;
    const uint8_t WARNING = 2;
    const uint8_t DANGER  = 3;
    const uint8_t COUNT   = 4;
}

/* =============================================================================
    Visual representation (color blinking or breathing) of a status
============================================================================= */

struct StatusVisual
{
    RGBColor color = RGB_RED;
    uint32_t transition = 0;
    uint32_t onDuration = 0;
    uint32_t offDuration = 1000;
    StatusVisual() { }
    StatusVisual(RGBColor c, uint32_t trans, uint32_t onDur, uint32_t offDur) {
        color = c;
        transition = trans;
        onDuration = onDur;
        offDuration = offDur;
    };
};

extern StatusVisual STATUS_NO_STATUS;
extern StatusVisual STATUS_WIFI_WORKING;
extern StatusVisual STATUS_WIFI_CONNECTED;
extern StatusVisual STATUS_IS_ALIVE;
extern StatusVisual STATUS_OTA_DOWNLOAD;
extern StatusVisual STATUS_OTA_UPGRADE;
extern StatusVisual STATUS_OTA_ROLLBACK;

/* =============================================================================
    Led Manager
============================================================================= */

/**
 * Manage the LED colors to reflect the device status.
 *
 * The role of the LED Manager is to translate all the different states, status
 * or values of the device into visual signals.
 *
 * The Led Manager has a queue of 2 colors:
 *  - one to show the operation state (baseline color)
 *  - one to show a specific satus (blinking or "breathing color")
 */
class LedManager
{
    public:
        /***** Preset values and default settings *****/
        static const uint8_t maxFeatureCount = 6;
        static const uint8_t LED_STRIP_DATA_PIN = 14; // IDE1.5_PORT_CHANGE Duplicate can be removed
        static const uint8_t LED_STRIP_CLOCK_PIN = 12; // Duplicate can be removed
        /***** Core *****/
        LedManager(RGBLed *led, RGBLed *ledStrip=NULL);
        void updateColors();
        /***** Operation state *****/
        void showOperationState(uint8_t state);
        uint8_t getOperationState() { return m_operationState; }
        /***** Status management *****/
        void showStatus(StatusVisual *status);
        void setBaselineStatus(StatusVisual *status);
        void resetStatus();
        /***** Color override *****/
        void overrideColor(RGBColor color);
        void stopColorOverride();

    protected:
        /***** Core *****/
        RGBLed *m_led;
        RGBLed *m_ledStrip;
        /***** Operation state management *****/
        uint8_t m_operationState = OperationState::IDLE;
        RGBColor m_getOpStateColor();
        /***** Status management *****/
        StatusVisual *m_baselineStatus;
        /***** Color override *****/
        bool m_overriden = false;
        StatusVisual m_forceVisual;
};

#endif // LEDMANAGER_H
