#pragma once
#include <juce_core/juce_core.h>
#include "AdgParser.h"
#include "PresetManager.h"
#include <functional>

class AbletonImporter
{
public:
    struct ImportResult
    {
        int presetsImported = 0;
        int samplesCopied = 0;
        int skippedNoSamples = 0;
        int skippedExisting = 0;
        int errors = 0;
        juce::StringArray errorMessages;
        juce::StringArray skippedNames;
    };

    static ImportResult importFromDirectories (
        const juce::Array<juce::File>& adgSourceDirs,
        const juce::File& samplesDir,
        const juce::File& presetsDir,
        AdgParser& parser,
        std::function<void (float progress, const juce::String& status)> onProgress = nullptr);

    static ImportResult importFromDirectory (
        const juce::File& adgSourceDir,
        const juce::File& samplesDir,
        const juce::File& presetsDir,
        AdgParser& parser,
        std::function<void (float progress, const juce::String& status)> onProgress = nullptr);

    static juce::Array<juce::File> findAbletonPresetDirs();

private:
    static juce::String computeRelativeSamplePath (const juce::String& absoluteSamplePath,
                                                    const juce::File& abletonCoreLib);
};
