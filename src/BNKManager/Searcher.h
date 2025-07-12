#pragma once
#include "PCH.h"

class Searcher {
	bool IsValid;
	bool IsSoundbanksInfoValid;
	bool IsSubtitlesValid;
	std::vector<Utils::SoundBankInfo> ParsedList;
	std::vector<BNK::SubtitleGroup> ParsedSubtitles;

public:
	Searcher();
	std::vector<Utils::SoundBankInfo> GetParsedList() { return ParsedList; }
	std::vector<BNK::SubtitleGroup> GetParsedSubtitles() { return ParsedSubtitles; }
	bool GetIsSoundbanksInfoValid() { return IsSoundbanksInfoValid; }
	bool GetIsSubtitlesValid() { return IsSubtitlesValid; }
	bool GetIsValid() { return IsValid; }
};