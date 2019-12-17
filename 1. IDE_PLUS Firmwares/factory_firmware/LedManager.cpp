#include "LedManager.h"

/* =============================================================================
    Visual representation (color blinking or breathing) of a status
============================================================================= */
StatusVisual STATUS_NO_STATUS(RGB_BLACK, 0, 0, 1000);

/* =============================================================================
    Led Manager - Core
============================================================================= */

LedManager::LedManager(RGBLed *led, RGBLed *ledStrip) :
    m_led(led),
    m_ledStrip(ledStrip),
    m_forceVisual(RGB_BLACK, 0, 1000, 0),
    m_overriden(false)
{
    m_led->startNewColorQueue(2);
    m_led->unlockColors();
    if (m_ledStrip) {
        m_ledStrip->startNewColorQueue(2);
        m_ledStrip->unlockColors();
    }
    setBaselineStatus(&STATUS_NO_STATUS);
}

void LedManager::updateColors()
{
    m_led->updateColors();
    if (m_ledStrip) {
        m_ledStrip->updateColors();
    }
}


/* =============================================================================
    Operation state
============================================================================= */

void LedManager::showOperationState(uint8_t state)
{
    m_operationState = state;
    RGBColor col = m_getOpStateColor();
    m_led->replaceColor(0, col);
    if (m_ledStrip) {
        m_ledStrip->replaceColor(0, col);
    }
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
        if (m_ledStrip) {
            m_ledStrip->replaceColor(0, m_getOpStateColor(),
                                     status->transition, status->offDuration);
            m_ledStrip->replaceColor(1, status->color, status->transition, status->onDuration);
        }
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
        if (m_ledStrip) {
            m_ledStrip->unlockColors();
        }
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
    if (m_ledStrip) {
        m_ledStrip->unlockColors();
    }
    m_forceVisual.color = color;
    m_overriden = false;
    if (m_ledStrip) {
        showStatus(&m_forceVisual);
    }
    m_overriden = true;
    m_led->lockColors();
    if (m_ledStrip) {
        m_ledStrip->lockColors();
    }
    updateColors();
}

void LedManager::stopColorOverride()
{
    m_overriden = false;
    resetStatus();
    updateColors();
}

