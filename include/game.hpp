#pragma once

#include "types.hpp"
#include "pieces.hpp"
#include "pit.hpp"
#include "audio.hpp"
#include "config.hpp"
#include <string>

namespace blockout {

class Game {
public:
    Game(AudioManager& audio, const Config& cfg = Config{});

    void reset();
    void update(float dt);

    // Inputs
    void move(int dx, int dy);
    void rotate_x(int dir);
    void rotate_y(int dir);
    void rotate_z(int dir);
    void hard_drop();
    void toggle_pause();

    // Accessors for rendering
    const Pit& pit() const { return pit_; }
    const Piece& current_piece() const { return current_piece_; }
    const Piece& next_piece() const { return next_piece_; }
    bool preview_next_piece() const { return preview_next_piece_; }
    Vec3i piece_pos() const { return pos_; }
    Mat3i piece_rot() const { return rot_; }
    std::vector<Vec3i> get_active_cubes() const;
    std::vector<Vec3i> get_ghost_cubes() const;

    GameState state() const { return state_; }
    int score() const { return score_; }
    int high_score() const { return high_score_; }
    int level() const { return level_; }
    int lines_cleared() const { return lines_cleared_; }
    int cubes_placed() const { return cubes_placed_; }

    bool has_spark() const { return spark_timer_ > 0.0f; }
    Vec3i spark_pos() const { return spark_pos_; }
    float spark_progress() const { return 1.0f - (spark_timer_ / SPARK_TIME); }

    bool show_ghost() const { return show_ghost_; }
    void toggle_ghost() { show_ghost_ = !show_ghost_; }

private:
    bool show_ghost_ = false;
    bool preview_next_piece_ = false;
    AudioManager& audio_;
    Pit pit_;
    PieceManager piece_mgr_;

    Piece current_piece_{};
    Piece next_piece_{};
    Vec3i pos_{};
    Mat3i rot_{};

    GameState state_ = GameState::Playing;

    float fall_timer_ = 0.0f;
    float step_time_ = 1.0f;
    int drop_start_z_ = 0;
    bool was_dropped_ = false;

    int score_ = 0;
    int high_score_ = 0;
    int level_ = 0;
    int lines_cleared_ = 0;
    int cubes_placed_ = 0;

    float spark_timer_ = 0.0f;
    Vec3i spark_pos_{};

    void spawn_piece();
    void lock_piece();
    void compute_score(int lines_removed, bool pit_empty);
    bool try_rotate(const Mat3i& target_rot);

    void load_high_score();
    void save_high_score();
};

} // namespace blockout
