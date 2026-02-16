#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <fstream>

MPSDrumMachineProcessor::MPSDrumMachineProcessor()
    : AudioProcessor (BusesProperties()
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      presetManager (adgParser)
{
    {
        auto logFile = juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                           .getChildFile ("mps_drum_debug.log");
        logFile.replaceWithText ("=== MPS Drum Machine started ===\n");
    }

    // Set up default Ableton library scan paths
    auto abletonLib = AdgParser::autoDetectAbletonLibrary();
    if (abletonLib.isDirectory())
    {
        adgParser.setAbletonLibraryPath (abletonLib);

        // Scan all known Ableton drum rack preset locations
        juce::StringArray drumRackPaths = {
            "Racks/Drum Racks",
            "Presets/Instruments/Drum Rack",
            "Defaults/Slicing"
        };

        for (auto& subPath : drumRackPaths)
        {
            auto dir = abletonLib.getChildFile (subPath);
            if (dir.isDirectory())
                presetManager.addScanDirectory (dir);
        }
    }

    // Add user library (multiple possible structures)
    auto userMusicDir = juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    juce::StringArray userLibPaths = {
        "Ableton/User Library/Presets/Instruments/Drum Rack",
        "Ableton/User Library/Racks/Drum Racks"
    };

    for (auto& subPath : userLibPaths)
    {
        auto dir = userMusicDir.getChildFile (subPath);
        if (dir.isDirectory())
            presetManager.addScanDirectory (dir);
    }

    // Set up preset loaded callback
    presetManager.onPresetLoaded = [this] (const AdgDrumKit& kit)
    {
        loadKitSamples (kit);
    };

    // Scan for presets on a background thread
    juce::Thread::launch ([this] { presetManager.scanForPresets(); });
}

MPSDrumMachineProcessor::~MPSDrumMachineProcessor() {}

void MPSDrumMachineProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleEngine.prepareToPlay (sampleRate, samplesPerBlock);
}

void MPSDrumMachineProcessor::releaseResources()
{
    sampleEngine.releaseResources();
}

bool MPSDrumMachineProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void MPSDrumMachineProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();

        // Check for navigation MIDI
        auto navAction = midiMapper.processForNavigation (msg);
        if (navAction == MidiMapper::NavAction::Next)
        {
            // Must happen on message thread for thread safety with preset loading
            juce::MessageManager::callAsync ([this] { presetManager.loadNextPreset(); });
            continue;
        }
        if (navAction == MidiMapper::NavAction::Previous)
        {
            juce::MessageManager::callAsync ([this] { presetManager.loadPreviousPreset(); });
            continue;
        }

        // Check for drum triggers
        if (midiMapper.isDrumTrigger (msg))
        {
            int note = msg.getNoteNumber();
            float velocity = msg.getFloatVelocity();
            sampleEngine.noteOn (note, velocity);

            if (onMidiTrigger)
                onMidiTrigger (note, velocity);
        }
    }

    sampleEngine.renderNextBlock (buffer, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* MPSDrumMachineProcessor::createEditor()
{
    return new MPSDrumMachineEditor (*this);
}

void MPSDrumMachineProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = std::make_unique<juce::XmlElement> ("MPSDrumMachineState");

    // Save Ableton library path
    state->setAttribute ("abletonLibPath", adgParser.getAbletonLibraryPath().getFullPathName());

    // Save navigation MIDI settings
    state->setAttribute ("navChannel", midiMapper.getNavChannel());
    state->setAttribute ("prevCC", midiMapper.getPrevCCNumber());
    state->setAttribute ("nextCC", midiMapper.getNextCCNumber());

    // Save current preset index
    state->setAttribute ("presetIndex", presetManager.getCurrentPresetIndex());

    // Save custom pad mappings (for drag & drop overrides)
    auto* padsEl = state->createNewChildElement ("PadMappings");
    for (auto& pad : MidiMapper::getAllPads())
    {
        auto file = sampleEngine.getSampleFile (pad.midiNote);
        if (file.existsAsFile())
        {
            auto* padEl = padsEl->createNewChildElement ("Pad");
            padEl->setAttribute ("note", pad.midiNote);
            padEl->setAttribute ("file", file.getFullPathName());
        }
    }

    copyXmlToBinary (*state, destData);
}

void MPSDrumMachineProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto state = getXmlFromBinary (data, sizeInBytes);
    if (state == nullptr || ! state->hasTagName ("MPSDrumMachineState"))
        return;

    // Restore Ableton library path
    auto libPath = state->getStringAttribute ("abletonLibPath");
    if (libPath.isNotEmpty())
        adgParser.setAbletonLibraryPath (juce::File (libPath));

    // Restore navigation settings
    midiMapper.setNavChannel (state->getIntAttribute ("navChannel", 0));
    midiMapper.setPrevCCNumber (state->getIntAttribute ("prevCC",
        state->getIntAttribute ("navCC", 1)));
    midiMapper.setNextCCNumber (state->getIntAttribute ("nextCC", 2));

    // Restore pad mappings
    auto* padsEl = state->getChildByName ("PadMappings");
    if (padsEl != nullptr)
    {
        for (auto* padEl : padsEl->getChildIterator())
        {
            int note = padEl->getIntAttribute ("note", -1);
            auto filePath = padEl->getStringAttribute ("file");
            if (note >= 0 && filePath.isNotEmpty())
            {
                juce::File file (filePath);
                if (file.existsAsFile())
                    sampleEngine.loadSample (note, file);
            }
        }
    }

    // Restore preset
    int presetIdx = state->getIntAttribute ("presetIndex", -1);
    if (presetIdx >= 0)
    {
        juce::MessageManager::callAsync ([this, presetIdx]
        {
            presetManager.scanForPresets();
            presetManager.loadPreset (presetIdx);
        });
    }
}

void MPSDrumMachineProcessor::loadKitSamples (const AdgDrumKit& kit)
{
    {
        auto lf = juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                      .getChildFile ("mps_drum_debug.log");
        juce::String logStr;
        logStr << "loadKitSamples: kit=" << kit.kitName
               << " mappings=" << (int) kit.mappings.size() << "\n";
        for (auto& m : kit.mappings)
        {
            juce::File sf (m.samplePath);
            logStr << "  note=" << m.midiNote << " path=" << m.samplePath
                   << " exists=" << (sf.existsAsFile() ? "YES" : "NO") << "\n";
        }
        lf.appendText (logStr);
    }

    sampleEngine.clearAllSamples();

    // Check if a custom mapping overlay exists for this preset
    auto presetId = PadMappingManager::makePresetId (kit.sourceFile);
    auto customMapping = padMappingManager.loadMapping (presetId);

    if (customMapping.has_value())
    {
        for (auto& [note, file] : customMapping.value())
        {
            if (file.existsAsFile())
                sampleEngine.loadSample (note, file);
        }
    }
    else
    {
        for (auto& mapping : kit.mappings)
        {
            juce::File sampleFile (mapping.samplePath);
            if (sampleFile.existsAsFile())
                sampleEngine.loadSample (mapping.midiNote, sampleFile);
        }
    }
}

void MPSDrumMachineProcessor::swapPadsAndSave (int noteA, int noteB)
{
    sampleEngine.swapSamples (noteA, noteB);
    saveCurrentMappingOverlay();
}

void MPSDrumMachineProcessor::saveCurrentMappingOverlay()
{
    auto& kit = presetManager.getCurrentKit();
    if (kit.sourceFile == juce::File())
        return;

    auto presetId = PadMappingManager::makePresetId (kit.sourceFile);

    PadMappingManager::PadMapping mapping;
    for (auto& pad : MidiMapper::getAllPads())
    {
        auto file = sampleEngine.getSampleFile (pad.midiNote);
        if (file.existsAsFile())
            mapping[pad.midiNote] = file;
    }

    padMappingManager.saveMapping (presetId, mapping);
}

void MPSDrumMachineProcessor::resetCurrentMappingToDefault()
{
    auto& kit = presetManager.getCurrentKit();
    if (kit.sourceFile == juce::File())
        return;

    auto presetId = PadMappingManager::makePresetId (kit.sourceFile);
    padMappingManager.clearMapping (presetId);

    // Reload the original kit samples
    sampleEngine.clearAllSamples();
    for (auto& mapping : kit.mappings)
    {
        juce::File sampleFile (mapping.samplePath);
        if (sampleFile.existsAsFile())
            sampleEngine.loadSample (mapping.midiNote, sampleFile);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MPSDrumMachineProcessor();
}
