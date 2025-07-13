#include "PCH.h"
#include "Utils.h"
#include "nfd.h"
#include "SimpleIni.h"
#pragma execution_character_set("utf-8")

namespace Utils {
    std::wstring utf8_to_utf16(const std::string& utf8) {
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    std::unordered_map<uint32_t, std::string> ParseSoundbanksInfoFile(const std::string& xmlPath) {
        std::wstring wpath = utf8_to_utf16(xmlPath);
        FILE* file = _wfopen(wpath.c_str(), L"rb");
        if (!file) {
            return {};
        }

        tinyxml2::XMLDocument doc;
        std::unordered_map<uint32_t, std::string> result;

        if (doc.LoadFile(file) != tinyxml2::XML_SUCCESS) {
            return {};
        }

        tinyxml2::XMLElement* root = doc.FirstChildElement("SoundBanksInfo");
        if (!root) return {};

        tinyxml2::XMLElement* soundBanks = root->FirstChildElement("SoundBanks");
        if (!soundBanks) return {};

        for (tinyxml2::XMLElement* soundBank = soundBanks->FirstChildElement("SoundBank"); soundBank; soundBank = soundBank->NextSiblingElement("SoundBank")) {
            tinyxml2::XMLElement* includedFiles = soundBank->FirstChildElement("IncludedFullFiles");
            if (!includedFiles)
                includedFiles = soundBank->FirstChildElement("IncludedMemoryFiles");
            if (!includedFiles) continue;

            for (tinyxml2::XMLElement* file = includedFiles->FirstChildElement("File"); file; file = file->NextSiblingElement("File")) {
                uint32_t fileId = 0;
                file->QueryUnsignedAttribute("Id", &fileId);
                tinyxml2::XMLElement* shortName = file->FirstChildElement("ShortName");
                if (shortName && shortName->GetText()) {
                    result[fileId] = shortName->GetText();
                }
                else {
                    result[fileId] = "";
                }
            }
        }

        return result;
    }

    std::vector<SoundBankInfo> ParseSoundbanksInfoFileForSearcher(const std::string& xmlPath) {
        std::wstring wpath = utf8_to_utf16(xmlPath);
        FILE* file = _wfopen(wpath.c_str(), L"rb");
        if (!file) {
            return {};
        }

        tinyxml2::XMLDocument doc;
        std::vector<SoundBankInfo> result;

        if (doc.LoadFile(file) != tinyxml2::XML_SUCCESS) {
            return {};
        }

        tinyxml2::XMLElement* root = doc.FirstChildElement("SoundBanksInfo");
        if (!root) return {};

        tinyxml2::XMLElement* soundBanks = root->FirstChildElement("SoundBanks");
        if (!soundBanks) return {};

        for (tinyxml2::XMLElement* soundBank = soundBanks->FirstChildElement("SoundBank");
            soundBank; soundBank = soundBank->NextSiblingElement("SoundBank")) {

            const char* lang = soundBank->Attribute("Language");
            if (!lang) continue;

            std::string language = lang;
            if (language != "SFX" && language != "English(US)")
                continue;
            SoundBankInfo sbInfo;

            tinyxml2::XMLElement* nameElem = soundBank->FirstChildElement("ShortName");
            if (nameElem && nameElem->GetText()) {
                sbInfo.name = nameElem->GetText();
            }
            else {
                sbInfo.name = "Unknown";
            }

            tinyxml2::XMLElement* includedFiles = soundBank->FirstChildElement("IncludedFullFiles");
            if (!includedFiles)
                includedFiles = soundBank->FirstChildElement("IncludedMemoryFiles");
            if (!includedFiles) continue;

            for (tinyxml2::XMLElement* file = includedFiles->FirstChildElement("File");
                file; file = file->NextSiblingElement("File")) {
                uint32_t fileId = 0;
                file->QueryUnsignedAttribute("Id", &fileId);
                std::string shortName;

                tinyxml2::XMLElement* shortNameElem = file->FirstChildElement("ShortName");
                if (shortNameElem && shortNameElem->GetText()) {
                    shortName = shortNameElem->GetText();
                    shortName = shortName.substr(0, shortName.size() - 4);
                }

                sbInfo.files.push_back({ fileId, shortName });
            }

            result.push_back(std::move(sbInfo));
        }

        return result;
    }

    std::string GetShortName(uint32_t id, const std::unordered_map<uint32_t, std::string>& idMap) {
        auto it = idMap.find(id);
        return (it != idMap.end()) ? it->second : "";
    }

    std::vector<BNK::SubtitleGroup> ParseSubtitleFile(const std::string& filePath) {
        std::wstring wpath = utf8_to_utf16(filePath);
        FILE* file = _wfopen(wpath.c_str(), L"rb");
        if (!file) {
            return {};
        }

        CSimpleIniA ini;
        ini.SetUnicode();

        SI_Error rc = ini.LoadFile(file);

        CSimpleIniA::TNamesDepend keys;
        ini.GetAllKeys("Script", keys);

        std::vector<BNK::SubtitleGroup> groups;

        for (auto& key : keys) {
            const char* val = ini.GetValue("Script", key.pItem);
            if (!val) continue;

            BNK::SubtitleGroup group;
            group.shortname = key.pItem;
            group.subtitles.push_back(val);

            groups.push_back(std::move(group));
        }

        return groups;
    }

    std::vector<std::string> GetSubtitlesByShortName(const std::vector<BNK::SubtitleGroup>& groups, const std::string& shortname) {
        for (const auto& group : groups) {
            if (group.shortname == shortname) {
                return group.subtitles;
            }
        }
        return {};
    }

    uint32_t ReadUINT32FromData(const std::vector<char>& data, size_t pos) {
        return
            static_cast<uint8_t>(data[pos]) |
            (static_cast<uint8_t>(data[pos + 1]) << 8) |
            (static_cast<uint8_t>(data[pos + 2]) << 16) |
            (static_cast<uint8_t>(data[pos + 3]) << 24);
    }

    void WriteUINT32ToVector(std::vector<char>& buf, uint32_t val) {
        buf.push_back(static_cast<char>(val & 0xFF));
        buf.push_back(static_cast<char>((val >> 8) & 0xFF));
        buf.push_back(static_cast<char>((val >> 16) & 0xFF));
        buf.push_back(static_cast<char>((val >> 24) & 0xFF));
    }

    void WriteUINT32(std::ostream& out, uint32_t val) {
        char bytes[4];
        bytes[0] = static_cast<char>(val & 0xFF);
        bytes[1] = static_cast<char>((val >> 8) & 0xFF);
        bytes[2] = static_cast<char>((val >> 16) & 0xFF);
        bytes[3] = static_cast<char>((val >> 24) & 0xFF);
        out.write(bytes, 4);
    }

    std::string FormatSecondsToTime(int total_seconds) {
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << total_seconds / 3600 << ":"
            << std::setfill('0') << std::setw(2) << (total_seconds % 3600) / 60 << ":"
            << std::setfill('0') << std::setw(2) << total_seconds % 60;
        return oss.str();
    }

    std::string FormatMillisecondsToTime(int total_milliseconds) {
        int total_seconds = total_milliseconds / 1000;
        int milliseconds = total_milliseconds % 1000;

        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << total_seconds / 3600 << ":"
            << std::setfill('0') << std::setw(2) << (total_seconds % 3600) / 60 << ":"
            << std::setfill('0') << std::setw(2) << total_seconds % 60 << "."
            << std::setfill('0') << std::setw(3) << milliseconds;
        return oss.str();
    }

    std::string OpenFileDialog(std::string filter) {
        std::string result;

        nfdchar_t* outPath = nullptr;
        nfdresult_t resultCode = NFD_OpenDialog(filter.c_str(), GetAppDirectory().c_str(), &outPath);

        if (resultCode == NFD_OKAY && outPath)
        {
            result = outPath;
            free(outPath);
        }

        return result;
    }
    std::vector<std::string> OpenMultipleFilesDialog(std::string filter) {
        std::vector<std::string> result;
        nfdpathset_t pathSet;

        nfdresult_t resultCode = NFD_OpenDialogMultiple(filter.c_str(), GetAppDirectory().c_str(), &pathSet);
        if (resultCode == NFD_OKAY)
        {
            size_t count = NFD_PathSet_GetCount(&pathSet);
            for (size_t i = 0; i < count; ++i)
            {
                result.emplace_back(NFD_PathSet_GetPath(&pathSet, i));
            }
            NFD_PathSet_Free(&pathSet);
        }

        return result;
    }

    std::string OpenFolderDialog() {
        char* out_path;
        NFD_PickFolder(GetAppDirectory().c_str(), &out_path);
        return std::string(out_path);
    }

    std::string SaveFileDialog(std::string filter) {
        std::string result;

        nfdchar_t* outPath = nullptr;
        nfdresult_t resultCode = NFD_SaveDialog(filter.c_str(), GetAppDirectory().c_str(), &outPath);

        if (resultCode == NFD_OKAY && outPath)
        {
            result = outPath;
            free(outPath);
        }

        return result;
    }

    void CreateWwiseExternalSources(const std::vector<std::string>& paths, const std::string& outputPath) {
        using namespace tinyxml2;

        tinyxml2::XMLDocument doc;

        tinyxml2::XMLDeclaration* decl = doc.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")");
        doc.InsertFirstChild(decl);

        tinyxml2::XMLElement* root = doc.NewElement("ExternalSourcesList");
        root->SetAttribute("SchemaVersion", "1");
        root->SetAttribute("Root", "growl_convert\\input");
        doc.InsertEndChild(root);

        for (const std::string& path : paths) {
            tinyxml2::XMLElement* sourceElem = doc.NewElement("Source");
            sourceElem->SetAttribute("Path", std::filesystem::u8path(path).filename().string().c_str());
            sourceElem->SetAttribute("Conversion", "Vorbis Quality High");
            root->InsertEndChild(sourceElem);
        }
        std::filesystem::create_directories(std::filesystem::u8path(outputPath).parent_path());
        tinyxml2::XMLError result = doc.SaveFile(outputPath.c_str());
    }

    std::string GetAppDirectory() {
        wchar_t buffer[MAX_PATH];
        DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        return std::filesystem::path(buffer).parent_path().string();
    }
}