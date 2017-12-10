#ifndef TEST_COMPONENT_H_INCLUDED
#define TEST_COMPONENT_H_INCLUDED


#include <ArduinoUnit.h>
#include "../Component.h"


test(Component__power_mode)
{
    Component component = Component();

    component.wakeUp();
    assertEqual(component.getPowerMode(), PowerMode::ACTIVE);
    component.sleep();
    assertEqual(component.getPowerMode(), PowerMode::SLEEP);
    component.suspend();
    assertEqual(component.getPowerMode(), PowerMode::SUSPEND);

    component.switchToPowerMode(PowerMode::ACTIVE);
    assertEqual(component.getPowerMode(), PowerMode::ACTIVE);
    component.switchToPowerMode(PowerMode::SLEEP);
    assertEqual(component.getPowerMode(), PowerMode::SLEEP);
    component.switchToPowerMode(PowerMode::SUSPEND);
    assertEqual(component.getPowerMode(), PowerMode::SUSPEND);
}

#endif // TEST_COMPONENT_H_INCLUDED
