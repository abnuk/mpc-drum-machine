#include "PresetListComponent.h"

//==============================================================================
// PresetListContent
//==============================================================================

PresetListContent::PresetListContent()
{
    setInterceptsMouseClicks (true, false);
}

void PresetListContent::paint (juce::Graphics& g)
{
    auto width = getWidth();

    for (int i = 0; i < presetNames.size(); ++i)
    {
        auto rowBounds = juce::Rectangle<int> (0, i * rowHeight, width, rowHeight);

        if (i == activeIndex)
        {
            g.setColour (DarkLookAndFeel::accent.withAlpha (0.3f));
            g.fillRect (rowBounds);
            g.setColour (DarkLookAndFeel::accent);
            g.fillRect (rowBounds.removeFromLeft (4));
            g.setColour (DarkLookAndFeel::textBright);
        }
        else
        {
            g.setColour (i % 2 == 0 ? DarkLookAndFeel::bgDark : DarkLookAndFeel::bgMedium);
            g.fillRect (rowBounds);
            g.setColour (DarkLookAndFeel::textDim);
        }

        auto textBounds = juce::Rectangle<int> (12, i * rowHeight, width - 24, rowHeight);
        g.setFont (juce::FontOptions (15.0f));
        g.drawText (juce::String (i + 1) + ".  " + presetNames[i],
                    textBounds, juce::Justification::centredLeft, true);
    }
}

void PresetListContent::mouseDown (const juce::MouseEvent& e)
{
    int clickedRow = e.getPosition().getY() / rowHeight;
    if (clickedRow < 0 || clickedRow >= presetNames.size())
        return;

    if (e.mods.isPopupMenu())
    {
        showContextMenu (clickedRow);
        return;
    }

    if (onPresetClicked)
        onPresetClicked (clickedRow);
}

void PresetListContent::showContextMenu (int rowIndex)
{
    juce::PopupMenu menu;
    menu.addItem (1, "Rename...");
    menu.addItem (2, "Delete");

    menu.showMenuAsync (juce::PopupMenu::Options(),
        [this, rowIndex] (int result)
        {
            if (result == 1)
                showRenameDialog (rowIndex);
            else if (result == 2)
                showDeleteConfirmation (rowIndex);
        });
}

void PresetListContent::showRenameDialog (int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= presetNames.size())
        return;

    auto currentName = presetNames[rowIndex];

    auto* alertWin = new juce::AlertWindow ("Rename Preset",
                                             "Enter a new name:",
                                             juce::MessageBoxIconType::QuestionIcon);
    alertWin->addTextEditor ("name", currentName);
    alertWin->addButton ("Rename", 1);
    alertWin->addButton ("Cancel", 0);

    alertWin->enterModalState (true, juce::ModalCallbackFunction::create (
        [this, alertWin, rowIndex] (int result)
        {
            if (result == 1)
            {
                auto newName = alertWin->getTextEditorContents ("name").trim();
                if (newName.isNotEmpty() && onRenameRequested)
                    onRenameRequested (rowIndex, newName);
            }
            delete alertWin;
        }), true);
}

void PresetListContent::showDeleteConfirmation (int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= presetNames.size())
        return;

    auto name = presetNames[rowIndex];

    auto* alertWin = new juce::AlertWindow ("Delete Preset",
                                             "Are you sure you want to delete \"" + name + "\"?",
                                             juce::MessageBoxIconType::WarningIcon);
    alertWin->addButton ("Delete", 1);
    alertWin->addButton ("Cancel", 0);

    alertWin->enterModalState (true, juce::ModalCallbackFunction::create (
        [this, alertWin, rowIndex] (int result)
        {
            if (result == 1)
            {
                if (onDeleteRequested)
                    onDeleteRequested (rowIndex);
            }
            delete alertWin;
        }), true);
}

void PresetListContent::setPresetNames (const juce::StringArray& names)
{
    presetNames = names;
    setSize (getWidth(), names.size() * rowHeight);
    repaint();
}

void PresetListContent::setActiveIndex (int index)
{
    activeIndex = index;
    repaint();
}

int PresetListContent::getFirstIndexForLetter (const juce::String& letter) const
{
    for (int i = 0; i < presetNames.size(); ++i)
    {
        auto firstChar = presetNames[i].trimStart().substring (0, 1).toUpperCase();

        if (letter == "#")
        {
            if (firstChar[0] >= '0' && firstChar[0] <= '9')
                return i;
        }
        else
        {
            if (firstChar == letter)
                return i;
        }
    }
    return -1;
}

//==============================================================================
// AlphabetBarComponent
//==============================================================================

AlphabetBarComponent::AlphabetBarComponent()
{
    letters.add ("#");
    for (char c = 'A'; c <= 'Z'; ++c)
        letters.add (juce::String::charToString (c));

    setInterceptsMouseClicks (true, false);
}

void AlphabetBarComponent::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::bgMedium);

    auto area = getLocalBounds();
    int numLetters = letters.size();
    float cellHeight = (float) area.getHeight() / (float) numLetters;

    for (int i = 0; i < numLetters; ++i)
    {
        auto cellBounds = juce::Rectangle<float> (0.0f, i * cellHeight,
                                                   (float) area.getWidth(), cellHeight);
        g.setColour (DarkLookAndFeel::textDim);
        g.setFont (juce::FontOptions (juce::jmin (cellHeight * 0.7f, 13.0f)));
        g.drawText (letters[i], cellBounds.toNearestInt(), juce::Justification::centred, false);
    }
}

void AlphabetBarComponent::mouseDown (const juce::MouseEvent& e)
{
    int numLetters = letters.size();
    float cellHeight = (float) getHeight() / (float) numLetters;
    int clickedIndex = (int) ((float) e.getPosition().getY() / cellHeight);

    if (clickedIndex >= 0 && clickedIndex < numLetters)
    {
        if (onLetterClicked)
            onLetterClicked (letters[clickedIndex]);
    }
}

//==============================================================================
// PresetListComponent
//==============================================================================

PresetListComponent::PresetListComponent (PresetManager& pm) : presetManager (pm)
{
    addAndMakeVisible (alphabetBar);
    alphabetBar.onLetterClicked = [this] (const juce::String& letter)
    {
        scrollToLetter (letter);
    };

    viewport.setViewedComponent (&listContent, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    listContent.onPresetClicked = [this] (int index)
    {
        if (onPresetSelected)
            onPresetSelected (index);
    };

    listContent.onDeleteRequested = [this] (int index)
    {
        if (onPresetDeleted)
            onPresetDeleted (index);
    };

    listContent.onRenameRequested = [this] (int index, const juce::String& newName)
    {
        if (onPresetRenamed)
            onPresetRenamed (index, newName);
    };

    upButton.onClick = [this] { scrollPageUp(); };
    addAndMakeVisible (upButton);

    downButton.onClick = [this] { scrollPageDown(); };
    addAndMakeVisible (downButton);

    addButton.onClick = [this]
    {
        if (onSaveNewPreset)
            onSaveNewPreset();
    };
    addAndMakeVisible (addButton);
}

void PresetListComponent::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::bgDark);
}

void PresetListComponent::resized()
{
    auto area = getLocalBounds();

    constexpr int buttonWidth = 70;
    auto rightStrip = area.removeFromRight (buttonWidth);
    addButton.setBounds (rightStrip.removeFromBottom (50).reduced (2));
    upButton.setBounds (rightStrip.removeFromTop (rightStrip.getHeight() / 2).reduced (2));
    downButton.setBounds (rightStrip.reduced (2));

    constexpr int alphabetWidth = 30;
    alphabetBar.setBounds (area.removeFromLeft (alphabetWidth));

    viewport.setBounds (area);
    listContent.setSize (area.getWidth(), listContent.getHeight());
}

void PresetListComponent::refreshPresetList()
{
    juce::StringArray names;
    for (int i = 0; i < presetManager.getNumPresets(); ++i)
        names.add (presetManager.getPresetName (i));

    listContent.setPresetNames (names);
    listContent.setActiveIndex (presetManager.getCurrentPresetIndex());
    listContent.setSize (viewport.getWidth(), names.size() * PresetListContent::rowHeight);
}

void PresetListComponent::setActivePreset (int index)
{
    listContent.setActiveIndex (index);
    ensureActiveVisible();
}

void PresetListComponent::scrollPageUp()
{
    auto pos = viewport.getViewPosition();
    int newY = juce::jmax (0, pos.getY() - viewport.getHeight());
    viewport.setViewPosition (0, newY);
}

void PresetListComponent::scrollPageDown()
{
    auto pos = viewport.getViewPosition();
    int maxY = listContent.getHeight() - viewport.getHeight();
    int newY = juce::jmin (maxY, pos.getY() + viewport.getHeight());
    viewport.setViewPosition (0, juce::jmax (0, newY));
}

void PresetListComponent::ensureActiveVisible()
{
    int index = listContent.getActiveIndex();
    if (index < 0)
        return;

    int rowTop = index * PresetListContent::rowHeight;
    int rowBottom = rowTop + PresetListContent::rowHeight;
    int viewTop = viewport.getViewPosition().getY();
    int viewBottom = viewTop + viewport.getHeight();

    if (rowTop < viewTop)
    {
        viewport.setViewPosition (0, juce::jmax (0, rowTop - PresetListContent::rowHeight));
    }
    else if (rowBottom > viewBottom)
    {
        int newY = rowBottom - viewport.getHeight() + PresetListContent::rowHeight;
        int maxY = listContent.getHeight() - viewport.getHeight();
        viewport.setViewPosition (0, juce::jmin (newY, juce::jmax (0, maxY)));
    }
}

void PresetListComponent::scrollToLetter (const juce::String& letter)
{
    int index = listContent.getFirstIndexForLetter (letter);
    if (index >= 0)
    {
        int y = index * PresetListContent::rowHeight;
        int maxY = juce::jmax (0, listContent.getHeight() - viewport.getHeight());
        viewport.setViewPosition (0, juce::jmin (y, maxY));
    }
}
