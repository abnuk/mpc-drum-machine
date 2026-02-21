#include "SampleEngine.h"

SampleEngine::SampleEngine()
{
    formatManager.registerBasicFormats();
}

void SampleEngine::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
}

void SampleEngine::releaseResources()
{
    for (auto& slot : slots)
        for (auto& voice : slot.voices)
            voice.active.store (false);
}

void SampleEngine::loadSample (int midiNote, const juce::File& file)
{
    if (midiNote < 0 || midiNote >= kTotalSlots)
        return;

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr)
        return;

    juce::AudioBuffer<float> newBuffer ((int) reader->numChannels, (int) reader->lengthInSamples);
    reader->read (&newBuffer, 0, (int) reader->lengthInSamples, 0, true, true);

    // Resample if needed
    if (reader->sampleRate != currentSampleRate && currentSampleRate > 0)
    {
        double ratio = currentSampleRate / reader->sampleRate;
        int newLength = (int) (newBuffer.getNumSamples() * ratio);
        juce::AudioBuffer<float> resampled (newBuffer.getNumChannels(), newLength);

        for (int ch = 0; ch < newBuffer.getNumChannels(); ++ch)
        {
            auto* src = newBuffer.getReadPointer (ch);
            auto* dst = resampled.getWritePointer (ch);
            for (int i = 0; i < newLength; ++i)
            {
                double srcPos = i / ratio;
                int idx = (int) srcPos;
                float frac = (float) (srcPos - idx);
                if (idx + 1 < newBuffer.getNumSamples())
                    dst[i] = src[idx] * (1.0f - frac) + src[idx + 1] * frac;
                else if (idx < newBuffer.getNumSamples())
                    dst[i] = src[idx];
                else
                    dst[i] = 0.0f;
            }
        }
        newBuffer = std::move (resampled);
    }

    {
        std::lock_guard<std::mutex> lock (loadMutex);
        auto& slot = slots[(size_t) midiNote];

        for (auto& voice : slot.voices)
            voice.active.store (false);

        slot.buffer = std::move (newBuffer);
        slot.sampleName = file.getFileNameWithoutExtension();
        slot.sampleFile = file;
        slot.loaded = true;
        slot.missing = false;
    }
}

void SampleEngine::clearSample (int midiNote)
{
    if (midiNote < 0 || midiNote >= kTotalSlots)
        return;

    std::lock_guard<std::mutex> lock (loadMutex);
    auto& slot = slots[(size_t) midiNote];
    for (auto& voice : slot.voices)
        voice.active.store (false);
    slot.buffer.setSize (0, 0);
    slot.sampleName.clear();
    slot.sampleFile = juce::File();
    slot.loaded = false;
    slot.missing = false;
    slot.volume = 1.0f;
}

void SampleEngine::swapSamples (int noteA, int noteB)
{
    if (noteA < 0 || noteA >= kTotalSlots || noteB < 0 || noteB >= kTotalSlots || noteA == noteB)
        return;

    std::lock_guard<std::mutex> lock (loadMutex);

    auto& slotA = slots[(size_t) noteA];
    auto& slotB = slots[(size_t) noteB];

    for (auto& v : slotA.voices)
        v.active.store (false);
    for (auto& v : slotB.voices)
        v.active.store (false);

    std::swap (slotA.buffer, slotB.buffer);
    std::swap (slotA.sampleName, slotB.sampleName);
    std::swap (slotA.sampleFile, slotB.sampleFile);
    std::swap (slotA.loaded, slotB.loaded);
    std::swap (slotA.missing, slotB.missing);
    std::swap (slotA.volume, slotB.volume);
}

bool SampleEngine::hasSample (int midiNote) const
{
    if (midiNote < 0 || midiNote >= kTotalSlots)
        return false;
    return slots[(size_t) midiNote].loaded;
}

juce::String SampleEngine::getSampleName (int midiNote) const
{
    if (midiNote < 0 || midiNote >= kTotalSlots)
        return {};
    return slots[(size_t) midiNote].sampleName;
}

juce::File SampleEngine::getSampleFile (int midiNote) const
{
    if (midiNote < 0 || midiNote >= kTotalSlots)
        return {};
    return slots[(size_t) midiNote].sampleFile;
}

void SampleEngine::setPadVolume (int midiNote, float volume)
{
    if (midiNote < 0 || midiNote >= kTotalSlots)
        return;
    slots[(size_t) midiNote].volume = juce::jlimit (0.0f, 2.0f, volume);
}

float SampleEngine::getPadVolume (int midiNote) const
{
    if (midiNote < 0 || midiNote >= kTotalSlots)
        return 1.0f;
    return slots[(size_t) midiNote].volume;
}

void SampleEngine::noteOn (int midiNote, float velocity)
{
    if (midiNote < 0 || midiNote >= kTotalSlots)
        return;

    auto& slot = slots[(size_t) midiNote];
    if (! slot.loaded)
        return;

    for (auto& voice : slot.voices)
    {
        if (! voice.active.load())
        {
            voice.position = 0;
            voice.velocity = velocity;
            voice.active.store (true);
            return;
        }
    }

    // Steal oldest voice (voice 0)
    slot.voices[0].position = 0;
    slot.voices[0].velocity = velocity;
    slot.voices[0].active.store (true);
}

void SampleEngine::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    for (auto& slot : slots)
    {
        if (! slot.loaded)
            continue;

        for (auto& voice : slot.voices)
        {
            if (! voice.active.load())
                continue;

            int samplesAvailable = slot.buffer.getNumSamples() - voice.position;
            int samplesToRender = juce::jmin (numSamples, samplesAvailable);

            if (samplesToRender <= 0)
            {
                voice.active.store (false);
                continue;
            }

            float gain = voice.velocity * slot.volume;
            int outChannels = outputBuffer.getNumChannels();
            int srcChannels = slot.buffer.getNumChannels();

            for (int ch = 0; ch < outChannels; ++ch)
            {
                int srcCh = juce::jmin (ch, srcChannels - 1);
                outputBuffer.addFrom (ch, startSample, slot.buffer,
                                      srcCh, voice.position, samplesToRender, gain);
            }

            voice.position += samplesToRender;
            if (voice.position >= slot.buffer.getNumSamples())
                voice.active.store (false);
        }
    }
}

void SampleEngine::clearAllSamples()
{
    for (int i = 0; i < kTotalSlots; ++i)
        clearSample (i);
}

void SampleEngine::markSampleMissing (int midiNote, const juce::String& name)
{
    if (midiNote < 0 || midiNote >= kTotalSlots)
        return;

    std::lock_guard<std::mutex> lock (loadMutex);
    auto& slot = slots[(size_t) midiNote];
    for (auto& voice : slot.voices)
        voice.active.store (false);
    slot.buffer.setSize (0, 0);
    slot.sampleName = name;
    slot.sampleFile = juce::File();
    slot.loaded = false;
    slot.missing = true;
}

bool SampleEngine::isSampleMissing (int midiNote) const
{
    if (midiNote < 0 || midiNote >= kTotalSlots)
        return false;
    return slots[(size_t) midiNote].missing;
}

void SampleEngine::previewSample (const juce::File& file)
{
    stopPreview();
    loadSample (kPreviewSlot, file);
    noteOn (kPreviewSlot, 0.8f);
}

void SampleEngine::stopPreview()
{
    auto& slot = slots[(size_t) kPreviewSlot];
    for (auto& voice : slot.voices)
        voice.active.store (false);
}
