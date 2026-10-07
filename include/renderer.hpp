#pragma once

#include "raylib.h"
#include "game.hpp"

namespace blockout {

class Renderer {
public:
    Renderer(int screen_width, int screen_height);

    void draw(const Game& game, bool quit_requested = false);

private:
    int screen_width_;
    int screen_height_;
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
    void draw_hud(const Game& game, bool quit_requested = false);

    Color get_piece_color(int id, float alpha = 1.0f) const;
};

} // namespace blockout
