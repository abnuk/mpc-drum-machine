#include "PresetManager.h"

PresetManager::PresetManager()
{
    samplesDir = getDefaultSamplesDir();
    presetsDir = getDefaultPresetsDir();
}

juce::File PresetManager::getDefaultSamplesDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Beatwerk/Samples");
}

juce::File PresetManager::getDefaultPresetsDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Beatwerk/Presets");
}

void PresetManager::setSamplesDir (const juce::File& dir)
{
    samplesDir = dir;
}

void PresetManager::setPresetsDir (const juce::File& dir)
{
    presetsDir = dir;
}

void PresetManager::scanForPresets()
{
    presets.clear();

    if (! presetsDir.isDirectory())
        return;

    auto dkitFiles = presetsDir.findChildFiles (juce::File::findFiles, true, "*.dkit");
    dkitFiles.sort();

    for (auto& f : dkitFiles)
    {
        PresetEntry entry;
        entry.name = f.getFileNameWithoutExtension();
        entry.file = f;
        presets.push_back (entry);
    }

    std::sort (presets.begin(), presets.end(), [] (const PresetEntry& a, const PresetEntry& b)
    {
        return a.name.compareIgnoreCase (b.name) < 0;
    });
}

int PresetManager::getNumPresets() const
{
    return (int) presets.size();
}

juce::String PresetManager::getPresetName (int index) const
{
    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].name;
    return {};
}

bool PresetManager::loadPreset (int index)
{
    if (index < 0 || index >= (int) presets.size())
        return false;

    auto& entry = presets[(size_t) index];

    if (! loadDkitFile (entry.file))
        return false;

    currentIndex = index;

    if (onPresetLoaded)
        onPresetLoaded (currentKit);

    return true;
}

bool PresetManager::loadNextPreset()
{
    if (presets.empty())
        return false;

    int next = currentIndex + 1;
    if (next >= (int) presets.size())
        next = 0;

    return loadPreset (next);
}

bool PresetManager::loadPreviousPreset()
{
    if (presets.empty())
        return false;

    int prev = currentIndex - 1;
    if (prev < 0)
        prev = (int) presets.size() - 1;

    return loadPreset (prev);
}

bool PresetManager::loadDkitFile (const juce::File& file)
{
    auto preset = parseDkitJson (file);
    if (preset.name.isEmpty())
        return false;

    currentKit = preset;
    return true;
}

DkitPreset PresetManager::parseDkitJson (const juce::File& file)
{
    DkitPreset preset;
    preset.sourceFile = file;

    auto jsonText = file.loadFileAsString();
    auto parsed = juce::JSON::parse (jsonText);

    if (! parsed.isObject())
        return preset;

    preset.name = parsed.getProperty ("name", "").toString();
    preset.author = parsed.getProperty ("author", "").toString();
    preset.description = parsed.getProperty ("description", "").toString();
    preset.source = parsed.getProperty ("source", "").toString();
    preset.createdAt = parsed.getProperty ("createdAt", "").toString();

    auto padsArray = parsed.getProperty ("pads", juce::var());
    if (padsArray.isArray())
    {
        for (int i = 0; i < padsArray.size(); ++i)
        {
            auto padVar = padsArray[i];
            DkitPadMapping mapping;
            mapping.midiNote = (int) padVar.getProperty ("midiNote", -1);
            mapping.sampleFile = padVar.getProperty ("sampleFile", "").toString();
            mapping.sampleName = padVar.getProperty ("sampleName", "").toString();
            if (mapping.midiNote >= 0)
                preset.pads.push_back (mapping);
        }
    }

    return preset;
}

bool PresetManager::writeDkitJson (const juce::File& file, const DkitPreset& preset)
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty ("formatVersion", 1);
    root->setProperty ("name", preset.name);
    root->setProperty ("author", preset.author);
    root->setProperty ("description", preset.description);
    root->setProperty ("source", preset.source);
    root->setProperty ("createdAt", preset.createdAt);

    juce::Array<juce::var> padsArray;
    for (auto& pad : preset.pads)
    {
        juce::DynamicObject::Ptr padObj = new juce::DynamicObject();
        padObj->setProperty ("midiNote", pad.midiNote);
        padObj->setProperty ("sampleFile", pad.sampleFile);
        padObj->setProperty ("sampleName", pad.sampleName);
        padsArray.add (juce::var (padObj.get()));
    }

    root->setProperty ("pads", padsArray);

    auto jsonText = juce::JSON::toString (juce::var (root.get()));
    return file.replaceWithText (jsonText);
}

bool PresetManager::savePreset (const juce::String& name,
                                 const std::map<int, juce::File>& padMappings)
{
    presetsDir.createDirectory();

    DkitPreset preset;
    preset.name = name;
    preset.source = "User created";
    preset.createdAt = juce::Time::getCurrentTime().toISO8601 (true);

    for (auto& [note, sampleFile] : padMappings)
    {
        DkitPadMapping pad;
        pad.midiNote = note;
        pad.sampleFile = makeRelativeSamplePath (sampleFile);
        pad.sampleName = sampleFile.getFileNameWithoutExtension();
        preset.pads.push_back (pad);
    }

    auto file = presetsDir.getChildFile (name + ".dkit");
    return writeDkitJson (file, preset);
}

juce::File PresetManager::getPresetFile (int index) const
{
    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].file;
    return {};
}

bool PresetManager::deletePreset (int index)
{
    if (index < 0 || index >= (int) presets.size())
        return false;

    auto& entry = presets[(size_t) index];

    if (! entry.file.deleteFile())
        return false;

    presets.erase (presets.begin() + index);

    if (presets.empty())
    {
        currentIndex = -1;
        currentKit = {};
    }
    else if (index == currentIndex)
    {
        int newIdx = juce::jmin (index, (int) presets.size() - 1);
        loadPreset (newIdx);
    }
    else if (index < currentIndex)
    {
        --currentIndex;
    }

    return true;
}

bool PresetManager::renamePreset (int index, const juce::String& newName)
{
    if (index < 0 || index >= (int) presets.size())
        return false;

    if (newName.trim().isEmpty())
        return false;

    auto& entry = presets[(size_t) index];
    auto newFile = entry.file.getParentDirectory().getChildFile (newName + ".dkit");

    if (newFile.existsAsFile() && newFile != entry.file)
        return false;

    auto preset = parseDkitJson (entry.file);
    if (preset.name.isEmpty())
        return false;

    preset.name = newName;
    if (! writeDkitJson (entry.file, preset))
        return false;

    if (newFile != entry.file)
    {
        if (! entry.file.moveFileTo (newFile))
            return false;
    }

    entry.name = newName;
    entry.file = newFile;

    if (index == currentIndex)
    {
        currentKit.name = newName;
        currentKit.sourceFile = newFile;
    }

    return true;
}

juce::File PresetManager::resolveSamplePath (const juce::String& relativePath) const
{
    if (relativePath.isEmpty())
        return {};

    return samplesDir.getChildFile (relativePath);
}

juce::String PresetManager::makeRelativeSamplePath (const juce::File& sampleFile)
{
    if (! sampleFile.existsAsFile())
        return {};

    // If the file is already inside samplesDir, compute relative path
    auto sampleFullPath = sampleFile.getFullPathName();
    auto samplesFullPath = samplesDir.getFullPathName();

    if (sampleFullPath.startsWith (samplesFullPath))
    {
        auto relative = sampleFullPath.substring (samplesFullPath.length());
        if (relative.startsWith (juce::File::getSeparatorString()))
            relative = relative.substring (1);
        return relative;
    }

    // File is outside samplesDir -- copy it in
    samplesDir.createDirectory();
    auto destFile = samplesDir.getChildFile (sampleFile.getFileName());

    // Avoid overwriting if a different file with the same name exists
    if (destFile.existsAsFile() && destFile.getFullPathName() != sampleFile.getFullPathName())
    {
        auto baseName = sampleFile.getFileNameWithoutExtension();
        auto ext = sampleFile.getFileExtension();
        int counter = 2;
        while (destFile.existsAsFile())
        {
            destFile = samplesDir.getChildFile (baseName + "_" + juce::String (counter) + ext);
            ++counter;
        }
    }

    if (! destFile.existsAsFile())
        sampleFile.copyFileTo (destFile);

    return destFile.getFileName();
}
