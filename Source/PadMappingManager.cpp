#include "PadMappingManager.h"

PadMappingManager::PadMappingManager() {}

juce::String PadMappingManager::makePresetId (const juce::File& presetFile)
{
    return juce::String (presetFile.getFullPathName().hashCode64());
}

juce::File PadMappingManager::getMappingsDir() const
{
    auto appData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    return appData.getChildFile ("Beatwerk/PadMappings");
}

juce::File PadMappingManager::getMappingFile (const juce::String& presetId) const
{
    return getMappingsDir().getChildFile (presetId + ".json");
}

void PadMappingManager::saveMapping (const juce::String& presetId, const PadMapping& mapping,
                                     const VolumeMap& volumes)
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

        auto volIt = volumes.find (note);
        if (volIt != volumes.end() && std::abs (volIt->second - 1.0f) > 0.001f)
            pad->setProperty ("volume", (double) volIt->second);

        padsArray.add (juce::var (pad.get()));
    }

    root->setProperty ("pads", padsArray);

    auto jsonText = juce::JSON::toString (juce::var (root.get()));
    getMappingFile (presetId).replaceWithText (jsonText);
}

std::optional<PadMappingManager::MappingData> PadMappingManager::loadMapping (const juce::String& presetId) const
{
    auto file = getMappingFile (presetId);
    if (! file.existsAsFile())
        return std::nullopt;

    auto jsonText = file.loadFileAsString();
    auto parsed = juce::JSON::parse (jsonText);

    if (! parsed.isObject())
        return std::nullopt;

    MappingData data;
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
                    data.pads[note] = f;
            }

            float vol = (float) (double) padVar.getProperty ("volume", 1.0);
            if (note >= 0 && std::abs (vol - 1.0f) > 0.001f)
                data.volumes[note] = vol;
        }
    }

    if (data.pads.empty())
        return std::nullopt;

    return data;
}

bool PadMappingManager::hasCustomMapping (const juce::String& presetId) const
{
    return getMappingFile (presetId).existsAsFile();
}

void PadMappingManager::clearMapping (const juce::String& presetId)
{
    getMappingFile (presetId).deleteFile();
}
