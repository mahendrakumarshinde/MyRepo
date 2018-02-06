#ifndef COMPONENT_H
#define COMPONENT_H

#include "Keywords.h"
#include "Logger.h"


/**
 * Base class for all Infinite Uptime board components
 */
class Component
{
    public:
        Component() {}
        virtual ~Component() {}
        /***** Hardware & power management methods *****/
        virtual void setupHardware() {}
        virtual void wakeUp() { m_powerMode = PowerMode::ACTIVE; }
        virtual void lowPower() { m_powerMode = PowerMode::ECONOMY; }
        virtual void suspend() { m_powerMode = PowerMode::SUSPEND; }
        virtual PowerMode::option getPowerMode() { return m_powerMode; }
        virtual void switchToPowerMode(PowerMode::option pMode)
        {
            switch (pMode)
            {
            case PowerMode::ACTIVE:
                wakeUp();
                break;
            case PowerMode::ECONOMY:
                lowPower();
                break;
            case PowerMode::SUSPEND:
                suspend();
                break;
            default:
                break;
            }
        }

    protected:
        PowerMode::option m_powerMode;
};

#endif // COMPONENT_H
