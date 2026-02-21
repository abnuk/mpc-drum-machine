#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <functional>

struct DkitPadMapping
{
    int midiNote = -1;
    juce::String sampleFile;   // relative to samplesDir
    juce::String sampleName;
};

struct DkitPreset
{
    juce::String name;
    juce::String author;
    juce::String description;
    juce::String source;
    juce::String createdAt;
    juce::File sourceFile;
    std::vector<DkitPadMapping> pads;
};

class PresetManager
{
public:
    PresetManager();

    void setSamplesDir (const juce::File& dir);
    void setPresetsDir (const juce::File& dir);
    juce::File getSamplesDir() const { return samplesDir; }
    juce::File getPresetsDir() const { return presetsDir; }

    void scanForPresets();

    int getNumPresets() const;
    juce::String getPresetName (int index) const;
    int getCurrentPresetIndex() const { return currentIndex; }

    bool loadPreset (int index);
    bool loadNextPreset();
    bool loadPreviousPreset();

    const DkitPreset& getCurrentKit() const { return currentKit; }

    bool savePreset (const juce::String& name,
                     const std::map<int, juce::File>& padMappings);

    std::function<void (const DkitPreset&)> onPresetLoaded;

    static juce::File getDefaultSamplesDir();
    static juce::File getDefaultPresetsDir();

    juce::File resolveSamplePath (const juce::String& relativePath) const;
    juce::String makeRelativeSamplePath (const juce::File& sampleFile);

    static DkitPreset parseDkitJson (const juce::File& file);
    static bool writeDkitJson (const juce::File& file, const DkitPreset& preset);

private:
    struct PresetEntry
    {
        juce::String name;
        juce::File file;
    };

    juce::File samplesDir;
    juce::File presetsDir;
    std::vector<PresetEntry> presets;
    int currentIndex = -1;
    DkitPreset currentKit;

    bool loadDkitFile (const juce::File& file);
};
