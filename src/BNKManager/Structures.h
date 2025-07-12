#pragma once
#include "PCH.h"

namespace BNK {
    struct DIDXEntry {
        uint32_t id;
        uint32_t offset;
        uint32_t size;
    };

    struct Block {
        std::string id;
        size_t offset;
        uint32_t size;
        uint32_t bytesPerSecond;
    };

    struct SubtitleGroup {
        std::string shortname;
        std::vector<std::string> subtitles;
    };

    struct WemFile {
        uint32_t id;
        uint32_t offset;
        uint32_t size;
        uint32_t duration; 
        std::string duration_format;
        std::string shortname;
        std::vector<std::string> subtitles;
    };
}