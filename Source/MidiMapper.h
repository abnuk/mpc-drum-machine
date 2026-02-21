#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <functional>
#include <string>
#include <vector>

struct PadInfo
{
    int midiNote;
    std::string padName;
    std::string triggerName;
};

struct DrumKitDefinition;

class MidiMapper
{
public:
    MidiMapper();

    bool isPadNote (int midiNote) const;
    const PadInfo* getPadInfo (int midiNote) const;

    const std::vector<PadInfo>& getAllPads() const;
    const DrumKitDefinition* getActiveKit() const { return activeKit; }

    void setActiveKit (const juce::String& kitId);
    juce::String getActiveKitId() const;

    // Navigation MIDI config
    void setNavChannel (int channel);
    void setPrevCCNumber (int cc);
    void setNextCCNumber (int cc);
    int getNavChannel() const { return navChannel; }
    int getPrevCCNumber() const { return prevCCNumber; }
    int getNextCCNumber() const { return nextCCNumber; }

    enum class NavAction { None, Next, Previous };

    NavAction processForNavigation (const juce::MidiMessage& msg) const;
    bool isDrumTrigger (const juce::MidiMessage& msg) const;

    // MIDI Learn
    enum class LearnTarget { None, Prev, Next };

    void startLearn (LearnTarget target);
    void cancelLearn();
    bool isLearning() const;
    LearnTarget getLearnTarget() const;

    bool processForLearn (const juce::MidiMessage& msg);

    std::function<void (LearnTarget, int)> onLearnComplete;

private:
    const DrumKitDefinition* activeKit = nullptr;

    int navChannel = 0;
    int prevCCNumber = 1;
    int nextCCNumber = 2;

    std::atomic<int> learnTarget { 0 };
};
