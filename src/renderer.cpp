#include "renderer.hpp"
#include <string>
#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

namespace blockout {

Renderer::Renderer(int screen_width, int screen_height)
    : screen_width_(screen_width), screen_height_(screen_height) {
    camera_.up = {0.0f, 1.0f, 0.0f};
    camera_.fovy = 50.0f;
    camera_.projection = CAMERA_PERSPECTIVE;
}

void Renderer::update_camera(const Pit& pit) {
    float panel_w = 300.0f;
    float play_area_w = static_cast<float>(screen_width_) - panel_w;
    float aspect_play = play_area_w / static_cast<float>(screen_height_);
    float half_fov_rad = 50.0f * 0.5f * (3.14159265f / 180.0f);

    float half_w = (static_cast<float>(pit.width()) * 0.5f) * 1.25f;
    float half_h = (static_cast<float>(pit.height()) * 0.5f) * 1.25f;

    float req_dist_v = half_h / std::tan(half_fov_rad);
    float req_dist_h = half_w / (std::tan(half_fov_rad) * std::max(0.1f, aspect_play));
    float dist = std::max(req_dist_v, req_dist_h);

    // Exact horizontal offset to center pit in the left play area (screen_width_ - 300)
    float offset_x = -(panel_w / static_cast<float>(screen_height_)) * dist * std::tan(half_fov_rad);

    camera_.position = {offset_x, 0.0f, -0.5f - dist};
    camera_.target   = {offset_x, 0.0f, static_cast<float>(pit.depth()) * 0.5f};
}

Vector3 Renderer::grid_to_world(int gx, int gy, int gz, int pit_w, int pit_h) const {
    // Looking along +Z: Screen Right is -X in world space, Screen Left is +X in world space.
    // gx = 0 is left (+wx), gx = pit_w - 1 is right (-wx).
    float wx = (static_cast<float>(pit_w - 1) * 0.5f - static_cast<float>(gx)) * cube_size_;
    // gy = 0 is top (+wy), gy = pit_h - 1 is bottom (-wy).
    float wy = (static_cast<float>(pit_h - 1) * 0.5f - static_cast<float>(gy)) * cube_size_;
    float wz = static_cast<float>(gz) * cube_size_;
    return {wx, wy, wz};
}

Vector3 Renderer::vertex_to_world(float vx, float vy, float vz, int pit_w, int pit_h) const {
    // Vertex coordinates:
    // vx = 0 is left (+wx = 0.5 * pit_w * cube_size_), vx = pit_w is right (-wx).
    // vy = 0 is top (+wy = 0.5 * pit_h * cube_size_), vy = pit_h is bottom (-wy).
    // vz = 0 is front (-0.5 * cube_size_), vz = pit_d is back ((pit_d - 0.5) * cube_size_).
    float wx = (static_cast<float>(pit_w) * 0.5f - vx) * cube_size_;
    float wy = (static_cast<float>(pit_h) * 0.5f - vy) * cube_size_;
    float wz = (vz - 0.5f) * cube_size_;
    return {wx, wy, wz};
}

Color Renderer::get_piece_color(int id, float alpha) const {
    Color base_colors[] = {
        {229, 192, 123, 255}, // 0: Gold
        {86, 182, 194, 255},  // 1: Cyan
        {152, 195, 121, 255}, // 2: Lime
        {209, 154, 102, 255}, // 3: Orange
        {97, 175, 239, 255},  // 4: Sky Blue
        {198, 120, 221, 255}, // 5: Purple
        {224, 108, 117, 255}, // 6: Coral Red
        {78, 201, 176, 255}   // 7: Teal
    };
    Color c = base_colors[std::abs(id) % 8];
    return Fade(c, alpha);
}

void Renderer::draw_pit_wireframe(const Pit& pit) {
    float hw = static_cast<float>(pit.width()) * 0.5f * cube_size_;
    float hh = static_cast<float>(pit.height()) * 0.5f * cube_size_;
    float z_front = -0.5f * cube_size_;
    float z_back = (static_cast<float>(pit.depth()) - 0.5f) * cube_size_;

    // 1. Longitudinal grid lines along Top and Bottom walls (dividing 5 vertical columns)
    for (int x = 0; x <= pit.width(); ++x) {
        float wx = -hw + static_cast<float>(x) * cube_size_;
        bool is_corner = (x == 0 || x == pit.width());
        Color col = is_corner ? Color{50, 180, 255, 255} : Color{40, 130, 200, 140};
        DrawLine3D({wx,  hh, z_front}, {wx,  hh, z_back}, col);
        DrawLine3D({wx, -hh, z_front}, {wx, -hh, z_back}, col);
    }

    // 2. Longitudinal grid lines along Left and Right walls (dividing 5 horizontal rows)
    for (int y = 0; y <= pit.height(); ++y) {
        float wy = -hh + static_cast<float>(y) * cube_size_;
        bool is_corner = (y == 0 || y == pit.height());
        Color col = is_corner ? Color{50, 180, 255, 255} : Color{40, 130, 200, 140};
        DrawLine3D({-hw, wy, z_front}, {-hw, wy, z_back}, col);
        DrawLine3D({ hw, wy, z_front}, { hw, wy, z_back}, col);
    }

    // 3. Depth perimeter rings at each slice z in [0, pit.depth()]
    for (int z = 0; z <= pit.depth(); ++z) {
        float wz = (static_cast<float>(z) - 0.5f) * cube_size_;
        Color ring_col;
        if (z == 0) {
            ring_col = {0, 255, 200, 255}; // Bold neon opening
        } else if (z == pit.depth()) {
            ring_col = {0, 220, 255, 240}; // Back floor perimeter
        } else {
            ring_col = {30, 95, 150, 85};  // Subtle depth rings
        }
        DrawLine3D({-hw, -hh, wz}, { hw, -hh, wz}, ring_col);
        DrawLine3D({ hw, -hh, wz}, { hw,  hh, wz}, ring_col);
        DrawLine3D({ hw,  hh, wz}, {-hw,  hh, wz}, ring_col);
        DrawLine3D({-hw,  hh, wz}, {-hw, -hh, wz}, ring_col);
    }

    // 4. Back floor grid
    Color floor_grid_col = {40, 140, 210, 160};
    for (int x = 1; x < pit.width(); ++x) {
        float wx = -hw + static_cast<float>(x) * cube_size_;
        DrawLine3D({wx, -hh, z_back}, {wx, hh, z_back}, floor_grid_col);
    }
    for (int y = 1; y < pit.height(); ++y) {
        float wy = -hh + static_cast<float>(y) * cube_size_;
        DrawLine3D({-hw, wy, z_back}, {hw, wy, z_back}, floor_grid_col);
    }
}

void Renderer::draw_cubes(const Pit& pit) {
    float sz = cube_size_ * 0.94f;
    for (int z = 0; z < pit.depth(); ++z) {
        for (int y = 0; y < pit.height(); ++y) {
            for (int x = 0; x < pit.width(); ++x) {
                int val = pit.get(x, y, z);
                if (val > 0) {
                    Vector3 pos = grid_to_world(x, y, z, pit.width(), pit.height());
                    Color col = get_piece_color(val - 1, 0.85f);
                    DrawCube(pos, sz, sz, sz, col);
                    DrawCubeWires(pos, sz, sz, sz, Fade(WHITE, 0.6f));
                }
            }
        }
    }
}

std::vector<Renderer::Edge3D> Renderer::get_outline_edges(
    const std::vector<Vec3i>& cubes, int pit_w, int pit_h) const {
    if (cubes.empty()) return {};

    auto contains = [&](int x, int y, int z) -> bool {
        for (const auto& c : cubes) {
            if (c.x == x && c.y == y && c.z == z) return true;
        }
        return false;
    };

    struct UnitEdge {
        int axis; // 0=X, 1=Y, 2=Z
        int x, y, z;
        auto operator<=>(const UnitEdge&) const = default;
    };

    std::set<UnitEdge> candidate_edges;
    for (const auto& c : cubes) {
        // 4 X-edges
        candidate_edges.insert({0, c.x, c.y, c.z});
        candidate_edges.insert({0, c.x, c.y + 1, c.z});
        candidate_edges.insert({0, c.x, c.y, c.z + 1});
        candidate_edges.insert({0, c.x, c.y + 1, c.z + 1});

        // 4 Y-edges
        candidate_edges.insert({1, c.x, c.y, c.z});
        candidate_edges.insert({1, c.x + 1, c.y, c.z});
        candidate_edges.insert({1, c.x, c.y, c.z + 1});
        candidate_edges.insert({1, c.x + 1, c.y, c.z + 1});

        // 4 Z-edges
        candidate_edges.insert({2, c.x, c.y, c.z});
        candidate_edges.insert({2, c.x + 1, c.y, c.z});
        candidate_edges.insert({2, c.x, c.y + 1, c.z});
        candidate_edges.insert({2, c.x + 1, c.y + 1, c.z});
    }

    std::map<std::pair<int, int>, std::vector<int>> x_lines; // (y, z) -> [x]
    std::map<std::pair<int, int>, std::vector<int>> y_lines; // (x, z) -> [y]
    std::map<std::pair<int, int>, std::vector<int>> z_lines; // (x, y) -> [z]

    for (const auto& e : candidate_edges) {
        bool b0 = false, b1 = false, b2 = false, b3 = false;
        if (e.axis == 0) {
            b0 = contains(e.x, e.y - 1, e.z - 1);
            b1 = contains(e.x, e.y,     e.z - 1);
            b2 = contains(e.x, e.y,     e.z);
            b3 = contains(e.x, e.y - 1, e.z);
        } else if (e.axis == 1) {
            b0 = contains(e.x - 1, e.y, e.z - 1);
            b1 = contains(e.x,     e.y, e.z - 1);
            b2 = contains(e.x,     e.y, e.z);
            b3 = contains(e.x - 1, e.y, e.z);
        } else {
            b0 = contains(e.x - 1, e.y - 1, e.z);
            b1 = contains(e.x,     e.y - 1, e.z);
            b2 = contains(e.x,     e.y,     e.z);
            b3 = contains(e.x - 1, e.y,     e.z);
        }

        int k = (b0 ? 1 : 0) + (b1 ? 1 : 0) + (b2 ? 1 : 0) + (b3 ? 1 : 0);
        // An edge is an exterior outline if k == 1 (convex 90 deg corner),
        // k == 3 (concave 90 deg corner), or k == 2 with diagonally opposite voxels (b0 == b2).
        // If k == 2 and voxels are adjacent (b0 != b2), it is a flat coplanar internal seam.
        bool is_outline = (k == 1) || (k == 3) || (k == 2 && (b0 == b2));
        if (is_outline) {
            if (e.axis == 0) x_lines[{e.y, e.z}].push_back(e.x);
            else if (e.axis == 1) y_lines[{e.x, e.z}].push_back(e.y);
            else z_lines[{e.x, e.y}].push_back(e.z);
        }
    }

    std::vector<Edge3D> result;

    auto merge_and_emit = [&](const auto& line_map, int axis) {
        for (const auto& [fixed, vals] : line_map) {
            std::vector<int> sorted_vals = vals;
            std::sort(sorted_vals.begin(), sorted_vals.end());

            size_t i = 0;
            while (i < sorted_vals.size()) {
                int start = sorted_vals[i];
                int end = start;
                while (i + 1 < sorted_vals.size() && sorted_vals[i + 1] == end + 1) {
                    end = sorted_vals[i + 1];
                    ++i;
                }
                ++i;

                Vector3 p1{}, p2{};
                if (axis == 0) {
                    p1 = vertex_to_world(static_cast<float>(start), static_cast<float>(fixed.first), static_cast<float>(fixed.second), pit_w, pit_h);
                    p2 = vertex_to_world(static_cast<float>(end + 1), static_cast<float>(fixed.first), static_cast<float>(fixed.second), pit_w, pit_h);
                } else if (axis == 1) {
                    p1 = vertex_to_world(static_cast<float>(fixed.first), static_cast<float>(start), static_cast<float>(fixed.second), pit_w, pit_h);
                    p2 = vertex_to_world(static_cast<float>(fixed.first), static_cast<float>(end + 1), static_cast<float>(fixed.second), pit_w, pit_h);
                } else {
                    p1 = vertex_to_world(static_cast<float>(fixed.first), static_cast<float>(fixed.second), static_cast<float>(start), pit_w, pit_h);
                    p2 = vertex_to_world(static_cast<float>(fixed.first), static_cast<float>(fixed.second), static_cast<float>(end + 1), pit_w, pit_h);
                }
                result.push_back({p1, p2});
            }
        }
    };

    merge_and_emit(x_lines, 0);
    merge_and_emit(y_lines, 1);
    merge_and_emit(z_lines, 2);

    return result;
}

void Renderer::draw_active_piece(const Game& game) {
    Color edge_col = get_piece_color(game.current_piece().id, 1.0f);
    auto cubes = game.get_active_cubes();
    auto edges = get_outline_edges(cubes, game.pit().width(), game.pit().height());

    for (const auto& e : edges) {
        DrawLine3D(e.p1, e.p2, edge_col);
    }
}

void Renderer::draw_ghost_piece(const Game& game) {
    float sz = cube_size_ * 0.94f;
    Color ghost_col = get_piece_color(game.current_piece().id, 0.22f);
    Color wire_col = Fade(WHITE, 0.40f);
    auto ghost_cubes = game.get_ghost_cubes();

    for (const auto& c : ghost_cubes) {
        Vector3 pos = grid_to_world(c.x, c.y, c.z, game.pit().width(), game.pit().height());
        DrawCube(pos, sz, sz, sz, ghost_col);
    }

    auto edges = get_outline_edges(ghost_cubes, game.pit().width(), game.pit().height());
    for (const auto& e : edges) {
        DrawLine3D(e.p1, e.p2, wire_col);
    }
}

void Renderer::draw_sparks(const Game& game) {
    if (!game.has_spark()) return;

    Vector3 p = grid_to_world(game.spark_pos().x, game.spark_pos().y, game.spark_pos().z,
                              game.pit().width(), game.pit().height());
    float prog = game.spark_progress();
    float radius = 0.6f * prog;
    Color spark_col = Fade(ORANGE, 1.0f - prog);

    DrawSphereWires(p, radius, 8, 8, spark_col);
    DrawLine3D({p.x - radius, p.y, p.z}, {p.x + radius, p.y, p.z}, YELLOW);
    DrawLine3D({p.x, p.y - radius, p.z}, {p.x, p.y + radius, p.z}, YELLOW);
    DrawLine3D({p.x, p.y, p.z - radius}, {p.x, p.y, p.z + radius}, YELLOW);
}

void Renderer::draw_hud(const Game& game, bool quit_requested) {
    int panel_x = screen_width_ - 300;
    int panel_w = 300;

    // Solid background for HUD sidebar
    DrawRectangle(panel_x, 0, panel_w, screen_height_, {15, 20, 30, 255});
    DrawLine(panel_x, 0, panel_x, screen_height_, {40, 100, 160, 255});

    // Title
    int title_y = (screen_height_ < 720) ? 20 : 30;
    DrawText("BLOCKOUT", panel_x + 35, title_y, 36, {0, 220, 255, 255});
    DrawText("MODERN C++20", panel_x + 37, title_y + 40, 14, {140, 170, 200, 200});
    DrawText(TextFormat("PIT: %dx%dx%d", game.pit().width(), game.pit().height(), game.pit().depth()),
             panel_x + 37, title_y + 60, 16, {100, 230, 255, 255});

    // Score Board
    int y = (screen_height_ < 720) ? 95 : 125;
    DrawText("SCORE", panel_x + 30, y, 16, {140, 170, 200, 255});
    DrawText(TextFormat("%08d", game.score()), panel_x + 30, y + 24, 30, {255, 235, 120, 255});

    int step_score = (screen_height_ < 720) ? 60 : 75;
    int step_stat  = (screen_height_ < 720) ? 50 : 62;

    y += step_score;
    DrawText("HIGH SCORE", panel_x + 30, y, 16, {140, 170, 200, 255});
    DrawText(TextFormat("%08d", game.high_score()), panel_x + 30, y + 24, 26, {200, 200, 200, 255});

    y += step_stat + 8;
    DrawText("LEVEL", panel_x + 30, y, 16, {140, 170, 200, 255});
    DrawText(TextFormat("%d", game.level()), panel_x + 30, y + 22, 24, {100, 255, 160, 255});

    y += step_stat;
    DrawText("LAYERS CLEARED", panel_x + 30, y, 16, {140, 170, 200, 255});
    DrawText(TextFormat("%d", game.lines_cleared()), panel_x + 30, y + 22, 24, WHITE);

    y += step_stat;
    DrawText("CUBES PLACED", panel_x + 30, y, 16, {140, 170, 200, 255});
    DrawText(TextFormat("%d", game.cubes_placed()), panel_x + 30, y + 22, 24, WHITE);

    // Controls Legend Card
    int card_y = (screen_height_ < 720) ? y + 50 : y + 65;
    int card_h = (screen_height_ < 720) ? 218 : 230;
    DrawRectangle(panel_x + 18, card_y, panel_w - 36, card_h, Fade({25, 35, 50, 255}, 0.7f));
    DrawRectangleLines(panel_x + 18, card_y, panel_w - 36, card_h, Fade({60, 120, 180, 255}, 0.4f));

    int item_y = card_y + 8;
    int cstep = (screen_height_ < 720) ? 19 : 21;
    DrawText("CONTROLS", panel_x + 30, item_y, 16, {0, 220, 255, 255}); item_y += cstep + 2;
    DrawText("Move: Arrows", panel_x + 30, item_y, 14, RAYWHITE); item_y += cstep;
    DrawText("Pitch (X): Q / A", panel_x + 30, item_y, 14, RAYWHITE); item_y += cstep;
    DrawText("Yaw   (Y): W / S", panel_x + 30, item_y, 14, RAYWHITE); item_y += cstep;
    DrawText("Roll  (Z): E / D", panel_x + 30, item_y, 14, RAYWHITE); item_y += cstep;
    DrawText("Hard Drop: SPACE", panel_x + 30, item_y, 14, {255, 210, 80, 255}); item_y += cstep;
    DrawText(TextFormat("Shadow (G): %s", game.show_ghost() ? "ON" : "OFF"),
             panel_x + 30, item_y, 14, game.show_ghost() ? Color{100, 255, 160, 255} : Color{180, 180, 180, 255}); item_y += cstep;
    DrawText("Fullscreen: F11", panel_x + 30, item_y, 14, {140, 200, 255, 255}); item_y += cstep;
    DrawText("Pause: P", panel_x + 30, item_y, 14, RAYWHITE); item_y += cstep;
    DrawText("Quit: ESC", panel_x + 30, item_y, 14, {255, 120, 120, 255});

    // Modals (centered over left play area)
    int play_area_w = screen_width_ - 300;
    int cx = play_area_w / 2;
    int cy = screen_height_ / 2;

    if (quit_requested) {
        int modal_w = 400;
        int modal_h = 180;
        int mx = cx - modal_w / 2;
        int my = cy - modal_h / 2;

        // Dark modal card with amber/gold border
        DrawRectangle(mx, my, modal_w, modal_h, Fade({12, 16, 26, 255}, 0.96f));
        DrawRectangleLines(mx, my, modal_w, modal_h, {255, 170, 50, 255});
        DrawRectangleLines(mx + 1, my + 1, modal_w - 2, modal_h - 2, {255, 120, 40, 180});

        DrawText("QUIT GAME?", cx - 85, my + 22, 28, {255, 200, 60, 255});
        DrawText("Are you sure you want to exit?", cx - 128, my + 62, 17, RAYWHITE);

        // Interactive Yes and No buttons
        Rectangle btn_yes = {static_cast<float>(cx - 135), static_cast<float>(my + 108), 120.0f, 44.0f};
        Rectangle btn_no  = {static_cast<float>(cx + 15),  static_cast<float>(my + 108), 120.0f, 44.0f};

        Vector2 mouse = GetMousePosition();
        bool hover_yes = CheckCollisionPointRec(mouse, btn_yes);
        bool hover_no  = CheckCollisionPointRec(mouse, btn_no);

        // Yes Button
        DrawRectangleRec(btn_yes, hover_yes ? Color{210, 60, 60, 255} : Color{140, 35, 35, 220});
        DrawRectangleLinesEx(btn_yes, 2, hover_yes ? Color{255, 140, 140, 255} : Color{220, 80, 80, 255});
        DrawText("YES (Y)", static_cast<int>(btn_yes.x) + 26, static_cast<int>(btn_yes.y) + 13, 17, WHITE);

        // No Button
        DrawRectangleRec(btn_no, hover_no ? Color{40, 160, 80, 255} : Color{25, 110, 55, 220});
        DrawRectangleLinesEx(btn_no, 2, hover_no ? Color{120, 255, 160, 255} : Color{60, 200, 100, 255});
        DrawText("NO (N)", static_cast<int>(btn_no.x) + 30, static_cast<int>(btn_no.y) + 13, 17, WHITE);
    } else if (game.state() == GameState::Paused) {
        DrawRectangle(cx - 160, cy - 60, 320, 120, Fade({10, 15, 25, 255}, 0.92f));
        DrawRectangleLines(cx - 160, cy - 60, 320, 120, {0, 220, 255, 255});
        DrawText("PAUSED", cx - 65, cy - 35, 32, {255, 230, 100, 255});
        DrawText("Press P to Resume", cx - 80, cy + 15, 18, RAYWHITE);
    } else if (game.state() == GameState::GameOver) {
        DrawRectangle(cx - 180, cy - 70, 360, 140, Fade({15, 10, 15, 255}, 0.94f));
        DrawRectangleLines(cx - 180, cy - 70, 360, 140, {255, 80, 80, 255});
        DrawText("GAME OVER", cx - 95, cy - 45, 34, {255, 80, 80, 255});
        DrawText(TextFormat("Final Score: %d", game.score()), cx - 80, cy, 20, RAYWHITE);
        DrawText("Press R or ENTER to Restart", cx - 125, cy + 30, 18, {100, 255, 160, 255});
    }
}

void Renderer::draw(const Game& game, bool quit_requested) {
    screen_width_ = GetScreenWidth();
    screen_height_ = GetScreenHeight();

    BeginDrawing();
    ClearBackground({10, 12, 18, 255});

    // 1. Draw 3D scene (camera dynamically adapts to pit dimensions and centers in play area)
    update_camera(game.pit());
    BeginMode3D(camera_);
    draw_pit_wireframe(game.pit());
    draw_cubes(game.pit());
    if (game.state() != GameState::GameOver) {
        if (game.show_ghost()) {
            draw_ghost_piece(game);
        }
        draw_active_piece(game);
    }
    draw_sparks(game);
    EndMode3D();

    // 2. Draw 2D HUD sidebar on the right side
    draw_hud(game, quit_requested);

    EndDrawing();
}

} // namespace blockout
