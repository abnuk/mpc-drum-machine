#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "MidiMapper.h"
#include "SampleEngine.h"
#include "LookAndFeel.h"

class PadComponent : public juce::Component,
                     public juce::FileDragAndDropTarget,
                     public juce::DragAndDropTarget,
                     public juce::Timer
{
public:
    PadComponent (const PadInfo& padInfo, SampleEngine& engine);

    void paint (juce::Graphics& g) override;
    void resized() override;

    // Mouse handling for preview + drag initiation
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

    // FileDragAndDropTarget (external files from OS)
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    // DragAndDropTarget (internal pad-to-pad)
    bool isInterestedInDragSource (const SourceDetails& details) override;
    void itemDragEnter (const SourceDetails& details) override;
    void itemDragExit (const SourceDetails& details) override;
    void itemDropped (const SourceDetails& details) override;

    // Timer for flash animation
    void timerCallback() override;

    void triggerFlash (float velocity);
    void updateSampleDisplay();

    int getMidiNote() const { return padInfo.midiNote; }

    // Callback when a sample is dropped from external file
    std::function<void(int midiNote, const juce::File& file)> onSampleDropped;

    // Callback when pads are swapped via internal drag & drop
    std::function<void(int sourceNote, int targetNote)> onPadSwapped;

    // Callback when user requests reset to default mapping
    std::function<void()> onResetMapping;

    static const juce::String dragSourceId;

private:
    PadInfo padInfo;
    SampleEngine& sampleEngine;
    juce::String sampleName;
    bool isDragOver = false;
    bool isDragging = false;
    float flashAlpha = 0.0f;
    float flashVelocity = 0.0f;
    juce::Point<int> dragStartPos;

    static constexpr int dragThreshold = 5;
};
