#include "PluginEditor.h"

//==============================================================================
// SettingsOverlay
//==============================================================================

SettingsOverlay::SettingsOverlay (MPSDrumMachineProcessor& proc) : processor (proc)
{
    titleLabel.setText ("Settings", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::textBright);
    addAndMakeVisible (titleLabel);

    libPathLabel.setText ("Ableton Core Library:", juce::dontSendNotification);
    libPathLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::textDim);
    addAndMakeVisible (libPathLabel);

    libPathEditor.setText (processor.getAdgParser().getAbletonLibraryPath().getFullPathName());
    addAndMakeVisible (libPathEditor);

    browseButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> (
            "Select Ableton Core Library folder",
            processor.getAdgParser().getAbletonLibraryPath(),
            "",
            true);

        chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectDirectories,
                              [this, chooser] (const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.isDirectory())
            {
                processor.getAdgParser().setAbletonLibraryPath (result);
                libPathEditor.setText (result.getFullPathName());

                processor.getPresetManager().clearScanDirectories();

                juce::StringArray drumRackPaths = {
                    "Racks/Drum Racks",
                    "Presets/Instruments/Drum Rack",
                    "Defaults/Slicing"
                };
                for (auto& subPath : drumRackPaths)
                {
                    auto dir = result.getChildFile (subPath);
                    if (dir.isDirectory())
                        processor.getPresetManager().addScanDirectory (dir);
                }

                processor.getPresetManager().scanForPresets();
            }
        });
    };
    addAndMakeVisible (browseButton);

    navChannelLabel.setText ("Nav MIDI Channel:", juce::dontSendNotification);
    navChannelLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::textDim);
    addAndMakeVisible (navChannelLabel);

    navChannelBox.addItem ("Any", 1);
    for (int i = 1; i <= 16; ++i)
        navChannelBox.addItem ("Ch " + juce::String (i), i + 1);
    navChannelBox.setSelectedId (processor.getMidiMapper().getNavChannel() + 1);
    navChannelBox.onChange = [this]
    {
        processor.getMidiMapper().setNavChannel (navChannelBox.getSelectedId() - 1);
    };
    addAndMakeVisible (navChannelBox);

    prevCCLabel.setText ("Prev CC Number:", juce::dontSendNotification);
    prevCCLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::textDim);
    addAndMakeVisible (prevCCLabel);

    for (int i = 0; i < 128; ++i)
        prevCCBox.addItem ("CC " + juce::String (i), i + 1);
    prevCCBox.setSelectedId (processor.getMidiMapper().getPrevCCNumber() + 1);
    prevCCBox.onChange = [this]
    {
        processor.getMidiMapper().setPrevCCNumber (prevCCBox.getSelectedId() - 1);
    };
    addAndMakeVisible (prevCCBox);

    nextCCLabel.setText ("Next CC Number:", juce::dontSendNotification);
    nextCCLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::textDim);
    addAndMakeVisible (nextCCLabel);

    for (int i = 0; i < 128; ++i)
        nextCCBox.addItem ("CC " + juce::String (i), i + 1);
    nextCCBox.setSelectedId (processor.getMidiMapper().getNextCCNumber() + 1);
    nextCCBox.onChange = [this]
    {
        processor.getMidiMapper().setNextCCNumber (nextCCBox.getSelectedId() - 1);
    };
    addAndMakeVisible (nextCCBox);

    scanButton.onClick = [this]
    {
        scanButton.setEnabled (false);
        scanButton.setButtonText ("Scanning...");

        processor.getPresetManager().scanForPresets();

        auto numPresets = processor.getPresetManager().getNumPresets();
        scanButton.setButtonText ("Found " + juce::String (numPresets) + " presets");

        auto* btn = &scanButton;
        juce::Timer::callAfterDelay (2000, [safeThis = juce::Component::SafePointer<SettingsOverlay> (this), btn]
        {
            if (safeThis != nullptr)
            {
                btn->setButtonText ("Scan Library");
                btn->setEnabled (true);
            }
        });
    };
    addAndMakeVisible (scanButton);

    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeButton);

    savePresetButton.onClick = [this]
    {
        auto* alertWin = new juce::AlertWindow ("Save Preset",
                                                 "Enter a name for the preset:",
                                                 juce::MessageBoxIconType::QuestionIcon);
        alertWin->addTextEditor ("name", "My Drum Kit");
        alertWin->addButton ("Save", 1);
        alertWin->addButton ("Cancel", 0);

        alertWin->enterModalState (true, juce::ModalCallbackFunction::create (
            [this, alertWin] (int result)
            {
                if (result == 1)
                {
                    auto name = alertWin->getTextEditorContents ("name");
                    std::map<int, juce::File> mappings;
                    for (auto& pad : MidiMapper::getAllPads())
                    {
                        auto file = processor.getSampleEngine().getSampleFile (pad.midiNote);
                        if (file.existsAsFile())
                            mappings[pad.midiNote] = file;
                    }
                    processor.getPresetManager().saveCustomPreset (name, mappings);
                    processor.getPresetManager().scanForPresets();
                }
                delete alertWin;
            }), true);
    };
    addAndMakeVisible (savePresetButton);
}

void SettingsOverlay::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::bgDark.withAlpha (0.95f));

    auto area = getLocalBounds().reduced (30);
    g.setColour (DarkLookAndFeel::bgMedium);
    g.fillRoundedRectangle (area.toFloat(), 10.0f);

    g.setColour (DarkLookAndFeel::bgLight);
    g.drawRoundedRectangle (area.toFloat(), 10.0f, 1.0f);
}

void SettingsOverlay::resized()
{
    auto area = getLocalBounds().reduced (50);

    titleLabel.setBounds (area.removeFromTop (35));
    area.removeFromTop (15);

    libPathLabel.setBounds (area.removeFromTop (22));
    auto pathRow = area.removeFromTop (30);
    browseButton.setBounds (pathRow.removeFromRight (90));
    pathRow.removeFromRight (5);
    libPathEditor.setBounds (pathRow);

    area.removeFromTop (8);
    scanButton.setBounds (area.removeFromTop (30).withWidth (140));

    area.removeFromTop (15);

    navChannelLabel.setBounds (area.removeFromTop (22));
    navChannelBox.setBounds (area.removeFromTop (28).withWidth (200));

    area.removeFromTop (10);

    prevCCLabel.setBounds (area.removeFromTop (22));
    prevCCBox.setBounds (area.removeFromTop (28).withWidth (200));

    area.removeFromTop (10);

    nextCCLabel.setBounds (area.removeFromTop (22));
    nextCCBox.setBounds (area.removeFromTop (28).withWidth (200));

    area.removeFromTop (20);
    savePresetButton.setBounds (area.removeFromTop (32).withWidth (160));

    closeButton.setBounds (area.removeFromBottom (32).withWidth (100));
}

//==============================================================================
// MPSDrumMachineEditor
//==============================================================================

MPSDrumMachineEditor::MPSDrumMachineEditor (MPSDrumMachineProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&darkLnf);
    setOpaque (true);

    // Top bar
    prevButton.onClick = [this]
    {
        processorRef.getPresetManager().loadPreviousPreset();
        updatePresetLabel();
        refreshPads();
    };
    addAndMakeVisible (prevButton);

    nextButton.onClick = [this]
    {
        processorRef.getPresetManager().loadNextPreset();
        updatePresetLabel();
        refreshPads();
    };
    addAndMakeVisible (nextButton);

    presetLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    presetLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::textBright);
    presetLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (presetLabel);
    updatePresetLabel();

    presetsViewButton.onClick = [this] { togglePresetView(); };
    addAndMakeVisible (presetsViewButton);

    settingsButton.onClick = [this] { showSettings(); };
    addAndMakeVisible (settingsButton);

    // Preset browser
    presetListComponent = std::make_unique<PresetListComponent> (processorRef.getPresetManager());
    presetListComponent->setVisible (false);
    presetListComponent->onPresetSelected = [this] (int index)
    {
        processorRef.getPresetManager().loadPreset (index);
        updatePresetLabel();
        refreshPads();
    };
    addAndMakeVisible (presetListComponent.get());

    // Create pad components
    for (auto& padInfo : MidiMapper::getAllPads())
    {
        auto* pad = new PadComponent (padInfo, processorRef.getSampleEngine());
        pad->onSampleDropped = [this] (int /*note*/, const juce::File& /*file*/)
        {
            refreshPads();
            processorRef.saveCurrentMappingOverlay();
        };
        pad->onPadSwapped = [this] (int sourceNote, int targetNote)
        {
            processorRef.swapPadsAndSave (sourceNote, targetNote);
            refreshPads();
        };
        pad->onResetMapping = [this]
        {
            processorRef.resetCurrentMappingToDefault();
            refreshPads();
        };
        addAndMakeVisible (pad);
        padComponents.add (pad);
    }

    // MIDI trigger callback for flash animation
    processorRef.onMidiTrigger = [this] (int midiNote, float velocity)
    {
        juce::MessageManager::callAsync ([this, midiNote, velocity]
        {
            for (auto* pad : padComponents)
            {
                if (pad->getMidiNote() == midiNote)
                {
                    pad->triggerFlash (velocity);
                    break;
                }
            }
        });
    };

    // Preset load callback to refresh UI
    processorRef.getPresetManager().onPresetLoaded = [this] (const AdgDrumKit& kit)
    {
        processorRef.loadKitSamples (kit);
        juce::MessageManager::callAsync ([this]
        {
            updatePresetLabel();
            refreshPads();
            if (presetListComponent != nullptr)
                presetListComponent->setActivePreset (processorRef.getPresetManager().getCurrentPresetIndex());
        });
    };

    // Set size LAST so resized() runs after all components exist
    setSize (820, 660);
    setResizable (true, true);
    setResizeLimits (600, 500, 1600, 1200);
}

MPSDrumMachineEditor::~MPSDrumMachineEditor()
{
    processorRef.onMidiTrigger = nullptr;
    setLookAndFeel (nullptr);
}

void MPSDrumMachineEditor::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::bgDark);

    // Top bar background
    g.setColour (DarkLookAndFeel::bgMedium);
    g.fillRect (0, 0, getWidth(), 50);

    // Separator line
    g.setColour (DarkLookAndFeel::accent.withAlpha (0.5f));
    g.fillRect (0, 49, getWidth(), 2);
}

void MPSDrumMachineEditor::resized()
{
    auto area = getLocalBounds();

    // Top bar
    auto topBar = area.removeFromTop (50).reduced (10, 8);
    prevButton.setBounds (topBar.removeFromLeft (36));
    topBar.removeFromLeft (5);
    nextButton.setBounds (topBar.removeFromLeft (36));
    settingsButton.setBounds (topBar.removeFromRight (90));
    topBar.removeFromRight (5);
    presetsViewButton.setBounds (topBar.removeFromRight (90));
    topBar.removeFromRight (10);
    presetLabel.setBounds (topBar);

    // Content area
    area.reduce (15, 10);

    if (showingPresetList)
    {
        if (presetListComponent != nullptr)
            presetListComponent->setBounds (area);
    }
    else
    {
        // Pad grid: 4 columns x 6 rows
        int cols = 4;
        int rows = 6;
        int padWidth = area.getWidth() / cols;
        int padHeight = area.getHeight() / rows;

        for (int i = 0; i < padComponents.size(); ++i)
        {
            int row = i / cols;
            int col = i % cols;
            padComponents[i]->setBounds (area.getX() + col * padWidth,
                                         area.getY() + row * padHeight,
                                         padWidth, padHeight);
        }
    }
}

void MPSDrumMachineEditor::refreshPads()
{
    for (auto* pad : padComponents)
        pad->updateSampleDisplay();
}

void MPSDrumMachineEditor::updatePresetLabel()
{
    auto& pm = processorRef.getPresetManager();
    int idx = pm.getCurrentPresetIndex();
    if (idx >= 0 && idx < pm.getNumPresets())
    {
        presetLabel.setText (juce::String (idx + 1) + "/" + juce::String (pm.getNumPresets())
                            + "  " + pm.getPresetName (idx),
                            juce::dontSendNotification);
    }
    else
    {
        int total = pm.getNumPresets();
        if (total > 0)
            presetLabel.setText (juce::String (total) + " presets available",
                                juce::dontSendNotification);
        else
            presetLabel.setText ("MPS Drum Machine", juce::dontSendNotification);
    }
}

void MPSDrumMachineEditor::togglePresetView()
{
    showingPresetList = ! showingPresetList;

    if (showingPresetList)
    {
        presetsViewButton.setButtonText ("Pads");

        for (auto* pad : padComponents)
            pad->setVisible (false);

        if (presetListComponent != nullptr)
        {
            presetListComponent->refreshPresetList();
            presetListComponent->setVisible (true);
        }
    }
    else
    {
        presetsViewButton.setButtonText ("Presets");

        if (presetListComponent != nullptr)
            presetListComponent->setVisible (false);

        for (auto* pad : padComponents)
            pad->setVisible (true);
    }

    resized();
}

void MPSDrumMachineEditor::showSettings()
{
    settingsOverlay = std::make_unique<SettingsOverlay> (processorRef);
    settingsOverlay->onClose = [this] { hideSettings(); };
    settingsOverlay->setBounds (getLocalBounds());
    addAndMakeVisible (settingsOverlay.get());
}

void MPSDrumMachineEditor::hideSettings()
{
    settingsOverlay.reset();
    updatePresetLabel();
}
