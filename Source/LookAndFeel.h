#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class DarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DarkLookAndFeel();

    // Colours used throughout the UI
    static inline const juce::Colour bgDark       { 0xff1a1a2e };
    static inline const juce::Colour bgMedium     { 0xff252545 };
    static inline const juce::Colour bgLight      { 0xff353560 };
    static inline const juce::Colour accent        { 0xff5aafff };
    static inline const juce::Colour accentDim     { 0xff3a6abf };
    static inline const juce::Colour textBright    { 0xffffffff };
    static inline const juce::Colour textDim       { 0xffa0a0b8 };
    static inline const juce::Colour padDefault    { 0xff303058 };
    static inline const juce::Colour padHover      { 0xff404070 };
    static inline const juce::Colour padActive     { 0xff5050a0 };
    static inline const juce::Colour padLoaded     { 0xff2a5040 };
    static inline const juce::Colour triggerFlash  { 0xffff8844 };
    static inline const juce::Colour padMissing    { 0xff6b2a2a };

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool isMouseOver, bool isButtonDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool isMouseOver, bool isButtonDown) override;
};
