#pragma once
#include <juce_core/juce_core.h>
#include "MidiMapper.h"
#include <vector>

struct DrumKitDefinition
{
    juce::String id;
    juce::String name;
    juce::String manufacturer;
    std::vector<PadInfo> pads;
    int columns = 4;
};

class DrumKitLibrary
{
public:
    static const std::vector<DrumKitDefinition>& getAllKits();
    static const DrumKitDefinition* findKit (const juce::String& id);
    static const DrumKitDefinition& getDefaultKit();
    static std::vector<juce::String> getManufacturers();
    static std::vector<const DrumKitDefinition*> getKitsByManufacturer (const juce::String& mfr);
};
