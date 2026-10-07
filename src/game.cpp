#include "game.hpp"
#include <fstream>
#include <cmath>
#include <algorithm>

namespace blockout {

Game::Game(AudioManager& audio, const Config& cfg)
    : preview_next_piece_(cfg.preview_next_piece),
      audio_(audio),
      pit_(cfg.width, cfg.length, cfg.depth),
      difficulty_(parse_difficulty(cfg.difficulty)),
      start_level_(difficulty_start_level(difficulty_)),
      step_times_(cfg.step_times) {
    load_high_score();
    reset();
}

void Game::reset() {
    pit_.clear();
    piece_mgr_.reset_bag();
    score_ = 0;
    level_ = start_level_;
    lines_cleared_ = 0;
    cubes_placed_ = 0;
    fall_timer_ = 0.0f;
    spark_timer_ = 0.0f;
    state_ = GameState::Playing;

    initial_step_time_ = step_times_.get(difficulty_);
    step_time_ = initial_step_time_;

    next_piece_ = piece_mgr_.draw_next();
    spawn_piece();
}

void Game::spawn_piece() {
    current_piece_ = next_piece_;
    next_piece_ = piece_mgr_.draw_next();
    rot_ = Mat3i::identity();

    // Center piece at the top opening
    pos_.x = (pit_.width() - current_piece_.width) / 2;
    pos_.y = (pit_.height() - current_piece_.height) / 2;
    pos_.z = 0;

    was_dropped_ = false;
    drop_start_z_ = 0;

    // Check game over collision upon spawn
    auto cubes = get_active_cubes();
    if (pit_.is_overlap(cubes)) {
        state_ = GameState::GameOver;
        audio_.play_game_over();
    }
}

void Game::update(float dt) {
    if (spark_timer_ > 0.0f) {
        spark_timer_ -= dt;
        if (spark_timer_ < 0.0f) spark_timer_ = 0.0f;
    }

    if (state_ != GameState::Playing) {
        return;
    }

    fall_timer_ += dt;
    if (fall_timer_ >= step_time_) {
        fall_timer_ = 0.0f;

        // Try stepping down
        Vec3i next_pos = {pos_.x, pos_.y, pos_.z + 1};
        if (!pit_.is_overlap(current_piece_.get_transformed(rot_, next_pos))) {
            pos_ = next_pos;
            audio_.play_step();
        } else {
            lock_piece();
        }
    }
}

void Game::move(int dx, int dy) {
    if (state_ != GameState::Playing) return;

    Vec3i next_pos = {pos_.x + dx, pos_.y + dy, pos_.z};
    if (!pit_.is_overlap(current_piece_.get_transformed(rot_, next_pos))) {
        pos_ = next_pos;
    }
}

bool Game::try_rotate(const Mat3i& target_rot) {
    if (state_ != GameState::Playing) return false;

    Vec3i col_pos, max_nudge;
    if (!pit_.is_overlap(current_piece_.get_transformed(target_rot, pos_), &col_pos, &max_nudge)) {
        rot_ = target_rot;
        audio_.play_rotate();
        return true;
    }

    // Try wall-kick nudge
    Vec3i kicked = pos_ + max_nudge;
    if (!pit_.is_overlap(current_piece_.get_transformed(target_rot, kicked))) {
        pos_ = kicked;
        rot_ = target_rot;
        audio_.play_rotate();
        return true;
    }

    // Blocked rotation - trigger collision effect
    spark_pos_ = col_pos;
    spark_timer_ = SPARK_TIME;
    audio_.play_hit();
    return false;
}

void Game::rotate_x(int dir) {
    try_rotate(rot_.multiply(Mat3i::rot_x(dir)));
}

void Game::rotate_y(int dir) {
    try_rotate(rot_.multiply(Mat3i::rot_y(dir)));
}

void Game::rotate_z(int dir) {
    try_rotate(rot_.multiply(Mat3i::rot_z(dir)));
}

void Game::hard_drop() {
    if (state_ != GameState::Playing) return;

    drop_start_z_ = pos_.z;
    was_dropped_ = true;

    while (true) {
        Vec3i next_pos = {pos_.x, pos_.y, pos_.z + 1};
        if (pit_.is_overlap(current_piece_.get_transformed(rot_, next_pos))) {
            break;
        }
        pos_ = next_pos;
    }

    lock_piece();
}

void Game::lock_piece() {
    auto cubes = get_active_cubes();
    pit_.add_cubes(cubes, current_piece_.id + 1);
    audio_.play_drop();

    cubes_placed_ += static_cast<int>(cubes.size());

    int cleared = pit_.remove_full_lines();
    bool pit_empty = pit_.is_empty();

    if (cleared > 0) {
        lines_cleared_ += cleared;
        if (pit_empty) {
            audio_.play_empty();
        } else {
            audio_.play_line();
        }
    }

    compute_score(cleared, pit_empty);

    // Check level progression
    int cube_per_level = pit_.height() * 15 + pit_.width() * 15;
    if (cubes_placed_ >= cube_per_level * (level_ - start_level_ + 1) && level_ < 10) {
        level_++;
        int diff_levels = level_ - start_level_;
        step_time_ = std::max(0.05f, initial_step_time_ * std::pow(TIME_LEVEL_FACTOR, static_cast<float>(diff_levels)));
        audio_.play_level();
    }

    spawn_piece();
}

void Game::compute_score(int lines_removed, bool pit_empty) {
    int drop_dist = was_dropped_ ? (pit_.depth() - 1 - drop_start_z_) : 0;
    if (drop_dist < 0) drop_dist = 0;

    float fPos = static_cast<float>(drop_dist) / static_cast<float>(pit_.depth() - 1);
    float lScore = static_cast<float>(current_piece_.low_score);
    float hScore = static_cast<float>(current_piece_.high_score);
    float pScore = (lScore + (hScore - lScore) * fPos) * P_LEVEL_FACTOR[level_];

    float lineScore = 0.0f;
    if (lines_removed > 0 && lines_removed <= 5) {
        lineScore = LINE_BASE_FLAT * L_LEVEL_FACTOR[level_] * L_NUMBER_FACTOR[lines_removed];
    }

    float pitScore = 0.0f;
    if (pit_empty) {
        pitScore = LINE_BASE_FLAT * L_LEVEL_FACTOR[level_] * L_NUMBER_FACTOR[2];
    }

    float depth_factor = (pit_.depth() < static_cast<int>(DEPTH_FACTOR.size())) ?
                          DEPTH_FACTOR[pit_.depth()] : 1.0f;
    float total = (lineScore + pScore + pitScore) * depth_factor;

    int points = std::max(1, static_cast<int>(std::round(total)));
    score_ += points;

    size_t diff_idx = static_cast<size_t>(difficulty_);
    if (score_ > high_scores_[diff_idx]) {
        high_scores_[diff_idx] = score_;
        save_high_score();
    }
}

std::vector<Vec3i> Game::get_active_cubes() const {
    return current_piece_.get_transformed(rot_, pos_);
}

std::vector<Vec3i> Game::get_ghost_cubes() const {
    if (state_ != GameState::Playing) return {};
    Vec3i gpos = pos_;
    while (true) {
        Vec3i next_pos = {gpos.x, gpos.y, gpos.z + 1};
        if (pit_.is_overlap(current_piece_.get_transformed(rot_, next_pos))) {
            break;
        }
        gpos = next_pos;
    }
    return current_piece_.get_transformed(rot_, gpos);
}

void Game::toggle_pause() {
    if (state_ == GameState::Playing) {
        state_ = GameState::Paused;
    } else if (state_ == GameState::Paused) {
        state_ = GameState::Playing;
    }
}

void Game::load_high_score() {
    std::ifstream file("highscore.txt");
    if (!file.is_open()) return;

    std::string line;
    bool found_named_entries = false;

    while (std::getline(file, line)) {
        auto start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        auto end = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(start, end - start + 1);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        auto colon = trimmed.find(':');
        if (colon != std::string::npos) {
            found_named_entries = true;
            std::string key = trimmed.substr(0, colon);
            std::string val_str = trimmed.substr(colon + 1);

            auto ks = key.find_first_not_of(" \t");
            auto ke = key.find_last_not_of(" \t");
            if (ks != std::string::npos) key = key.substr(ks, ke - ks + 1);
            for (char& c : key) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            int val = 0;
            try { val = std::stoi(val_str); } catch (...) {}

            if (key == "easy") high_scores_[static_cast<size_t>(Difficulty::Easy)] = val;
            else if (key == "normal") high_scores_[static_cast<size_t>(Difficulty::Normal)] = val;
            else if (key == "hard") high_scores_[static_cast<size_t>(Difficulty::Hard)] = val;
            else if (key == "extreme") high_scores_[static_cast<size_t>(Difficulty::Extreme)] = val;
        } else if (!found_named_entries) {
            // Legacy single integer file (attribute to easy)
            try {
                int legacy_val = std::stoi(trimmed);
                high_scores_[static_cast<size_t>(Difficulty::Easy)] = legacy_val;
                return;
            } catch (...) {}
        }
    }
}

void Game::save_high_score() {
    std::ofstream file("highscore.txt");
    if (file.is_open()) {
        file << "easy: " << high_scores_[static_cast<size_t>(Difficulty::Easy)] << "\n";
        file << "normal: " << high_scores_[static_cast<size_t>(Difficulty::Normal)] << "\n";
        file << "hard: " << high_scores_[static_cast<size_t>(Difficulty::Hard)] << "\n";
        file << "extreme: " << high_scores_[static_cast<size_t>(Difficulty::Extreme)] << "\n";
    }
}

} // namespace blockout
