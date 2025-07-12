#include "PCH.h"
#include "BNKManager.h"

std::string BNKManager::ConvertWemToOgg(const std::string& wempath) {
    std::string path = wempath.substr(0, wempath.size() - 4) + ".ogg";
    Wwise_RIFF_Vorbis ww(wempath, "", false, false, kNoForcePacketFormat);
    
    ofstream of(path, ios::binary);
    if (!of) {
        throw std::runtime_error("Failed to open output ogg file");
    }
    
    ww.generate_ogg(of);
    of.close();
    return path;
}

std::string BNKManager::NormalizeOgg(const std::string& oggpath) {
    if (Config::Instance().GetExtractionType() == Utils::ExtractionType::ExtractOggOnly) std::filesystem::remove(std::filesystem::u8path(oggpath).replace_extension(".wem"));
    return revorb(oggpath);
}

std::vector<char> BNKManager::Read(const std::string& filepath) {
    std::ifstream in(std::filesystem::u8path(filepath), std::ios::binary);
    if (!in) {
        return std::vector<char>{};
    }
    in.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(in.tellg());
    in.seekg(0, std::ios::beg);
    std::vector<char> data(size);
    in.read(data.data(), size);
    return data;
}

void BNKManager::WriteBnk(const std::string& path, const std::vector<char>& new_didx, const std::vector<char>& new_data) {
    std::ofstream out(std::filesystem::u8path(path), std::ios::binary);
    if (!out) {
        return;
    }

    size_t pos = 0;
    while (pos + 8 <= Data.size()) {
        std::string block_id(&Data[pos], &Data[pos] + 4);
        uint32_t block_size = Utils::ReadUINT32FromData(Data, pos + 4);
        size_t data_offset = pos + 8;

        if (block_id == "DIDX") {
            out.write(block_id.c_str(), 4);
            Utils::WriteUINT32(out, static_cast<uint32_t>(new_didx.size()));
            out.write(new_didx.data(), new_didx.size());
        }
        else if (block_id == "DATA") {
            out.write(block_id.c_str(), 4);
            Utils::WriteUINT32(out, static_cast<uint32_t>(new_data.size()));
            out.write(new_data.data(), new_data.size());
        }
        else {
            out.write(block_id.c_str(), 4);
            Utils::WriteUINT32(out, block_size);
            out.write(Data.data() + data_offset, block_size);
        }
        pos = data_offset + block_size;
    }
    out.close();
}

BNKManager::BNKManager(const std::string& filepath) {
    IsValid = false;
    Path = filepath;
    Data = Read(Path);
    Blocks = ParseBnkBlocks(Data);
    if (!Blocks.count("DIDX") || !Blocks.count("DATA")) {
        throw std::runtime_error("Error: blocks DIDX or DATA missing");
    }
    Entries = ParseDIDX(Data, Blocks["DIDX"]);
    GenerateWemInfo();
    IsValid = true;
}

std::map<std::string, BNK::Block> BNKManager::ParseBnkBlocks(const std::vector<char>& data) {
    std::map<std::string, BNK::Block> blocks;
    size_t pos = 0;
    while (pos + 8 <= data.size()) {
        std::string block_id(&data[pos], &data[pos] + 4);
        uint32_t block_size = Utils::ReadUINT32FromData(data, pos + 4);
        size_t data_offset = pos + 8;
        uint32_t data_bytesPerSecond = Utils::ReadUINT32FromData(data, pos + 28);
        if (data_offset + block_size > data.size()) {
            return std::map<std::string, BNK::Block>{};
        }
        blocks[block_id] = { block_id, data_offset, block_size, data_bytesPerSecond };
        pos = data_offset + block_size;
    }
    return blocks;
}

std::vector<BNK::DIDXEntry> BNKManager::ParseDIDX(const std::vector<char>& data, const BNK::Block& didx_block) {
    std::vector<BNK::DIDXEntry> entries;
    size_t pos = didx_block.offset;
    size_t end = pos + didx_block.size;
    while (pos + 12 <= end) {
        uint32_t id = Utils::ReadUINT32FromData(data, pos);
        uint32_t offset = Utils::ReadUINT32FromData(data, pos + 4);
        uint32_t size = Utils::ReadUINT32FromData(data, pos + 8);

        entries.push_back({ id, offset, size });
        pos += 12;
    }
    return entries;
}

std::vector<char> BNKManager::BuildDIDX(const std::vector<BNK::DIDXEntry>& entries) {
    std::vector<char> new_didx;
    for (auto& entry : entries) {
        Utils::WriteUINT32ToVector(new_didx, entry.id);
        Utils::WriteUINT32ToVector(new_didx, entry.offset);
        Utils::WriteUINT32ToVector(new_didx, entry.size);
    }
    return new_didx;
}

std::string BNKManager::ReplaceWems(const std::vector<std::pair<int, std::string>>& replacements, const std::string& out_path) {
    std::unordered_map<int, std::vector<char>> new_wem_data_map;

    for (const auto& [index, wem_path] : replacements) {
        if (index < 0 || index >= static_cast<int>(Entries.size()))
            return "";

        std::vector<char> wem_data = Read(wem_path);
        new_wem_data_map[index] = std::move(wem_data);
        Entries[index].size = static_cast<uint32_t>(new_wem_data_map[index].size());
    }

    std::vector<char> rebuilt_data;
    uint32_t cur_offset = 0;

    for (size_t i = 0; i < Entries.size(); ++i) {
        auto& entry = Entries[i];

        auto it = new_wem_data_map.find(static_cast<int>(i));
        if (it != new_wem_data_map.end()) {
            rebuilt_data.insert(rebuilt_data.end(), it->second.begin(), it->second.end());
            entry.offset = cur_offset;
            cur_offset += static_cast<uint32_t>(it->second.size());
        }
        else {
            size_t old_data_pos = Blocks["DATA"].offset + entry.offset;
            rebuilt_data.insert(
                rebuilt_data.end(),
                Data.begin() + old_data_pos,
                Data.begin() + old_data_pos + entry.size
            );
            entry.offset = cur_offset;
            cur_offset += entry.size;
        }
    }
    WriteBnk(out_path, BuildDIDX(Entries), rebuilt_data);
    return out_path;
}

std::string BNKManager::ExtractWem(int index, const std::string out_path) {
    if (index < 0 || index >= (int)Entries.size()) {
        return "";
    }
    const auto& entry = Entries[index];
    size_t data_pos = Blocks["DATA"].offset + entry.offset;
    if (data_pos + entry.size > Data.size()) {
        return "";
    }
    std::ofstream out(out_path, std::ios::binary);
    if (!out) {
        return "";
    }
    out.write(Data.data() + data_pos, entry.size);
    out.close();

    int extractionType = Config::Instance().GetExtractionType();
    if (extractionType == Utils::ExtractionType::ExtractWemOnly) {
        return out_path;
    }
    else {
        return NormalizeOgg(ConvertWemToOgg(out_path));
    }
}

void BNKManager::GenerateWemInfo() {
    bool withshortnames = false;
    bool withsubtitles = false;
    std::unordered_map<uint32_t, std::string> shortnames;
    std::vector<BNK::SubtitleGroup> all_subtitles;
    if (std::filesystem::exists(std::filesystem::u8path(Config::Instance().GetSoundbanksInfoPath())) && std::filesystem::is_regular_file(std::filesystem::u8path(Config::Instance().GetSoundbanksInfoPath()))) {
        shortnames = Utils::ParseSoundbanksInfoFile(Config::Instance().GetSoundbanksInfoPath());
        withshortnames = true;
    }
    if (withshortnames && (std::filesystem::exists(std::filesystem::u8path(Config::Instance().GetSubtitlesPath())) && std::filesystem::is_regular_file(std::filesystem::u8path(Config::Instance().GetSubtitlesPath())))) {
        all_subtitles = Utils::ParseSubtitleFile(Config::Instance().GetSubtitlesPath());
        withsubtitles = true;
    }
    for (size_t i = 0; i < Entries.size(); ++i) {
        size_t data_pos = Blocks["DATA"].offset + Entries[i].offset;
        uint32_t duration = Entries[i].size / Utils::ReadUINT32FromData(Data, data_pos + 28);
        std::string duration_format = Utils::FormatSecondsToTime(duration);
        std::string shortname = "";
        std::vector<std::string> subtitles = {};

        if (withshortnames) {
            shortname = Utils::GetShortName(Entries[i].id, shortnames);
            shortname = shortname.substr(0, shortname.size() - 4);
            if (withsubtitles) subtitles = Utils::GetSubtitlesByShortName(all_subtitles, shortname);
        }

        Wems.push_back({ Entries[i].id, Entries[i].offset, Entries[i].size, duration, duration_format, shortname, subtitles });
    }
}