#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <atomic>
#include <mutex>

class SampleEngine
{
public:
    SampleEngine();

    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();

    void loadSample (int midiNote, const juce::File& file);
    void clearSample (int midiNote);
    void swapSamples (int noteA, int noteB);
    bool hasSample (int midiNote) const;
    juce::String getSampleName (int midiNote) const;
    juce::File getSampleFile (int midiNote) const;

    void noteOn (int midiNote, float velocity);
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);

    void clearAllSamples();

    void setPadVolume (int midiNote, float volume);
    float getPadVolume (int midiNote) const;

    void markSampleMissing (int midiNote, const juce::String& name);
    bool isSampleMissing (int midiNote) const;

    void previewSample (const juce::File& file);
    void stopPreview();

    static constexpr int kPreviewSlot = 0;

private:
    static constexpr int kMaxVoicesPerPad = 8;
    static constexpr int kTotalSlots = 128;

    struct Voice
    {
        std::atomic<bool> active { false };
        int position = 0;
        float velocity = 1.0f;
    };

    struct SampleSlot
    {
        juce::AudioBuffer<float> buffer;
        juce::String sampleName;
        juce::File sampleFile;
        bool loaded = false;
        bool missing = false;
        float volume = 1.0f;
        std::array<Voice, kMaxVoicesPerPad> voices;
    };

    std::array<SampleSlot, kTotalSlots> slots;
    juce::AudioFormatManager formatManager;
    double currentSampleRate = 44100.0;
    std::mutex loadMutex;
};
