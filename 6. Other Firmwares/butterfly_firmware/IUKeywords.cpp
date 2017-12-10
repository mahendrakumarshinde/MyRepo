#include "IUKeywords.h"

powerMode::option usagePreset::powerPresetDetails[usagePreset::COUNT] =
{
  powerMode::ACTIVE,
  powerMode::ACTIVE,
  powerMode::ACTIVE,
};

acquisitionMode::option usagePreset::acquisitionModeDetails[usagePreset::COUNT] =
{
  acquisitionMode::FEATURE,
  acquisitionMode::RAWDATA,
  acquisitionMode::FEATURE,
};

streamingMode::option usagePreset::streamingModeDetails[usagePreset::COUNT] =
{
  streamingMode::WIRED,
  streamingMode::WIRED,
  streamingMode::BLE,
};
