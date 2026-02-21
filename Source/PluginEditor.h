#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "PadComponent.h"
#include "PresetListComponent.h"
#include "LookAndFeel.h"

class SettingsOverlay : public juce::Component
{
public:
    SettingsOverlay (MPSDrumMachineProcessor& proc);
    ~SettingsOverlay() override;
    void paint (juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onClose;

private:
    MPSDrumMachineProcessor& processor;
    juce::Label titleLabel;

    juce::Label samplesPathLabel;
    juce::TextEditor samplesPathEditor;
    juce::TextButton samplesBrowseButton { "Browse..." };

    juce::Label presetsPathLabel;
    juce::TextEditor presetsPathEditor;
    juce::TextButton presetsBrowseButton { "Browse..." };

    juce::TextButton importAbletonButton { "Import from Ableton Live" };
    juce::TextButton scanButton { "Scan Library" };

    double importProgress = 0.0;
    juce::ProgressBar importProgressBar { importProgress };
    juce::Label importStatusLabel;
    bool importRunning = false;

    juce::Label navChannelLabel;
    juce::ComboBox navChannelBox;
    juce::Label prevCCLabel;
    juce::ComboBox prevCCBox;
    juce::TextButton prevLearnButton { "Learn" };
    juce::Label nextCCLabel;
    juce::ComboBox nextCCBox;
    juce::TextButton nextLearnButton { "Learn" };

    juce::TextButton savePresetButton { "Save Preset..." };
    juce::TextButton closeButton { "Close" };

    void updateLearnButtonStates();
    void doAbletonImport();
};

class MPSDrumMachineEditor : public juce::AudioProcessorEditor,
                             public juce::DragAndDropContainer
{
public:
    MPSDrumMachineEditor (MPSDrumMachineProcessor&);
    ~MPSDrumMachineEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void refreshPads();
    void updatePresetLabel();

private:
    MPSDrumMachineProcessor& processorRef;
    DarkLookAndFeel darkLnf;

    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::Label presetLabel;
    juce::TextButton presetsViewButton { "Presets" };
    juce::TextButton settingsButton { "Settings" };

    juce::OwnedArray<PadComponent> padComponents;

    std::unique_ptr<PresetListComponent> presetListComponent;
    bool showingPresetList = false;
    void togglePresetView();

    std::unique_ptr<SettingsOverlay> settingsOverlay;

    void showSettings();
    void hideSettings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MPSDrumMachineEditor)
};
