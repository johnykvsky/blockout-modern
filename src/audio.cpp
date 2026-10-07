#include "audio.hpp"
#include <iostream>

namespace blockout {

AudioManager::AudioManager() {
    InitAudioDevice();
    if (IsAudioDeviceReady()) {
        inited_ = true;
        snd_blub_ = load_safe("assets/sounds/blub.wav");
        snd_wozz_ = load_safe("assets/sounds/wozz.wav");
        snd_tchh_ = load_safe("assets/sounds/tchh.wav");
        snd_line_ = load_safe("assets/sounds/line.wav");
        snd_empty_ = load_safe("assets/sounds/empty.wav");
        snd_hit_ = load_safe("assets/sounds/hit.wav");
        snd_level_ = load_safe("assets/sounds/level.wav");
        snd_welldone_ = load_safe("assets/sounds/welldone.wav");
    }
}

AudioManager::~AudioManager() {
    if (inited_) {
        if (snd_blub_.frameCount > 0) UnloadSound(snd_blub_);
        if (snd_wozz_.frameCount > 0) UnloadSound(snd_wozz_);
        if (snd_tchh_.frameCount > 0) UnloadSound(snd_tchh_);
        if (snd_line_.frameCount > 0) UnloadSound(snd_line_);
        if (snd_empty_.frameCount > 0) UnloadSound(snd_empty_);
        if (snd_hit_.frameCount > 0) UnloadSound(snd_hit_);
        if (snd_level_.frameCount > 0) UnloadSound(snd_level_);
        if (snd_welldone_.frameCount > 0) UnloadSound(snd_welldone_);
        CloseAudioDevice();
    }
}

Sound AudioManager::load_safe(const std::string& path) {
    if (!FileExists(path.c_str())) {
        std::cerr << "Warning: sound not found: " << path << std::endl;
        return Sound{};
    }
    return LoadSound(path.c_str());
}

void AudioManager::play_step() {
    if (enabled_ && inited_ && snd_blub_.frameCount > 0) PlaySound(snd_blub_);
}

void AudioManager::play_rotate() {
    if (enabled_ && inited_ && snd_wozz_.frameCount > 0) PlaySound(snd_wozz_);
}

void AudioManager::play_drop() {
    if (enabled_ && inited_ && snd_tchh_.frameCount > 0) PlaySound(snd_tchh_);
}

void AudioManager::play_line() {
    if (enabled_ && inited_ && snd_line_.frameCount > 0) PlaySound(snd_line_);
}

void AudioManager::play_empty() {
    if (enabled_ && inited_ && snd_empty_.frameCount > 0) PlaySound(snd_empty_);
}

void AudioManager::play_hit() {
    if (enabled_ && inited_ && snd_hit_.frameCount > 0) PlaySound(snd_hit_);
}

void AudioManager::play_level() {
    if (enabled_ && inited_ && snd_level_.frameCount > 0) PlaySound(snd_level_);
}

void AudioManager::play_game_over() {
    if (enabled_ && inited_ && snd_welldone_.frameCount > 0) PlaySound(snd_welldone_);
}

} // namespace blockout
