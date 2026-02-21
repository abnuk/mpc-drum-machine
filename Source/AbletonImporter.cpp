#include "AbletonImporter.h"

static juce::File getImportLogFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Beatwerk/import_log.txt");
}

static void logImport (const juce::String& msg)
{
    getImportLogFile().appendText (msg + "\n");
}

juce::Array<juce::File> AbletonImporter::findAbletonPresetDirs()
{
    juce::Array<juce::File> dirs;

    auto coreLib = AdgParser::autoDetectAbletonLibrary();
    if (coreLib.isDirectory())
    {
        juce::StringArray subPaths = {
            "Racks/Drum Racks",
            "Presets/Instruments/Drum Rack",
            "Defaults/Slicing"
        };

        for (auto& subPath : subPaths)
        {
            auto dir = coreLib.getChildFile (subPath);
            if (dir.isDirectory())
                dirs.add (dir);
        }
    }

    auto userMusicDir = juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    juce::StringArray userLibPaths = {
        "Ableton/User Library/Presets/Instruments/Drum Rack",
        "Ableton/User Library/Racks/Drum Racks"
    };

    for (auto& subPath : userLibPaths)
    {
        auto dir = userMusicDir.getChildFile (subPath);
        if (dir.isDirectory())
            dirs.add (dir);
    }

    return dirs;
}

juce::String AbletonImporter::computeRelativeSamplePath (const juce::String& absoluteSamplePath,
                                                          const juce::File& abletonCoreLib)
{
    if (abletonCoreLib.isDirectory())
    {
        auto coreLibPath = abletonCoreLib.getFullPathName();
        if (absoluteSamplePath.startsWith (coreLibPath))
        {
            auto relative = absoluteSamplePath.substring (coreLibPath.length());
            if (relative.startsWith (juce::File::getSeparatorString()))
                relative = relative.substring (1);
            return relative;
        }
    }

    auto userLib = juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                       .getChildFile ("Ableton/User Library");
    if (userLib.isDirectory())
    {
        auto userLibPath = userLib.getFullPathName();
        if (absoluteSamplePath.startsWith (userLibPath))
        {
            auto relative = absoluteSamplePath.substring (userLibPath.length());
            if (relative.startsWith (juce::File::getSeparatorString()))
                relative = relative.substring (1);
            return "User Library/" + relative;
        }
    }

    return juce::File (absoluteSamplePath).getFileName();
}

AbletonImporter::ImportResult AbletonImporter::importFromDirectory (
    const juce::File& adgSourceDir,
    const juce::File& samplesDir,
    const juce::File& presetsDir,
    AdgParser& parser,
    std::function<void (float progress, const juce::String& status)> onProgress)
{
    juce::Array<juce::File> dirs;
    dirs.add (adgSourceDir);
    return importFromDirectories (dirs, samplesDir, presetsDir, parser, onProgress);
}

AbletonImporter::ImportResult AbletonImporter::importFromDirectories (
    const juce::Array<juce::File>& adgSourceDirs,
    const juce::File& samplesDir,
    const juce::File& presetsDir,
    AdgParser& parser,
    std::function<void (float progress, const juce::String& status)> onProgress)
{
    ImportResult result;

    samplesDir.createDirectory();
    presetsDir.createDirectory();

    auto logFile = getImportLogFile();
    logFile.replaceWithText ("=== Ableton Import " + juce::Time::getCurrentTime().toISO8601 (true) + " ===\n");
    logImport ("Samples dir: " + samplesDir.getFullPathName());
    logImport ("Presets dir: " + presetsDir.getFullPathName());

    auto abletonCoreLib = parser.getAbletonLibraryPath();
    logImport ("Ableton Core Library: " + abletonCoreLib.getFullPathName());

    juce::Array<juce::File> adgFiles;
    for (auto& dir : adgSourceDirs)
    {
        if (dir.isDirectory())
        {
            logImport ("Scanning: " + dir.getFullPathName());
            auto found = dir.findChildFiles (juce::File::findFiles, true, "*.adg");
            logImport ("  Found " + juce::String (found.size()) + " .adg files");
            adgFiles.addArray (found);
        }
    }

    logImport ("Total .adg files: " + juce::String (adgFiles.size()));

    if (adgFiles.isEmpty())
        return result;

    for (int i = 0; i < adgFiles.size(); ++i)
    {
        auto adgFile = adgFiles[i];
        auto kitName = adgFile.getFileNameWithoutExtension();

        if (onProgress)
        {
            float progress = (float) i / (float) adgFiles.size();
            onProgress (progress, "Importing: " + kitName);
        }

        auto dkitFile = presetsDir.getChildFile (kitName + ".dkit");
        if (dkitFile.existsAsFile())
        {
            result.skippedExisting++;
            logImport ("[SKIP-EXISTS] " + kitName);
            continue;
        }

        auto adgKit = parser.parseFile (adgFile);

        if (adgKit.mappings.empty())
        {
            result.skippedNoSamples++;
            result.skippedNames.add (kitName);
            logImport ("[SKIP-NO-SAMPLES] " + kitName + " (" + adgFile.getFullPathName() + ")");
            continue;
        }

        logImport ("[IMPORT] " + kitName + " - " + juce::String ((int) adgKit.mappings.size()) + " mappings");

        DkitPreset preset;
        preset.name = adgKit.kitName;
        preset.source = "Imported from Ableton Live";
        preset.createdAt = juce::Time::getCurrentTime().toISO8601 (true);

        int missingSamplesInKit = 0;

        for (auto& mapping : adgKit.mappings)
        {
            juce::File srcSample (mapping.samplePath);

            if (! srcSample.existsAsFile())
            {
                missingSamplesInKit++;
                logImport ("  [MISSING] note=" + juce::String (mapping.midiNote)
                           + " path=" + mapping.samplePath);

                DkitPadMapping pad;
                pad.midiNote = mapping.midiNote;
                pad.sampleFile = computeRelativeSamplePath (mapping.samplePath, abletonCoreLib);
                pad.sampleName = mapping.sampleName;
                preset.pads.push_back (pad);
                continue;
            }

            auto relativePath = computeRelativeSamplePath (mapping.samplePath, abletonCoreLib);
            auto destSample = samplesDir.getChildFile (relativePath);

            if (! destSample.existsAsFile())
            {
                destSample.getParentDirectory().createDirectory();
                if (srcSample.copyFileTo (destSample))
                {
                    result.samplesCopied++;
                }
                else
                {
                    result.errors++;
                    result.errorMessages.add ("Failed to copy: " + srcSample.getFileName());
                    logImport ("  [COPY-FAIL] " + srcSample.getFullPathName()
                               + " -> " + destSample.getFullPathName());
                }
            }

            DkitPadMapping pad;
            pad.midiNote = mapping.midiNote;
            pad.sampleFile = relativePath;
            pad.sampleName = mapping.sampleName;
            preset.pads.push_back (pad);
        }

        if (missingSamplesInKit > 0)
            logImport ("  " + juce::String (missingSamplesInKit) + " missing samples in this kit");

        if (PresetManager::writeDkitJson (dkitFile, preset))
        {
            result.presetsImported++;
            logImport ("  -> Written: " + dkitFile.getFileName());
        }
        else
        {
            result.errors++;
            result.errorMessages.add ("Failed to write: " + dkitFile.getFileName());
            logImport ("  [WRITE-FAIL] " + dkitFile.getFullPathName());
        }
    }

    logImport ("\n=== Summary ===");
    logImport ("Imported: " + juce::String (result.presetsImported));
    logImport ("Samples copied: " + juce::String (result.samplesCopied));
    logImport ("Skipped (no audio samples): " + juce::String (result.skippedNoSamples));
    logImport ("Skipped (already exist): " + juce::String (result.skippedExisting));
    logImport ("Errors: " + juce::String (result.errors));

    if (onProgress)
        onProgress (1.0f, "Import complete");

    return result;
}
