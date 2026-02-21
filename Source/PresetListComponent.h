#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PresetManager.h"
#include "LookAndFeel.h"

class PresetListContent : public juce::Component
{
public:
    PresetListContent();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

    void setPresetNames (const juce::StringArray& names);
    void setActiveIndex (int index);
    int getActiveIndex() const { return activeIndex; }

    int getFirstIndexForLetter (const juce::String& letter) const;

    std::function<void(int)> onPresetClicked;
    std::function<void(int)> onDeleteRequested;
    std::function<void(int, const juce::String&)> onRenameRequested;

    static constexpr int rowHeight = 40;

private:
    juce::StringArray presetNames;
    int activeIndex = -1;

    void showContextMenu (int rowIndex);
    void showRenameDialog (int rowIndex);
    void showDeleteConfirmation (int rowIndex);
};

class AlphabetBarComponent : public juce::Component
{
public:
    AlphabetBarComponent();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

    std::function<void(const juce::String&)> onLetterClicked;

private:
    juce::StringArray letters;
};

class PresetListComponent : public juce::Component
{
public:
    PresetListComponent (PresetManager& pm);

    void resized() override;
    void paint (juce::Graphics& g) override;

    void refreshPresetList();
    void setActivePreset (int index);

    std::function<void(int)> onPresetSelected;
    std::function<void(int)> onPresetDeleted;
    std::function<void(int, const juce::String&)> onPresetRenamed;
    std::function<void()> onSaveNewPreset;

private:
    PresetManager& presetManager;

    AlphabetBarComponent alphabetBar;
    juce::Viewport viewport;
    PresetListContent listContent;

    juce::TextButton upButton { "UP" };
    juce::TextButton downButton { "DOWN" };
    juce::TextButton addButton { "+" };

    void scrollPageUp();
    void scrollPageDown();
    void ensureActiveVisible();
    void scrollToLetter (const juce::String& letter);
};
