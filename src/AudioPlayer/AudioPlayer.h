#pragma once
#include "PCH.h"
#include "miniaudio_libvorbis.h"

class AudioPlayer {
public:
    static AudioPlayer& Instance();

    int Play(const std::string& filepath);
    void Stop();
    bool IsPlaying() const;
    std::string GetCurrentFile() const;
    double GetCurrentPositionMilliseconds() const;
    double GetDurationMilliseconds() const;
    float GetVolume() const;
    void SetVolume(float volume);
    

private:
    AudioPlayer();
    ~AudioPlayer();
    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;
    ma_uint64 GetCurrentPositionFrames() const;
    

    ma_engine engine{};
    ma_resource_manager resourceManager{};
    ma_sound sound;
    ma_uint64 sampleRate;
    bool hasSound = false;
    std::string currentFile;
};