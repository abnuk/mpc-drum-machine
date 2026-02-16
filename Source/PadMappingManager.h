#pragma once
#include <juce_core/juce_core.h>
#include <map>
#include <optional>

class PadMappingManager
{
public:
    PadMappingManager();

    using PadMapping = std::map<int, juce::File>;

    void saveMapping (const juce::String& presetId, const PadMapping& mapping);
    std::optional<PadMapping> loadMapping (const juce::String& presetId) const;
    bool hasCustomMapping (const juce::String& presetId) const;
    void clearMapping (const juce::String& presetId);

    static juce::String makePresetId (const juce::File& presetFile);

private:
    juce::File getMappingsDir() const;
    juce::File getMappingFile (const juce::String& presetId) const;
};
