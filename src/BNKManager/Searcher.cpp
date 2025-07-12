#include "PCH.h"
#include "Searcher.h"

Searcher::Searcher() {
    IsValid = false;
    
    IsSoundbanksInfoValid = !Config::Instance().GetSoundbanksInfoPath().empty();
    IsSubtitlesValid = !Config::Instance().GetSubtitlesPath().empty();

    if (!IsSoundbanksInfoValid) {
        return;
    }

    ParsedList = Utils::ParseSoundbanksInfoFileForSearcher(Config::Instance().GetSoundbanksInfoPath());
    if (IsSubtitlesValid) ParsedSubtitles = Utils::ParseSubtitleFile(Config::Instance().GetSubtitlesPath());

    IsValid = true;
}