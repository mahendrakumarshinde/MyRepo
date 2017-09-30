#ifndef IUCOMPONENT_H
#define IUCOMPONENT_H

#include "Keywords.h"

namespace IUComponent
{
    /**
     * Base class for all Infinite Uptime board components
     */
    class ABCComponent
    {
        public:
            ABCComponent() {}
            virtual ~ABCComponent() {}
            // Hardware & power management methods
            virtual void wakeUp() { m_powerMode = PowerMode::ACTIVE; }
            virtual void sleep() { m_powerMode = PowerMode::SLEEP; }
            virtual void suspend() { m_powerMode = PowerMode::SUSPEND; }
            virtual PowerMode::option getPowerMode() { return m_powerMode; }
            virtual void switchToPowerMode(PowerMode::option pMode)
            {
                switch (pMode)
                {
                case PowerMode::ACTIVE:
                    wakeUp();
                    break;
                case PowerMode::SLEEP:
                    sleep();
                    break;
                case PowerMode::SUSPEND:
                    suspend();
                    break;
                default:
                    if (debugMode)
                    {
                        raiseException("Undefined power mode");
                    }
                    break;
                }
            }

        protected:
            PowerMode::option m_powerMode;
    };
};

#endif // IUCOMPONENT_H
