#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "PadComponent.h"
#include "PresetListComponent.h"
#include "SampleBrowserComponent.h"
#include "LookAndFeel.h"

class SettingsOverlay : public juce::Component
{
public:
    SettingsOverlay (BeatwerkProcessor& proc);
    ~SettingsOverlay() override;
    void paint (juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onClose;

private:
    BeatwerkProcessor& processor;
    juce::Label titleLabel;

    juce::Label kitLabel;
    juce::ComboBox kitBox;
    juce::Label kitInfoLabel;

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
    juce::TextButton closeButton { juce::CharPointer_UTF8 ("\xc3\x97") };

    std::vector<juce::String> kitIds;
    void populateKitBox();
    void updateKitInfoLabel();
    void updateLearnButtonStates();
    void doAbletonImport();
};

class BeatwerkEditor : public juce::AudioProcessorEditor,
                             public juce::DragAndDropContainer
{
public:
    BeatwerkEditor (BeatwerkProcessor&);
    ~BeatwerkEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void refreshPads();
    void rebuildPadGrid();
    void updatePresetLabel();

    bool shouldDropFilesWhenDraggedExternally (
        const juce::DragAndDropTarget::SourceDetails& details,
        juce::StringArray& files, bool& canMoveFiles) override;

private:
    BeatwerkProcessor& processorRef;
    DarkLookAndFeel darkLnf;

    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::Label presetLabel;
    juce::TextButton samplesViewButton { "Samples" };
    juce::TextButton presetsViewButton { "Presets" };
    juce::TextButton settingsButton { "Settings" };

    juce::OwnedArray<PadComponent> padComponents;

    std::unique_ptr<SampleBrowserComponent> sampleBrowser;
    bool showingSampleBrowser = false;
    void toggleSampleBrowser();
    void showSampleInBrowser (const juce::File& file);

    std::unique_ptr<PresetListComponent> presetListComponent;
    bool showingPresetList = false;
    void togglePresetView();

    std::unique_ptr<SettingsOverlay> settingsOverlay;

    void showSettings();
    void hideSettings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatwerkEditor)
};
