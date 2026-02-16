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
    void paint (juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onClose;

private:
    MPSDrumMachineProcessor& processor;
    juce::Label titleLabel;
    juce::Label libPathLabel;
    juce::TextEditor libPathEditor;
    juce::TextButton browseButton { "Browse..." };
    juce::Label navChannelLabel;
    juce::ComboBox navChannelBox;
    juce::Label prevCCLabel;
    juce::ComboBox prevCCBox;
    juce::Label nextCCLabel;
    juce::ComboBox nextCCBox;
    juce::TextButton scanButton { "Scan Library" };
    juce::TextButton closeButton { "Close" };
    juce::TextButton savePresetButton { "Save Preset..." };
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

    // Top bar
    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::Label presetLabel;
    juce::TextButton presetsViewButton { "Presets" };
    juce::TextButton settingsButton { "Settings" };

    // Pad grid
    juce::OwnedArray<PadComponent> padComponents;

    // Preset browser
    std::unique_ptr<PresetListComponent> presetListComponent;
    bool showingPresetList = false;
    void togglePresetView();

    // Settings overlay
    std::unique_ptr<SettingsOverlay> settingsOverlay;

    void showSettings();
    void hideSettings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MPSDrumMachineEditor)
};
