#include "PadComponent.h"

const juce::String PadComponent::dragSourceId = "MPSPadDrag";

PadComponent::PadComponent (const PadInfo& info, SampleEngine& engine)
    : padInfo (info), sampleEngine (engine)
{
    updateSampleDisplay();
}

void PadComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);

    juce::Colour bgColour = DarkLookAndFeel::padDefault;
    if (sampleMissing)
        bgColour = DarkLookAndFeel::padMissing;
    else if (sampleEngine.hasSample (padInfo.midiNote))
        bgColour = DarkLookAndFeel::padLoaded;
    if (isDragOver)
        bgColour = DarkLookAndFeel::padHover;
    if (isDragging)
        bgColour = DarkLookAndFeel::padLoaded.darker (0.5f);

    g.setColour (bgColour);
    g.fillRoundedRectangle (bounds, 6.0f);

    if (flashAlpha > 0.01f)
    {
        g.setColour (DarkLookAndFeel::triggerFlash.withAlpha (flashAlpha * flashVelocity));
        g.fillRoundedRectangle (bounds, 6.0f);
    }

    auto borderColour = isDragOver ? DarkLookAndFeel::accent
                                   : (sampleMissing ? juce::Colour (0xffaa4444)
                                                    : DarkLookAndFeel::bgLight.brighter (0.3f));
    g.setColour (borderColour);
    g.drawRoundedRectangle (bounds, 6.0f, isDragOver ? 2.0f : 1.5f);

    // Exclamation badge for missing samples
    if (sampleMissing)
    {
        float badgeSize = 18.0f;
        float badgeX = bounds.getRight() - badgeSize - 4.0f;
        float badgeY = bounds.getY() + 4.0f;
        g.setColour (juce::Colour (0xffcc3333));
        g.fillEllipse (badgeX, badgeY, badgeSize, badgeSize);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("!", juce::Rectangle<float> (badgeX, badgeY, badgeSize, badgeSize).toNearestInt(),
                    juce::Justification::centred);
    }

    g.setColour (isDragging ? DarkLookAndFeel::textBright.withAlpha (0.4f)
                            : DarkLookAndFeel::textBright);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    auto textArea = bounds.reduced (6.0f);
    g.drawText (juce::String (padInfo.padName), textArea.removeFromTop (18.0f),
                juce::Justification::centredLeft);

    g.setColour (isDragging ? DarkLookAndFeel::accent.withAlpha (0.4f)
                            : DarkLookAndFeel::accent);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (juce::String (padInfo.triggerName), textArea.removeFromTop (15.0f),
                juce::Justification::centredLeft);

    g.setColour (DarkLookAndFeel::textDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("MIDI " + juce::String (padInfo.midiNote),
                textArea.removeFromTop (13.0f), juce::Justification::centredLeft);

    if (sampleName.isNotEmpty())
    {
        if (sampleMissing)
            g.setColour (juce::Colour (0xffcc6666));
        else if (isDragging)
            g.setColour (DarkLookAndFeel::textBright.withAlpha (0.3f));
        else
            g.setColour (DarkLookAndFeel::textBright.withAlpha (0.8f));

        g.setFont (juce::FontOptions (10.0f));
        g.drawFittedText (sampleName, textArea.toNearestInt(),
                          juce::Justification::centredLeft, 2);
    }
    else
    {
        g.setColour (DarkLookAndFeel::textDim.withAlpha (0.4f));
        g.setFont (juce::FontOptions (9.0f));
        g.drawText ("Drop sample here", textArea, juce::Justification::centred);
    }
}

void PadComponent::resized() {}

void PadComponent::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Reset Kit to Default", onResetMapping != nullptr);
        menu.showMenuAsync (juce::PopupMenu::Options(),
            [this] (int result)
            {
                if (result == 1 && onResetMapping)
                    onResetMapping();
            });
        return;
    }

    dragStartPos = event.getPosition();
}

void PadComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (isDragging || event.mods.isPopupMenu())
        return;

    if (! sampleEngine.hasSample (padInfo.midiNote))
        return;

    auto distance = event.getPosition().getDistanceFrom (dragStartPos);
    if (distance < dragThreshold)
        return;

    isDragging = true;
    repaint();

    auto dragImage = createComponentSnapshot (getLocalBounds())
                         .rescaled (getWidth() / 2, getHeight() / 2);
    dragImage.multiplyAllAlphas (0.6f);

    auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this);
    if (container != nullptr)
    {
        container->startDragging (dragSourceId + ":" + juce::String (padInfo.midiNote),
                                  this, juce::ScaledImage (dragImage), true);
    }
}

void PadComponent::mouseUp (const juce::MouseEvent& event)
{
    if (isDragging)
    {
        isDragging = false;
        repaint();
        return;
    }

    if (event.mods.isPopupMenu())
        return;

    if (sampleEngine.hasSample (padInfo.midiNote))
    {
        sampleEngine.noteOn (padInfo.midiNote, 0.7f);
        triggerFlash (0.7f);
    }
}

bool PadComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
    {
        auto ext = juce::File (f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3")
            return true;
    }
    return false;
}

void PadComponent::fileDragEnter (const juce::StringArray& /*files*/, int, int)
{
    isDragOver = true;
    repaint();
}

void PadComponent::fileDragExit (const juce::StringArray& /*files*/)
{
    isDragOver = false;
    repaint();
}

void PadComponent::filesDropped (const juce::StringArray& files, int, int)
{
    isDragOver = false;

    for (auto& f : files)
    {
        juce::File file (f);
        auto ext = file.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3")
        {
            sampleEngine.loadSample (padInfo.midiNote, file);
            updateSampleDisplay();

            if (onSampleDropped)
                onSampleDropped (padInfo.midiNote, file);
            break;
        }
    }

    repaint();
}

bool PadComponent::isInterestedInDragSource (const SourceDetails& details)
{
    auto desc = details.description.toString();
    if (! desc.startsWith (dragSourceId + ":"))
        return false;

    int sourceNote = desc.fromLastOccurrenceOf (":", false, false).getIntValue();
    return sourceNote != padInfo.midiNote;
}

void PadComponent::itemDragEnter (const SourceDetails& /*details*/)
{
    isDragOver = true;
    repaint();
}

void PadComponent::itemDragExit (const SourceDetails& /*details*/)
{
    isDragOver = false;
    repaint();
}

void PadComponent::itemDropped (const SourceDetails& details)
{
    isDragOver = false;

    auto desc = details.description.toString();
    int sourceNote = desc.fromLastOccurrenceOf (":", false, false).getIntValue();

    if (sourceNote != padInfo.midiNote && onPadSwapped)
        onPadSwapped (sourceNote, padInfo.midiNote);

    if (auto* sourcePad = dynamic_cast<PadComponent*> (details.sourceComponent.get()))
    {
        sourcePad->isDragging = false;
        sourcePad->repaint();
    }

    repaint();
}

void PadComponent::timerCallback()
{
    flashAlpha -= 0.08f;
    if (flashAlpha <= 0.0f)
    {
        flashAlpha = 0.0f;
        stopTimer();
    }
    repaint();
}

void PadComponent::triggerFlash (float velocity)
{
    flashAlpha = 1.0f;
    flashVelocity = velocity;
    startTimerHz (30);
}

void PadComponent::updateSampleDisplay()
{
    sampleName = sampleEngine.getSampleName (padInfo.midiNote);
    sampleMissing = sampleEngine.isSampleMissing (padInfo.midiNote);
    repaint();
}
