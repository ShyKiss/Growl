#include "PCH.h"
#include "AudioPlayer.h"
#define MINIAUDIO_IMPLEMENTATION
#include "GUI/Menu.h"
#include "miniaudio_libvorbis.h"

AudioPlayer& AudioPlayer::Instance() {
    static AudioPlayer instance;
    return instance;
}

AudioPlayer::AudioPlayer() {
    ma_result result;

    ma_decoding_backend_vtable* backends[] = { ma_decoding_backend_libvorbis };

    ma_resource_manager_config resCfg = ma_resource_manager_config_init();
    resCfg.ppCustomDecodingBackendVTables = backends;
    resCfg.customDecodingBackendCount = 1;

    result = ma_resource_manager_init(&resCfg, &resourceManager);
    if (result != MA_SUCCESS)
        throw std::runtime_error("Failed to initialize ma_resource_manager");

    ma_engine_config engCfg = ma_engine_config_init();
    engCfg.pResourceManager = &resourceManager;

    result = ma_engine_init(&engCfg, &engine);
    if (result != MA_SUCCESS)
        throw std::runtime_error("Failed to initialize ma_engine");
}

AudioPlayer::~AudioPlayer() {
    if (hasSound)
        ma_sound_uninit(&sound);
    ma_engine_uninit(&engine);
    ma_resource_manager_uninit(&resourceManager);
}

int AudioPlayer::Play(const std::string& filepath) {
    if (hasSound) {
        ma_sound_uninit(&sound);
        hasSound = false;
    }

    ma_result result = ma_sound_init_from_file(
        &engine,
        filepath.c_str(),
        0,
        nullptr, nullptr,
        &sound
    );

    if (result != MA_SUCCESS) return result;
    sampleRate = sound.engineNode.sampleRate;

    ma_sound_set_volume(&sound, Config::Instance().GetVolume());
    ma_sound_start(&sound);
    hasSound = true;
    currentFile = std::filesystem::path(filepath).stem().string();
    return MA_SUCCESS;
}

void AudioPlayer::Stop() {
    if (hasSound)
        ma_sound_stop(&sound);
}

bool AudioPlayer::IsPlaying() const {
    return hasSound && ma_sound_is_playing(&sound);
}

std::string AudioPlayer::GetCurrentFile() const {
    return currentFile;
}

ma_uint64 AudioPlayer::GetCurrentPositionFrames() const {
    if (!hasSound) return 0;
    ma_uint64 cursor = 0;
    ma_sound_get_cursor_in_pcm_frames((ma_sound*)&sound, &cursor);
    return cursor;
}

double AudioPlayer::GetCurrentPositionMilliseconds() const {
    if (!hasSound || sampleRate == 0) return 0.0;
    ma_uint64 cursor = GetCurrentPositionFrames();
    return static_cast<double>(cursor) / sampleRate * 1000.f;
}

double AudioPlayer::GetDurationMilliseconds() const {
    if (!hasSound || sampleRate == 0) return 0.0;

    ma_uint64 totalFrames = 0;
    ma_sound_get_length_in_pcm_frames((ma_sound*)&sound, &totalFrames);
;
    return static_cast<double>(totalFrames) / sampleRate;
}

float AudioPlayer::GetVolume() const {
    if (!hasSound) return 0.0f;
    return ma_sound_get_volume(&sound);
}

void AudioPlayer::SetVolume(float volume) {
    if (!hasSound) return;
    ma_sound_set_volume(&sound, volume);
}