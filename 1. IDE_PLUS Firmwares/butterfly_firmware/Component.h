#ifndef COMPONENT_H
#define COMPONENT_H

#include <Arduino.h>


/* =============================================================================
    Component Power Modes
============================================================================= */

/**
 * Define the power mode at component level
 */
namespace PowerMode
{
    enum option : uint8_t {SUSPEND,
                           DEEP_SLEEP,
                           SLEEP,
                           LOW_2,
                           LOW_1,
                           REGULAR,
                           ENHANCED,
                           PERFORMANCE,
                           COUNT};
}


/* =============================================================================
    Component base class
============================================================================= */

/**
 * Base class for all Infinite Uptime board components
 */
class Component
{
    public:
        Component() : m_powerMode(PowerMode::REGULAR) {}
        virtual ~Component() {}
        /***** Hardware & power management methods *****/
        virtual void setupHardware() {}
        virtual PowerMode::option getPowerMode() { return m_powerMode; }
        virtual void setPowerMode(PowerMode::option pMode)
            { m_powerMode = pMode; }

    protected:
        PowerMode::option m_powerMode;
};

#endif // COMPONENT_H
