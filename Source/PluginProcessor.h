#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "MidiMapper.h"
#include "SampleEngine.h"
#include "AdgParser.h"
#include "PresetManager.h"
#include "PadMappingManager.h"

class MPSDrumMachineProcessor : public juce::AudioProcessor
{
public:
    MPSDrumMachineProcessor();
    ~MPSDrumMachineProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    MidiMapper& getMidiMapper() { return midiMapper; }
    SampleEngine& getSampleEngine() { return sampleEngine; }
    AdgParser& getAdgParser() { return adgParser; }
    PresetManager& getPresetManager() { return presetManager; }
    PadMappingManager& getPadMappingManager() { return padMappingManager; }

    std::function<void (int midiNote, float velocity)> onMidiTrigger;

    void loadKitSamples (const DkitPreset& kit);

    void swapPadsAndSave (int noteA, int noteB);
    void saveCurrentMappingOverlay();
    void resetCurrentMappingToDefault();

    void setSamplesPath (const juce::File& path);
    void setPresetsPath (const juce::File& path);
    juce::File getSamplesPath() const { return presetManager.getSamplesDir(); }
    juce::File getPresetsPath() const { return presetManager.getPresetsDir(); }

private:
    MidiMapper midiMapper;
    SampleEngine sampleEngine;
    AdgParser adgParser;
    PresetManager presetManager;
    PadMappingManager padMappingManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MPSDrumMachineProcessor)
};
