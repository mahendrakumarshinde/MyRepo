#include "IUBattery.h"

IUBattery::IUBattery() : m_status(100)
{
    //ctor
}

IUBattery::~IUBattery()
{
    //dtor
}

//Battery Status calculation function
void IUBattery::updateStatus()
{
  float x = float(analogRead(39)) / float(1000);
  m_status = int(-261.7 * x  + 484.06);
}

void IUBattery::activate()
{
  pinMode(chargerCHG, INPUT_PULLUP); // Enable LiPo charger's CHG read.
  //For Battery Monitoring//
  analogReference(EXTERNAL);
  analogReadResolution(12);
  analogReadAveraging(32);
}
