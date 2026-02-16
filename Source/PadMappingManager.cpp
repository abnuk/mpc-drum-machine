#include "PadMappingManager.h"

PadMappingManager::PadMappingManager() {}

juce::String PadMappingManager::makePresetId (const juce::File& presetFile)
{
    return juce::String (presetFile.getFullPathName().hashCode64());
}

juce::File PadMappingManager::getMappingsDir() const
{
    auto appData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    return appData.getChildFile ("MPSDrumMachine/PadMappings");
}

juce::File PadMappingManager::getMappingFile (const juce::String& presetId) const
{
    return getMappingsDir().getChildFile (presetId + ".json");
}

void PadMappingManager::saveMapping (const juce::String& presetId, const PadMapping& mapping)
{
    auto dir = getMappingsDir();
    dir.createDirectory();

    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty ("presetId", presetId);

    juce::Array<juce::var> padsArray;
    for (auto& [note, sampleFile] : mapping)
    {
        juce::DynamicObject::Ptr pad = new juce::DynamicObject();
        pad->setProperty ("midiNote", note);
        pad->setProperty ("samplePath", sampleFile.getFullPathName());
        padsArray.add (juce::var (pad.get()));
    }

    root->setProperty ("pads", padsArray);

    auto jsonText = juce::JSON::toString (juce::var (root.get()));
    getMappingFile (presetId).replaceWithText (jsonText);
}

std::optional<PadMappingManager::PadMapping> PadMappingManager::loadMapping (const juce::String& presetId) const
{
    auto file = getMappingFile (presetId);
    if (! file.existsAsFile())
        return std::nullopt;

    auto jsonText = file.loadFileAsString();
    auto parsed = juce::JSON::parse (jsonText);

    if (! parsed.isObject())
        return std::nullopt;

    PadMapping mapping;
    auto padsArray = parsed.getProperty ("pads", juce::var());
    if (padsArray.isArray())
    {
        for (int i = 0; i < padsArray.size(); ++i)
        {
            auto padVar = padsArray[i];
            int note = (int) padVar.getProperty ("midiNote", -1);
            auto path = padVar.getProperty ("samplePath", "").toString();
            if (note >= 0 && path.isNotEmpty())
            {
                juce::File f (path);
                if (f.existsAsFile())
                    mapping[note] = f;
            }
        }
    }

    if (mapping.empty())
        return std::nullopt;

    return mapping;
}

bool PadMappingManager::hasCustomMapping (const juce::String& presetId) const
{
    return getMappingFile (presetId).existsAsFile();
}

void PadMappingManager::clearMapping (const juce::String& presetId)
{
    getMappingFile (presetId).deleteFile();
}
