#pragma once
#include "PCH.h"

namespace Utils {
	enum ExtractionType {
		ExtractWemOnly = 0,
		ExtractOggOnly = 1,
		ExtractWemAndOgg = 2
	};

	enum ExtractionName {
		ExtractId = 0,
		ExtractShortname = 1
	};

	struct SoundBankInfoFileInfo {
		uint32_t id;
		std::string shortName;
	};

	struct SoundBankInfo {
		std::string name;
		std::vector<SoundBankInfoFileInfo> files;
	};

	std::wstring									utf8_to_utf16						(const std::string& utf8);
	std::unordered_map<uint32_t, std::string>		ParseSoundbanksInfoFile				(const std::string& xmlPath);
	std::vector<SoundBankInfo>						ParseSoundbanksInfoFileForSearcher	(const std::string& xmlPath);
	std::string										GetShortName						(uint32_t id, const std::unordered_map<uint32_t, std::string>& idMap);
	std::vector<BNK::SubtitleGroup>					ParseSubtitleFile					(const std::string& filePath);
	std::vector<std::string>						GetSubtitlesByShortName				(const std::vector<BNK::SubtitleGroup>& groups, const std::string& shortname);
	uint32_t										ReadUINT32FromData					(const std::vector<char>& data, size_t pos);
	void											WriteUINT32ToVector					(std::vector<char>& buf, uint32_t val);
	void											WriteUINT32							(std::ostream& out, uint32_t val);
	std::string										FormatSecondsToTime					(int total_seconds);
	std::string										FormatMillisecondsToTime			(int total_milliseconds);
	std::string										OpenFileDialog						(std::string filter);
	std::vector<std::string>						OpenMultipleFilesDialog				(std::string filter);
	std::string										OpenFolderDialog();
	std::string										SaveFileDialog						(std::string);
	void											CreateWwiseExternalSources			(const std::vector<std::string>& paths, const std::string& outputPath);
	std::string										GetAppDirectory();
}