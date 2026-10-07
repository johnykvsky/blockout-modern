#pragma once

#include "raylib.h"
#include <string>
#include <unordered_map>

namespace blockout {

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    void play_step();
    void play_rotate();
    void play_drop();
    void play_line();
    void play_empty();
    void play_hit();
    void play_level();
    void play_game_over();

    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }

private:
    bool enabled_ = true;
    bool inited_ = false;

    Sound snd_blub_{};
    Sound snd_wozz_{};
    Sound snd_tchh_{};
    Sound snd_line_{};
    Sound snd_empty_{};
    Sound snd_hit_{};
    Sound snd_level_{};
    Sound snd_welldone_{};

    Sound load_safe(const std::string& path);
};

} // namespace blockout
