#include "MidiMapper.h"
#include "DrumKitLibrary.h"

MidiMapper::MidiMapper()
{
    activeKit = &DrumKitLibrary::getDefaultKit();
}

void MidiMapper::setActiveKit (const juce::String& kitId)
{
    auto* kit = DrumKitLibrary::findKit (kitId);
    if (kit != nullptr)
        activeKit = kit;
}

juce::String MidiMapper::getActiveKitId() const
{
    if (activeKit != nullptr)
        return activeKit->id;
    return {};
}

const std::vector<PadInfo>& MidiMapper::getAllPads() const
{
    jassert (activeKit != nullptr);
    return activeKit->pads;
}

bool MidiMapper::isPadNote (int midiNote) const
{
    for (auto& p : activeKit->pads)
        if (p.midiNote == midiNote)
            return true;
    return false;
}

const PadInfo* MidiMapper::getPadInfo (int midiNote) const
{
    for (auto& p : activeKit->pads)
        if (p.midiNote == midiNote)
            return &p;
    return nullptr;
}

void MidiMapper::setNavChannel (int channel)
{
    navChannel = channel;
}

void MidiMapper::setPrevCCNumber (int cc)
{
    prevCCNumber = cc;
}

void MidiMapper::setNextCCNumber (int cc)
{
    nextCCNumber = cc;
}

void MidiMapper::startLearn (LearnTarget target)
{
    learnTarget.store (static_cast<int> (target));
}

void MidiMapper::cancelLearn()
{
    learnTarget.store (0);
}

bool MidiMapper::isLearning() const
{
    return learnTarget.load() != 0;
}

MidiMapper::LearnTarget MidiMapper::getLearnTarget() const
{
    return static_cast<LearnTarget> (learnTarget.load());
}

bool MidiMapper::processForLearn (const juce::MidiMessage& msg)
{
    int target = learnTarget.load();
    if (target == 0)
        return false;

    if (! msg.isController())
        return false;

    int value = msg.getControllerValue();
    if (value == 0)
        return false;

    int cc = msg.getControllerNumber();
    auto learned = static_cast<LearnTarget> (target);

    if (learned == LearnTarget::Prev)
        prevCCNumber = cc;
    else if (learned == LearnTarget::Next)
        nextCCNumber = cc;

    learnTarget.store (0);

    if (onLearnComplete)
    {
        auto callback = onLearnComplete;
        juce::MessageManager::callAsync ([callback, learned, cc] { callback (learned, cc); });
    }

    return true;
}

MidiMapper::NavAction MidiMapper::processForNavigation (const juce::MidiMessage& msg) const
{
    if (! msg.isController())
        return NavAction::None;

    if (navChannel > 0 && msg.getChannel() != navChannel)
        return NavAction::None;

    int cc = msg.getControllerNumber();
    int value = msg.getControllerValue();

    if (value == 0)
        return NavAction::None;

    if (cc == prevCCNumber)
        return NavAction::Previous;

    if (cc == nextCCNumber)
        return NavAction::Next;

    return NavAction::None;
}

bool MidiMapper::isDrumTrigger (const juce::MidiMessage& msg) const
{
    if (! msg.isNoteOn())
        return false;
    return isPadNote (msg.getNoteNumber());
}
