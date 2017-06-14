#ifndef IUCONSTANTS_H
#define IUCONSTANTS_H


/* ============================= Operation Modes ============================= */

/**
 * Power modes are enforced at component level and are controlled either
 * automatically or by the user
 */
class powerMode
{
  public:
    enum  option : uint8_t
    {
      ACTIVE  = 0,
      SLEEP   = 1,
      SUSPEND = 2,
      COUNT   = 3,
    };
};

/**
 * Usage modes are user controlled, they describe how the device is being used
 */
class usageMode
{
  public:
    enum  option : uint8_t
    {
      CALIBRATION   = 0,
      CONFIGURATION = 1,
      OPERATION     = 2,
      COUNT         = 3,
    };
};

/**
 * Acquisition modes are mostly user controlled, with some automatic mode switching
 */
class acquisitionMode
{
  public:
    enum  option : uint8_t
    {
      FEATURE   = 0,
      EXTENSIVE = 1,
      RECORD    = 2,
      WIRED     = 3,
      NONE      = 4,
      COUNT     = 5,
    };
};

/**
 * Operation states describe the production status, inferred from calculated features
 * and user-defined thresholds
 */
class operationState
{
  public:
    enum  option : uint8_t
    {
      IDLE    = 0,
      NORMAL  = 1,
      WARNING = 2,
      DANGER  = 3,
      COUNT   = 4,
    };
};


/* ============================= Operation Presets ============================= */

class powerPreset
{
  public:
    enum  option : uint8_t
    {
      ACTIVE  = 0,
      SUSPEND = 2,
      LOW1    = 3,
      LOW2    = 4,
      COUNT   = 5,
    };
};


/* ============================= Machine Presets ============================= */

class machinePreset
{
  public:
    enum  option : uint8_t
    {
      MOTOR   = 0,
      PRESS   = 1,
      COUNT   = 2,
    };
};


#endif // IUCONSTANTS_H
