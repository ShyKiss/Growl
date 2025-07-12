#include "tinyxml2.h"
#include <string>
#include <iostream>

class Config {
    std::string configFile = Utils::GetAppDirectory() + "\\Growl-Config.xml";

    float audioVolume = 1.0f;
    bool audioMuted = false;
    int extractionType = 2;
    int extractionName = 0;

    std::string wwiseProjectPath;
    std::string wwiseCLIPath;
    std::string subtitlesPath;
    std::string soundbanksInfoPath;

    bool convertWemToOgg;

    Config() { Load(); }

public:
    static Config& Instance() {
        static Config instance;
        return instance;
    }

    void Load() {
        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(configFile.c_str()) != tinyxml2::XML_SUCCESS) {
            Save();
            return;
        }

        tinyxml2::XMLElement* root = doc.FirstChildElement("Config");
        if (!root) return;

        if (tinyxml2::XMLElement* e = root->FirstChildElement("WwiseProjectPath"); e && e->GetText())
            wwiseProjectPath = e->GetText();
        if (tinyxml2::XMLElement* e = root->FirstChildElement("WwiseCLIPath"); e && e->GetText())
            wwiseCLIPath = e->GetText();
        if (tinyxml2::XMLElement* e = root->FirstChildElement("SoundbanksInfoPath"); e && e->GetText())
            soundbanksInfoPath = e->GetText();
        if (tinyxml2::XMLElement* e = root->FirstChildElement("SubtitlesPath"); e && e->GetText())
            subtitlesPath = e->GetText();

        if (tinyxml2::XMLElement* bnkmanager = root->FirstChildElement("BNKManager")) {
            bnkmanager->QueryIntAttribute("ExtractionType", &extractionType);
            bnkmanager->QueryIntAttribute("ExtractionName", &extractionName);
        }

        if (tinyxml2::XMLElement* audio = root->FirstChildElement("AudioPlayer")) {
            audio->QueryFloatAttribute("Volume", &audioVolume);
        }
    }

    void Save() const {
        tinyxml2::XMLDocument doc;
        doc.InsertFirstChild(doc.NewDeclaration());

        tinyxml2::XMLElement* root = doc.NewElement("Config");
        doc.InsertEndChild(root);

        tinyxml2::XMLElement* wwiseprojectpath = doc.NewElement("WwiseProjectPath");
        wwiseprojectpath->SetText(wwiseProjectPath.c_str());
        root->InsertEndChild(wwiseprojectpath);

        tinyxml2::XMLElement* wwiseclipath = doc.NewElement("WwiseCLIPath");
        wwiseclipath->SetText(wwiseCLIPath.c_str());
        root->InsertEndChild(wwiseclipath);

        tinyxml2::XMLElement* soundbanksinfopath = doc.NewElement("SoundbanksInfoPath");
        soundbanksinfopath->SetText(soundbanksInfoPath.c_str());
        root->InsertEndChild(soundbanksinfopath);

        tinyxml2::XMLElement* subtitlespath = doc.NewElement("SubtitlesPath");
        subtitlespath->SetText(subtitlesPath.c_str());
        root->InsertEndChild(subtitlespath);

        tinyxml2::XMLElement* bnkmanager = doc.NewElement("BNKManager");
        bnkmanager->SetAttribute("ExtractionType", extractionType);
        bnkmanager->SetAttribute("ExtractionName", extractionName);
        root->InsertEndChild(bnkmanager);

        tinyxml2::XMLElement* audioplayer = doc.NewElement("AudioPlayer");
        audioplayer->SetAttribute("Volume", audioVolume);
        root->InsertEndChild(audioplayer);

        doc.SaveFile(configFile.c_str());
    }

    const std::string&  GetWwiseProjectPath() const { return wwiseProjectPath; }
    void                SetWwiseProjectPath(const std::string& path) { wwiseProjectPath = path; Save(); }
    const std::string&  GetWwiseCLIPath() const { return wwiseCLIPath; }
    void                SetWwiseCLIPath(const std::string& path) { wwiseCLIPath = path; Save(); }
    const std::string&  GetSoundbanksInfoPath() const { return soundbanksInfoPath; }
    void                SetSoundbanksInfoPath(const std::string& path) { soundbanksInfoPath = path; Save(); }
    const std::string&  GetSubtitlesPath() const { return subtitlesPath; }
    void                SetSubtitlesPath(const std::string& path) { subtitlesPath = path; Save(); }

    int                 GetExtractionType() const { return extractionType; }
    void                SetExtractionType(const int val) { extractionType = val; Save(); }
    int                 GetExtractionName() const { return extractionName; }
    void                SetExtractionName(const int val) { extractionName = val; Save(); }

    float               GetVolume() { return audioVolume; }
    void                SetVolume(float vol) { audioVolume = vol; Save(); }

    Config(const Config&) = delete;
    void operator=(const Config&) = delete;
};
