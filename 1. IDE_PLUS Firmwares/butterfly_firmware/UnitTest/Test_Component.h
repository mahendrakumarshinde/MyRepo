#ifndef TEST_COMPONENT_H_INCLUDED
#define TEST_COMPONENT_H_INCLUDED


#include <ArduinoUnit.h>
#include "../Component.h"


test(Component__power_mode)
{
    Component component = Component();

    component.wakeUp();
    assertEqual(component.getPowerMode(), PowerMode::ACTIVE);
    component.lowPower();
    assertEqual(component.getPowerMode(), PowerMode::ECONOMY);
    component.suspend();
    assertEqual(component.getPowerMode(), PowerMode::SUSPEND);

    component.switchToPowerMode(PowerMode::ACTIVE);
    assertEqual(component.getPowerMode(), PowerMode::ACTIVE);
    component.switchToPowerMode(PowerMode::ECONOMY);
    assertEqual(component.getPowerMode(), PowerMode::ECONOMY);
    component.switchToPowerMode(PowerMode::SUSPEND);
    assertEqual(component.getPowerMode(), PowerMode::SUSPEND);
}

#endif // TEST_COMPONENT_H_INCLUDED
