#include "PluginProcessor.h"
#include "PluginEditor.h"

BeatwerkProcessor::BeatwerkProcessor()
    : AudioProcessor (BusesProperties()
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    presetManager.onPresetLoaded = [this] (const DkitPreset& kit)
    {
        loadKitSamples (kit);
    };

    juce::Thread::launch ([this] { presetManager.scanForPresets(); });
}

BeatwerkProcessor::~BeatwerkProcessor() {}

void BeatwerkProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleEngine.prepareToPlay (sampleRate, samplesPerBlock);
}

void BeatwerkProcessor::releaseResources()
{
    sampleEngine.releaseResources();
}

bool BeatwerkProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void BeatwerkProcessor::processBlock (juce::AudioBuffer<float>& buffer,
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

juce::AudioProcessorEditor* BeatwerkProcessor::createEditor()
{
    return new BeatwerkEditor (*this);
}

void BeatwerkProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = std::make_unique<juce::XmlElement> ("BeatwerkState");

    state->setAttribute ("samplesPath", presetManager.getSamplesDir().getFullPathName());
    state->setAttribute ("presetsPath", presetManager.getPresetsDir().getFullPathName());

    state->setAttribute ("navChannel", midiMapper.getNavChannel());
    state->setAttribute ("prevCC", midiMapper.getPrevCCNumber());
    state->setAttribute ("nextCC", midiMapper.getNextCCNumber());

    state->setAttribute ("drumKit", midiMapper.getActiveKitId());
    state->setAttribute ("presetIndex", presetManager.getCurrentPresetIndex());

    auto* padsEl = state->createNewChildElement ("PadMappings");
    for (auto& pad : midiMapper.getAllPads())
    {
        auto file = sampleEngine.getSampleFile (pad.midiNote);
        if (file.existsAsFile())
        {
            auto* padEl = padsEl->createNewChildElement ("Pad");
            padEl->setAttribute ("note", pad.midiNote);
            padEl->setAttribute ("file", file.getFullPathName());

            float vol = sampleEngine.getPadVolume (pad.midiNote);
            if (std::abs (vol - 1.0f) > 0.001f)
                padEl->setAttribute ("volume", (double) vol);
        }
    }

    copyXmlToBinary (*state, destData);
}

void BeatwerkProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto state = getXmlFromBinary (data, sizeInBytes);
    if (state == nullptr || ! state->hasTagName ("BeatwerkState"))
        return;

    auto samplesPath = state->getStringAttribute ("samplesPath");
    if (samplesPath.isNotEmpty())
        presetManager.setSamplesDir (juce::File (samplesPath));

    auto presetsPath = state->getStringAttribute ("presetsPath");
    if (presetsPath.isNotEmpty())
        presetManager.setPresetsDir (juce::File (presetsPath));

    auto drumKitId = state->getStringAttribute ("drumKit");
    if (drumKitId.isNotEmpty())
        midiMapper.setActiveKit (drumKitId);
    else
        midiMapper.setActiveKit ("millenium_mps_1000");

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

            if (note >= 0 && padEl->hasAttribute ("volume"))
                sampleEngine.setPadVolume (note, (float) padEl->getDoubleAttribute ("volume", 1.0));
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

void BeatwerkProcessor::loadKitSamples (const DkitPreset& kit)
{
    sampleEngine.clearAllSamples();

    auto presetId = PadMappingManager::makePresetId (kit.sourceFile);
    auto customMapping = padMappingManager.loadMapping (presetId);

    if (customMapping.has_value())
    {
        for (auto& [note, file] : customMapping->pads)
        {
            if (file.existsAsFile())
                sampleEngine.loadSample (note, file);
        }

        for (auto& [note, vol] : customMapping->volumes)
            sampleEngine.setPadVolume (note, vol);
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

void BeatwerkProcessor::setSamplesPath (const juce::File& path)
{
    presetManager.setSamplesDir (path);
}

void BeatwerkProcessor::setPresetsPath (const juce::File& path)
{
    presetManager.setPresetsDir (path);
    presetManager.scanForPresets();
}

void BeatwerkProcessor::swapPadsAndSave (int noteA, int noteB)
{
    sampleEngine.swapSamples (noteA, noteB);
    saveCurrentMappingOverlay();
}

void BeatwerkProcessor::saveCurrentMappingOverlay()
{
    auto& kit = presetManager.getCurrentKit();
    if (kit.sourceFile == juce::File())
        return;

    auto presetId = PadMappingManager::makePresetId (kit.sourceFile);

    PadMappingManager::PadMapping mapping;
    PadMappingManager::VolumeMap volumes;
    for (auto& pad : midiMapper.getAllPads())
    {
        auto file = sampleEngine.getSampleFile (pad.midiNote);
        if (file.existsAsFile())
            mapping[pad.midiNote] = file;

        float vol = sampleEngine.getPadVolume (pad.midiNote);
        if (std::abs (vol - 1.0f) > 0.001f)
            volumes[pad.midiNote] = vol;
    }

    padMappingManager.saveMapping (presetId, mapping, volumes);
}

void BeatwerkProcessor::resetCurrentMappingToDefault()
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

    for (auto& pad : midiMapper.getAllPads())
        sampleEngine.setPadVolume (pad.midiNote, 1.0f);
}

void BeatwerkProcessor::setActiveKit (const juce::String& kitId)
{
    midiMapper.setActiveKit (kitId);

    if (onKitChanged)
        onKitChanged();
}

juce::String BeatwerkProcessor::getActiveKitId() const
{
    return midiMapper.getActiveKitId();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BeatwerkProcessor();
}
