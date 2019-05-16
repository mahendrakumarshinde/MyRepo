#ifndef TEST_COMPONENT_H_INCLUDED
#define TEST_COMPONENT_H_INCLUDED


#include <ArduinoUnit.h>
#include "../Component.h"


test(Component__power_mode)
{
    Component component = Component();
    assertEqual(component.getPowerMode(), PowerMode::REGULAR);
    component.setPowerMode(PowerMode::SLEEP);
    assertEqual(component.getPowerMode(), PowerMode::SLEEP);
}

#endif // TEST_COMPONENT_H_INCLUDED
