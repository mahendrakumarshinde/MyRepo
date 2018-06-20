#include "LedManager.h"

/* =============================================================================
    Visual representation (color blinking or breathing) of a status
============================================================================= */

StatusVisual STATUS_NO_STATUS(RGB_BLACK, 0, 0, 1000);
StatusVisual STATUS_WIFI_WORKING(RGB_PURPLE, 25, 50, 50);
StatusVisual STATUS_WIFI_CONNECTED(RGB_WHITE, 0, 100, 3000);
StatusVisual STATUS_IS_ALIVE(RGB_CYAN, 25, 50, 50);


/* =============================================================================
    Led Manager - Core
============================================================================= */

LedManager::LedManager(RGBLed *led) :
    m_led(led),
    m_forceVisual(RGB_BLACK, 0, 1000, 0),
    m_overriden(false)
{
    m_led->startNewColorQueue(2);
    m_led->unlockColors();
    setBaselineStatus(&STATUS_NO_STATUS);
}


/* =============================================================================
    Operation state
============================================================================= */

void LedManager::showOperationState(uint8_t state)
{
    m_operationState = state;
    m_led->replaceColor(0, m_getOpStateColor());
}

RGBColor LedManager::m_getOpStateColor()
{
    RGBColor col;
    switch(m_operationState) {
        case OperationState::IDLE:
            col = RGB_BLUE;
            break;
        case OperationState::NORMAL:
            col = RGB_GREEN;
            break;
        case OperationState::WARNING:
            col = RGB_ORANGE;
            break;
        case OperationState::DANGER:
            col = RGB_RED;
            break;
    }
    return col;
}


/* =============================================================================
    Status management
============================================================================= */

/**
 * Show the status color (the LED must be unlocked).
 *
 * Status in shown on color #2 for onDuration, while color #1 still shows the
 * operation state (for offDuration).
 */
void LedManager::showStatus(StatusVisual *status)
{
    if (!m_overriden) {
        m_led->replaceColor(0, m_getOpStateColor(),
                            status->transition, status->offDuration);
        m_led->replaceColor(1, status->color, status->transition, status->onDuration);
    }
}

/**
 *
 */
void LedManager::setBaselineStatus(StatusVisual *status)
{
    m_baselineStatus = status;
    showStatus(m_baselineStatus);
}

/**
 * Reset the LED management to OP state + baseline status.
 */
void LedManager::resetStatus()
{
    if (!m_overriden) {
        m_led->unlockColors();
        showStatus(m_baselineStatus);
    }
}


/* =============================================================================
    Color override
============================================================================= */

/**
 * Override the normal LED management to show a single color.
 *
 * This function locks the LED: the LED color cannot be changed until the LED is
 * unlocked or LedManager.resetStatus is called.
 */
void LedManager::overrideColor(RGBColor color)
{
    m_led->unlockColors();
    m_forceVisual.color = color;
    m_overriden = false;
    showStatus(&m_forceVisual);
    m_overriden = true;
    m_led->lockColors();
}

void LedManager::stopColorOverride()
{
    m_overriden = false;
    resetStatus();
}

