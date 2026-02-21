#include "PluginProcessor.h"
#include "PluginEditor.h"

MPSDrumMachineProcessor::MPSDrumMachineProcessor()
    : AudioProcessor (BusesProperties()
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    presetManager.onPresetLoaded = [this] (const DkitPreset& kit)
    {
        loadKitSamples (kit);
    };

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

        if (midiMapper.processForLearn (msg))
            continue;

        auto navAction = midiMapper.processForNavigation (msg);
        if (navAction == MidiMapper::NavAction::Next)
        {
            juce::MessageManager::callAsync ([this] { presetManager.loadNextPreset(); });
            continue;
        }
        if (navAction == MidiMapper::NavAction::Previous)
        {
            juce::MessageManager::callAsync ([this] { presetManager.loadPreviousPreset(); });
            continue;
        }

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

    state->setAttribute ("samplesPath", presetManager.getSamplesDir().getFullPathName());
    state->setAttribute ("presetsPath", presetManager.getPresetsDir().getFullPathName());

    state->setAttribute ("navChannel", midiMapper.getNavChannel());
    state->setAttribute ("prevCC", midiMapper.getPrevCCNumber());
    state->setAttribute ("nextCC", midiMapper.getNextCCNumber());

    state->setAttribute ("presetIndex", presetManager.getCurrentPresetIndex());

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

    auto samplesPath = state->getStringAttribute ("samplesPath");
    if (samplesPath.isNotEmpty())
        presetManager.setSamplesDir (juce::File (samplesPath));

    auto presetsPath = state->getStringAttribute ("presetsPath");
    if (presetsPath.isNotEmpty())
        presetManager.setPresetsDir (juce::File (presetsPath));

    midiMapper.setNavChannel (state->getIntAttribute ("navChannel", 0));
    midiMapper.setPrevCCNumber (state->getIntAttribute ("prevCC",
        state->getIntAttribute ("navCC", 1)));
    midiMapper.setNextCCNumber (state->getIntAttribute ("nextCC", 2));

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

void MPSDrumMachineProcessor::loadKitSamples (const DkitPreset& kit)
{
    sampleEngine.clearAllSamples();

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
        for (auto& pad : kit.pads)
        {
            auto sampleFile = presetManager.resolveSamplePath (pad.sampleFile);
            if (sampleFile.existsAsFile())
                sampleEngine.loadSample (pad.midiNote, sampleFile);
            else if (pad.sampleFile.isNotEmpty())
                sampleEngine.markSampleMissing (pad.midiNote, pad.sampleName);
        }
    }
}

void MPSDrumMachineProcessor::setSamplesPath (const juce::File& path)
{
    presetManager.setSamplesDir (path);
}

void MPSDrumMachineProcessor::setPresetsPath (const juce::File& path)
{
    presetManager.setPresetsDir (path);
    presetManager.scanForPresets();
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

    sampleEngine.clearAllSamples();
    for (auto& pad : kit.pads)
    {
        auto sampleFile = presetManager.resolveSamplePath (pad.sampleFile);
        if (sampleFile.existsAsFile())
            sampleEngine.loadSample (pad.midiNote, sampleFile);
        else if (pad.sampleFile.isNotEmpty())
            sampleEngine.markSampleMissing (pad.midiNote, pad.sampleName);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MPSDrumMachineProcessor();
}
