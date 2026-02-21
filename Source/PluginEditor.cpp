#include "PluginEditor.h"
#include "AbletonImporter.h"

//==============================================================================
// SettingsOverlay
//==============================================================================

SettingsOverlay::SettingsOverlay (MPSDrumMachineProcessor& proc) : processor (proc)
{
    titleLabel.setText ("Settings", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::textBright);
    addAndMakeVisible (titleLabel);

    // Samples path
    samplesPathLabel.setText ("Samples Directory:", juce::dontSendNotification);
    samplesPathLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::textDim);
    addAndMakeVisible (samplesPathLabel);

    samplesPathEditor.setText (processor.getSamplesPath().getFullPathName());
    addAndMakeVisible (samplesPathEditor);

    samplesBrowseButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> (
            "Select Samples Directory",
            processor.getSamplesPath(), "", true);

        chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectDirectories,
                              [this, chooser] (const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.isDirectory())
            {
                processor.setSamplesPath (result);
                samplesPathEditor.setText (result.getFullPathName());
            }
        });
    };
    addAndMakeVisible (samplesBrowseButton);

    // Presets path
    presetsPathLabel.setText ("Presets Directory:", juce::dontSendNotification);
    presetsPathLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::textDim);
    addAndMakeVisible (presetsPathLabel);

    presetsPathEditor.setText (processor.getPresetsPath().getFullPathName());
    addAndMakeVisible (presetsPathEditor);

    presetsBrowseButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> (
            "Select Presets Directory",
            processor.getPresetsPath(), "", true);

        chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectDirectories,
                              [this, chooser] (const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.isDirectory())
            {
                processor.setPresetsPath (result);
                presetsPathEditor.setText (result.getFullPathName());
            }
        });
    };
    addAndMakeVisible (presetsBrowseButton);

    // Import from Ableton
    importAbletonButton.onClick = [this] { doAbletonImport(); };
    addAndMakeVisible (importAbletonButton);

    importProgressBar.setVisible (false);
    addAndMakeVisible (importProgressBar);

    importStatusLabel.setFont (juce::FontOptions (11.0f));
    importStatusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::textDim);
    importStatusLabel.setVisible (false);
    addAndMakeVisible (importStatusLabel);

    // Scan Library
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

    // MIDI Navigation
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

    prevLearnButton.onClick = [this]
    {
        auto& mm = processor.getMidiMapper();
        if (mm.getLearnTarget() == MidiMapper::LearnTarget::Prev)
            mm.cancelLearn();
        else
            mm.startLearn (MidiMapper::LearnTarget::Prev);
        updateLearnButtonStates();
    };
    addAndMakeVisible (prevLearnButton);

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

    nextLearnButton.onClick = [this]
    {
        auto& mm = processor.getMidiMapper();
        if (mm.getLearnTarget() == MidiMapper::LearnTarget::Next)
            mm.cancelLearn();
        else
            mm.startLearn (MidiMapper::LearnTarget::Next);
        updateLearnButtonStates();
    };
    addAndMakeVisible (nextLearnButton);

    processor.getMidiMapper().onLearnComplete =
        [safeThis = juce::Component::SafePointer<SettingsOverlay> (this)]
        (MidiMapper::LearnTarget target, int cc)
    {
        if (safeThis == nullptr)
            return;

        if (target == MidiMapper::LearnTarget::Prev)
            safeThis->prevCCBox.setSelectedId (cc + 1, juce::sendNotification);
        else if (target == MidiMapper::LearnTarget::Next)
            safeThis->nextCCBox.setSelectedId (cc + 1, juce::sendNotification);

        safeThis->updateLearnButtonStates();
    };

    // Save Preset
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
                    processor.getPresetManager().savePreset (name, mappings);
                    processor.getPresetManager().scanForPresets();
                }
                delete alertWin;
            }), true);
    };
    addAndMakeVisible (savePresetButton);

    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeButton);
}

SettingsOverlay::~SettingsOverlay()
{
    processor.getMidiMapper().cancelLearn();
    processor.getMidiMapper().onLearnComplete = nullptr;
}

void SettingsOverlay::updateLearnButtonStates()
{
    auto target = processor.getMidiMapper().getLearnTarget();

    if (target == MidiMapper::LearnTarget::Prev)
    {
        prevLearnButton.setButtonText ("Listening...");
        nextLearnButton.setEnabled (false);
    }
    else if (target == MidiMapper::LearnTarget::Next)
    {
        nextLearnButton.setButtonText ("Listening...");
        prevLearnButton.setEnabled (false);
    }
    else
    {
        prevLearnButton.setButtonText ("Learn");
        prevLearnButton.setEnabled (true);
        nextLearnButton.setButtonText ("Learn");
        nextLearnButton.setEnabled (true);
    }
}

void SettingsOverlay::doAbletonImport()
{
    auto startImport = [this] (const juce::Array<juce::File>& dirs)
    {
        importRunning = true;
        importProgress = 0.0;
        importAbletonButton.setEnabled (false);
        importAbletonButton.setButtonText ("Importing...");
        importProgressBar.setVisible (true);
        importStatusLabel.setVisible (true);
        importStatusLabel.setText ("Preparing...", juce::dontSendNotification);

        auto safeThis = juce::Component::SafePointer<SettingsOverlay> (this);

        juce::Thread::launch ([this, dirs, safeThis]
        {
            auto importResult = AbletonImporter::importFromDirectories (
                dirs,
                processor.getSamplesPath(),
                processor.getPresetsPath(),
                processor.getAdgParser(),
                [safeThis] (float progress, const juce::String& status)
                {
                    juce::MessageManager::callAsync ([safeThis, progress, status]
                    {
                        if (safeThis != nullptr)
                        {
                            safeThis->importProgress = (double) progress;
                            safeThis->importStatusLabel.setText (status, juce::dontSendNotification);
                        }
                    });
                });

            juce::MessageManager::callAsync ([this, importResult, safeThis]
            {
                if (safeThis == nullptr)
                    return;

                importRunning = false;
                importProgress = 1.0;
                processor.getPresetManager().scanForPresets();

                auto msg = "Imported " + juce::String (importResult.presetsImported) + " presets, "
                         + juce::String (importResult.samplesCopied) + " samples copied";
                if (importResult.skippedNoSamples > 0)
                    msg += ", " + juce::String (importResult.skippedNoSamples) + " skipped (no audio samples)";
                if (importResult.skippedExisting > 0)
                    msg += ", " + juce::String (importResult.skippedExisting) + " already existed";

                importStatusLabel.setText (msg, juce::dontSendNotification);
                importAbletonButton.setButtonText ("Import from Ableton Live");
                importAbletonButton.setEnabled (true);

                if (importResult.skippedNoSamples > 0 || importResult.errors > 0)
                {
                    juce::String details;
                    if (importResult.skippedNoSamples > 0)
                    {
                        details += juce::String (importResult.skippedNoSamples)
                                 + " kits skipped (synth-based, no audio samples):\n";
                        for (auto& name : importResult.skippedNames)
                            details += "  - " + name + "\n";
                    }
                    if (importResult.errors > 0)
                    {
                        details += "\n" + juce::String (importResult.errors) + " errors:\n";
                        for (auto& err : importResult.errorMessages)
                            details += "  - " + err + "\n";
                    }
                    details += "\nFull log: ~/Library/Application Support/MPSDrumMachine/import_log.txt";

                    juce::AlertWindow::showMessageBoxAsync (
                        juce::MessageBoxIconType::InfoIcon,
                        "Import Summary",
                        "Imported " + juce::String (importResult.presetsImported) + " presets.\n\n" + details);
                }

                juce::Timer::callAfterDelay (10000, [safeThis]
                {
                    if (safeThis != nullptr && ! safeThis->importRunning)
                    {
                        safeThis->importProgressBar.setVisible (false);
                        safeThis->importStatusLabel.setVisible (false);
                    }
                });
            });
        });
    };

    auto abletonDirs = AbletonImporter::findAbletonPresetDirs();

    if (abletonDirs.isEmpty())
    {
        auto chooser = std::make_shared<juce::FileChooser> (
            "Select folder containing Ableton .adg preset files",
            juce::File::getSpecialLocation (juce::File::userHomeDirectory),
            "", true);

        chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectDirectories,
                              [startImport, chooser] (const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.isDirectory())
            {
                juce::Array<juce::File> dirs;
                dirs.add (result);
                startImport (dirs);
            }
        });
    }
    else
    {
        startImport (abletonDirs);
    }
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

    // Samples path
    samplesPathLabel.setBounds (area.removeFromTop (22));
    auto samplesRow = area.removeFromTop (30);
    samplesBrowseButton.setBounds (samplesRow.removeFromRight (90));
    samplesRow.removeFromRight (5);
    samplesPathEditor.setBounds (samplesRow);

    area.removeFromTop (8);

    // Presets path
    presetsPathLabel.setBounds (area.removeFromTop (22));
    auto presetsRow = area.removeFromTop (30);
    presetsBrowseButton.setBounds (presetsRow.removeFromRight (90));
    presetsRow.removeFromRight (5);
    presetsPathEditor.setBounds (presetsRow);

    area.removeFromTop (10);

    // Import + Scan buttons side by side
    {
        auto row = area.removeFromTop (32);
        importAbletonButton.setBounds (row.removeFromLeft (220));
        row.removeFromLeft (10);
        scanButton.setBounds (row.removeFromLeft (140));
    }

    area.removeFromTop (6);
    importProgressBar.setBounds (area.removeFromTop (18).withWidth (370));
    area.removeFromTop (2);
    importStatusLabel.setBounds (area.removeFromTop (16).withWidth (500));

    area.removeFromTop (10);

    // MIDI settings
    navChannelLabel.setBounds (area.removeFromTop (22));
    navChannelBox.setBounds (area.removeFromTop (28).withWidth (200));

    area.removeFromTop (10);

    prevCCLabel.setBounds (area.removeFromTop (22));
    {
        auto row = area.removeFromTop (28);
        prevCCBox.setBounds (row.removeFromLeft (200));
        row.removeFromLeft (8);
        prevLearnButton.setBounds (row.removeFromLeft (90));
    }

    area.removeFromTop (10);

    nextCCLabel.setBounds (area.removeFromTop (22));
    {
        auto row = area.removeFromTop (28);
        nextCCBox.setBounds (row.removeFromLeft (200));
        row.removeFromLeft (8);
        nextLearnButton.setBounds (row.removeFromLeft (90));
    }

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

    samplesViewButton.onClick = [this] { toggleSampleBrowser(); };
    addAndMakeVisible (samplesViewButton);

    presetsViewButton.onClick = [this] { togglePresetView(); };
    addAndMakeVisible (presetsViewButton);

    settingsButton.onClick = [this] { showSettings(); };
    addAndMakeVisible (settingsButton);

    sampleBrowser = std::make_unique<SampleBrowserComponent> (processorRef.getSampleEngine());
    sampleBrowser->setSamplesDirectory (processorRef.getSamplesPath());
    sampleBrowser->setVisible (false);
    addAndMakeVisible (sampleBrowser.get());

    presetListComponent = std::make_unique<PresetListComponent> (processorRef.getPresetManager());
    presetListComponent->setVisible (false);
    presetListComponent->onPresetSelected = [this] (int index)
    {
        processorRef.getPresetManager().loadPreset (index);
        updatePresetLabel();
        refreshPads();
    };
    presetListComponent->onPresetDeleted = [this] (int index)
    {
        auto& pm = processorRef.getPresetManager();
        auto presetFile = pm.getPresetFile (index);
        if (presetFile != juce::File())
        {
            auto presetId = PadMappingManager::makePresetId (presetFile);
            processorRef.getPadMappingManager().clearMapping (presetId);
        }

        pm.deletePreset (index);
        presetListComponent->refreshPresetList();
        updatePresetLabel();
        refreshPads();
    };
    presetListComponent->onPresetRenamed = [this] (int index, const juce::String& newName)
    {
        auto& pm = processorRef.getPresetManager();
        auto oldFile = pm.getPresetFile (index);
        auto oldPresetId = (oldFile != juce::File())
                               ? PadMappingManager::makePresetId (oldFile)
                               : juce::String();

        if (pm.renamePreset (index, newName))
        {
            auto newFile = pm.getPresetFile (index);
            if (oldPresetId.isNotEmpty() && newFile != juce::File())
            {
                auto newPresetId = PadMappingManager::makePresetId (newFile);
                if (oldPresetId != newPresetId)
                {
                    auto& pmm = processorRef.getPadMappingManager();
                    auto oldMapping = pmm.loadMapping (oldPresetId);
                    if (oldMapping.has_value())
                    {
                        pmm.saveMapping (newPresetId, oldMapping.value());
                        pmm.clearMapping (oldPresetId);
                    }
                }
            }

            presetListComponent->refreshPresetList();
            updatePresetLabel();
        }
    };
    presetListComponent->onSaveNewPreset = [this]
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
                    auto name = alertWin->getTextEditorContents ("name").trim();
                    if (name.isNotEmpty())
                    {
                        std::map<int, juce::File> mappings;
                        for (auto& pad : MidiMapper::getAllPads())
                        {
                            auto file = processorRef.getSampleEngine().getSampleFile (pad.midiNote);
                            if (file.existsAsFile())
                                mappings[pad.midiNote] = file;
                        }
                        processorRef.getPresetManager().savePreset (name, mappings);
                        processorRef.getPresetManager().scanForPresets();
                        presetListComponent->refreshPresetList();
                        updatePresetLabel();
                    }
                }
                delete alertWin;
            }), true);
    };
    addAndMakeVisible (presetListComponent.get());

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
        pad->onLocateSample = [this] (const juce::File& file)
        {
            showSampleInBrowser (file);
        };
        addAndMakeVisible (pad);
        padComponents.add (pad);
    }

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

    processorRef.getPresetManager().onPresetLoaded = [this] (const DkitPreset& kit)
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

    g.setColour (DarkLookAndFeel::bgMedium);
    g.fillRect (0, 0, getWidth(), 50);

    g.setColour (DarkLookAndFeel::accent.withAlpha (0.5f));
    g.fillRect (0, 49, getWidth(), 2);
}

void MPSDrumMachineEditor::resized()
{
    auto area = getLocalBounds();

    auto topBar = area.removeFromTop (50).reduced (10, 8);
    prevButton.setBounds (topBar.removeFromLeft (36));
    topBar.removeFromLeft (5);
    nextButton.setBounds (topBar.removeFromLeft (36));
    settingsButton.setBounds (topBar.removeFromRight (90));
    topBar.removeFromRight (5);
    presetsViewButton.setBounds (topBar.removeFromRight (90));
    topBar.removeFromRight (5);
    samplesViewButton.setBounds (topBar.removeFromRight (90));
    topBar.removeFromRight (10);
    presetLabel.setBounds (topBar);

    if (showingSampleBrowser && sampleBrowser != nullptr)
        sampleBrowser->setBounds (area.removeFromLeft (250));

    area.reduce (15, 10);

    if (showingPresetList)
    {
        if (presetListComponent != nullptr)
            presetListComponent->setBounds (area);
    }
    else
    {
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

void MPSDrumMachineEditor::toggleSampleBrowser()
{
    showingSampleBrowser = ! showingSampleBrowser;

    if (showingSampleBrowser)
    {
        if (sampleBrowser != nullptr)
        {
            sampleBrowser->setSamplesDirectory (processorRef.getSamplesPath());
            sampleBrowser->setVisible (true);
        }
    }
    else
    {
        if (sampleBrowser != nullptr)
            sampleBrowser->setVisible (false);
    }

    resized();
}

void MPSDrumMachineEditor::showSampleInBrowser (const juce::File& file)
{
    if (! showingSampleBrowser)
    {
        showingSampleBrowser = true;
        if (sampleBrowser != nullptr)
        {
            sampleBrowser->setSamplesDirectory (processorRef.getSamplesPath());
            sampleBrowser->setVisible (true);
        }
        resized();
    }

    if (sampleBrowser != nullptr)
        sampleBrowser->revealFile (file);
}

bool MPSDrumMachineEditor::shouldDropFilesWhenDraggedExternally (
    const juce::DragAndDropTarget::SourceDetails& details,
    juce::StringArray& files, bool& canMoveFiles)
{
    auto desc = details.description.toString();
    if (desc.startsWith (SampleBrowserComponent::dragSourceId + ":"))
    {
        auto filePath = desc.fromFirstOccurrenceOf (":", false, false);
        files.add (filePath);
        canMoveFiles = false;
        return true;
    }
    return false;
}
