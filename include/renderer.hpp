#pragma once

#include "raylib.h"
#include "game.hpp"

namespace blockout {

class Renderer {
public:
    Renderer(int screen_width, int screen_height,
             const ColorTheme& theme = ColorTheme::default_theme(),
             bool color_by_layer = true);

    void set_theme(const ColorTheme& theme) { theme_ = theme; }
    const ColorTheme& theme() const { return theme_; }

    void set_color_by_layer(bool enabled) { color_by_layer_ = enabled; }
    bool color_by_layer() const { return color_by_layer_; }

    void draw(const Game& game, bool quit_requested = false, bool restart_requested = false);

private:
    int screen_width_;
    int screen_height_;
    ColorTheme theme_;
    bool color_by_layer_ = true;
    Camera3D camera_{};
    float cube_size_ = 1.0f;

    struct Edge3D {
        Vector3 p1;
        Vector3 p2;
    };

    Vector3 grid_to_world(int gx, int gy, int gz, int pit_w, int pit_h) const;
    Vector3 vertex_to_world(float vx, float vy, float vz, int pit_w, int pit_h) const;
    std::vector<Edge3D> get_outline_edges(const std::vector<Vec3i>& cubes, int pit_w, int pit_h) const;
    void update_camera(const Pit& pit);

    void draw_pit_wireframe(const Pit& pit);
    void draw_cubes(const Pit& pit);
    void draw_active_piece(const Game& game);
    void draw_ghost_piece(const Game& game);
    void draw_sparks(const Game& game);
    void draw_left_panel(const Game& game);
    void draw_next_piece_preview(const Game& game, int card_x, int card_y, int card_w, int card_h);
    void draw_layer_tower(const Game& game, int tower_x, int tower_y, int tower_w, int tower_h);
    void draw_hud(const Game& game, bool quit_requested = false, bool restart_requested = false);

    Color get_piece_color(int id, float alpha = 1.0f) const;
    Color get_layer_color(int z, int depth, float alpha = 1.0f) const;
};

} // namespace blockout
