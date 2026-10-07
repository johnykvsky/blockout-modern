#include "renderer.hpp"
#include <string>
#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

namespace blockout {

Renderer::Renderer(int screen_width, int screen_height, const ColorTheme& theme, bool color_by_layer)
    : screen_width_(screen_width), screen_height_(screen_height), theme_(theme), color_by_layer_(color_by_layer) {
    camera_.up = {0.0f, 1.0f, 0.0f};
    camera_.fovy = 50.0f;
    camera_.projection = CAMERA_PERSPECTIVE;
}

void Renderer::update_camera(const Pit& pit) {
    float left_panel_w = 180.0f;
    float right_panel_w = 300.0f;
    float play_area_w = static_cast<float>(screen_width_) - left_panel_w - right_panel_w;
    float aspect_play = play_area_w / static_cast<float>(screen_height_);
    float half_fov_rad = 50.0f * 0.5f * (3.14159265f / 180.0f);

    float half_w = (static_cast<float>(pit.width()) * 0.5f) * 1.25f;
    float half_h = (static_cast<float>(pit.height()) * 0.5f) * 1.25f;

    float req_dist_v = half_h / std::tan(half_fov_rad);
    float req_dist_h = half_w / (std::tan(half_fov_rad) * std::max(0.1f, aspect_play));
    float dist = std::max(req_dist_v, req_dist_h);

    // Exact horizontal offset to center pit in the center play area between left (180px) and right (300px) sidebars:
    float offset_x = ((left_panel_w - right_panel_w) / static_cast<float>(screen_height_)) * dist * std::tan(half_fov_rad);

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
    if (theme_.piece_colors.empty()) {
        return Fade(WHITE, alpha);
    }
    Color c = theme_.piece_colors[std::abs(id) % theme_.piece_colors.size()];
    return Fade(c, alpha);
}

Color Renderer::get_layer_color(int z, int depth, float alpha) const {
    if (theme_.layer_colors.empty()) {
        return Fade(WHITE, alpha);
    }
    // Layer index ordered from floor (z = depth - 1) up to opening (z = 0)
    int level_from_floor = (depth - 1) - z;
    if (level_from_floor < 0) level_from_floor = 0;
    int idx = level_from_floor % static_cast<int>(theme_.layer_colors.size());
    return Fade(theme_.layer_colors[idx], alpha);
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
        Color col = is_corner ? theme_.pit_corner : theme_.pit_grid;
        DrawLine3D({wx,  hh, z_front}, {wx,  hh, z_back}, col);
        DrawLine3D({wx, -hh, z_front}, {wx, -hh, z_back}, col);
    }

    // 2. Longitudinal grid lines along Left and Right walls (dividing 5 horizontal rows)
    for (int y = 0; y <= pit.height(); ++y) {
        float wy = -hh + static_cast<float>(y) * cube_size_;
        bool is_corner = (y == 0 || y == pit.height());
        Color col = is_corner ? theme_.pit_corner : theme_.pit_grid;
        DrawLine3D({-hw, wy, z_front}, {-hw, wy, z_back}, col);
        DrawLine3D({ hw, wy, z_front}, { hw, wy, z_back}, col);
    }

    // 3. Depth perimeter rings at each slice z in [0, pit.depth()]
    for (int z = 0; z <= pit.depth(); ++z) {
        float wz = (static_cast<float>(z) - 0.5f) * cube_size_;
        Color ring_col;
        if (z == 0) {
            ring_col = theme_.pit_opening; // Bold neon opening
        } else if (z == pit.depth()) {
            ring_col = theme_.pit_floor_perimeter; // Back floor perimeter
        } else {
            ring_col = theme_.pit_depth_rings;  // Subtle depth rings
        }
        DrawLine3D({-hw, -hh, wz}, { hw, -hh, wz}, ring_col);
        DrawLine3D({ hw, -hh, wz}, { hw,  hh, wz}, ring_col);
        DrawLine3D({ hw,  hh, wz}, {-hw,  hh, wz}, ring_col);
        DrawLine3D({-hw,  hh, wz}, {-hw, -hh, wz}, ring_col);
    }

    // 4. Back floor grid
    Color floor_grid_col = theme_.pit_floor_grid;
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
                    Color col = color_by_layer_
                        ? get_layer_color(z, pit.depth(), 0.85f)
                        : get_piece_color(val - 1, 0.85f);
                    DrawCube(pos, sz, sz, sz, col);
                    DrawCubeWires(pos, sz, sz, sz, theme_.cube_wireframe);
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
    Color edge_col = (theme_.active_wireframe.a > 0)
        ? theme_.active_wireframe
        : get_piece_color(game.current_piece().id, 1.0f);
    auto cubes = game.get_active_cubes();
    auto edges = get_outline_edges(cubes, game.pit().width(), game.pit().height());

    for (const auto& e : edges) {
        DrawLine3D(e.p1, e.p2, edge_col);
    }
}

void Renderer::draw_ghost_piece(const Game& game) {
    float sz = cube_size_ * 0.94f;
    Color ghost_col = get_piece_color(game.current_piece().id, theme_.ghost_alpha);
    auto ghost_cubes = game.get_ghost_cubes();

    for (const auto& c : ghost_cubes) {
        Vector3 pos = grid_to_world(c.x, c.y, c.z, game.pit().width(), game.pit().height());
        DrawCube(pos, sz, sz, sz, ghost_col);
    }

    auto edges = get_outline_edges(ghost_cubes, game.pit().width(), game.pit().height());
    for (const auto& e : edges) {
        DrawLine3D(e.p1, e.p2, theme_.ghost_wireframe);
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

void Renderer::draw_next_piece_preview(const Game& game, int card_x, int card_y, int card_w, int card_h) {
    if (game.state() == GameState::GameOver) return;

    const Piece& next = game.next_piece();
    if (next.cubes.empty()) return;

    // Background card matching HUD styling
    DrawRectangle(card_x, card_y, card_w, card_h, Fade(theme_.hud_background, 0.88f));
    DrawRectangleLines(card_x, card_y, card_w, card_h, theme_.hud_border);

    // Title
    const char* title = "NEXT";
    int title_sz = 15;
    DrawText(title, card_x + (card_w - MeasureText(title, title_sz)) / 2, card_y + 10, title_sz, theme_.hud_title);

    // Calculate bounding box of next piece in default orientation
    int min_x = 999, max_x = -999, min_y = 999, max_y = -999;
    for (const auto& c : next.cubes) {
        if (c.x < min_x) min_x = c.x;
        if (c.x > max_x) max_x = c.x;
        if (c.y < min_y) min_y = c.y;
        if (c.y > max_y) max_y = c.y;
    }
    int pw = max_x - min_x + 1;
    int ph = max_y - min_y + 1;

    int cell_sz = 22;
    int preview_area_top = card_y + 30;
    int preview_area_h = card_h - 30 - 24;

    int cx = card_x + card_w / 2;
    int cy = preview_area_top + preview_area_h / 2;

    int start_x = cx - (pw * cell_sz) / 2;
    int start_y = cy - (ph * cell_sz) / 2;

    Color col = get_piece_color(next.id, 0.9f);

    for (const auto& c : next.cubes) {
        int bx = start_x + (c.x - min_x) * cell_sz;
        int by = start_y + (c.y - min_y) * cell_sz;

        // Filled cube body
        DrawRectangle(bx + 1, by + 1, cell_sz - 2, cell_sz - 2, col);

        // Subtle 3D bevel / border highlight
        DrawLine(bx + 1, by + 1, bx + cell_sz - 2, by + 1, Fade(WHITE, 0.6f));
        DrawLine(bx + 1, by + 1, bx + 1, by + cell_sz - 2, Fade(WHITE, 0.6f));
        DrawLine(bx + cell_sz - 2, by + 1, bx + cell_sz - 2, by + cell_sz - 2, Fade(BLACK, 0.4f));
        DrawLine(bx + 1, by + cell_sz - 2, bx + cell_sz - 2, by + cell_sz - 2, Fade(BLACK, 0.4f));

        DrawRectangleLines(bx, by, cell_sz, cell_sz, Fade({10, 15, 25, 255}, 0.8f));
    }

    // Piece name at bottom of card
    int name_sz = 12;
    DrawText(next.name.c_str(),
             card_x + (card_w - MeasureText(next.name.c_str(), name_sz)) / 2,
             card_y + card_h - 18,
             name_sz,
             theme_.hud_label);
}

void Renderer::draw_layer_tower(const Game& game, int tower_x, int tower_y, int tower_w, int tower_h) {
    int num_layers = game.pit().depth();
    if (num_layers <= 0) return;

    // Header Title
    int title_sz = 16;
    const char* title = "LAYERS";
    DrawText(title, tower_x + (tower_w - MeasureText(title, title_sz)) / 2, tower_y, title_sz, theme_.hud_title);

    const char* sub = "TOP -- FLOOR";
    int sub_sz = 11;
    DrawText(sub, tower_x + (tower_w - MeasureText(sub, sub_sz)) / 2, tower_y + 20, sub_sz, Fade(theme_.hud_label, 0.7f));

    int content_top = tower_y + 36;
    int content_h = tower_h - 40;
    int slot_h = content_h / num_layers;
    if (slot_h > 36) slot_h = 36;
    if (slot_h < 18) slot_h = 18;

    int total_h = slot_h * num_layers;
    int start_y = content_top + (content_h - total_h) / 2;

    int max_cubes = game.pit().width() * game.pit().height();

    // Render each depth layer from top (z=0, opening) down to bottom (z=num_layers-1, floor)
    for (int z = 0; z < num_layers; ++z) {
        int sy = start_y + z * slot_h;
        int sx = tower_x + 10;
        int sw = tower_w - 20;

        int cube_count = game.pit().line_cube_count(z);
        bool occupied = (cube_count > 0);
        Color layer_col = get_layer_color(z, num_layers, 1.0f);

        // Layer depth label: L12 down to L01 (or 12 down to 1 from floor)
        int lvl_from_floor = (num_layers - z);
        const char* lvl_str = TextFormat("%2d", lvl_from_floor);
        int lbl_sz = (slot_h >= 24) ? 12 : 11;
        DrawText(lvl_str, sx + 2, sy + (slot_h - lbl_sz) / 2, lbl_sz, occupied ? theme_.hud_score : Fade(theme_.hud_label, 0.7f));

        // Slot bar coordinates
        int bar_x = sx + 24;
        int bar_w = sw - 28;
        int bar_h = slot_h - 3;
        int bar_y = sy + 1;

        if (occupied) {
            // Layer background tint
            DrawRectangle(bar_x, bar_y, bar_w, bar_h, Fade(layer_col, 0.22f));

            // Fill ratio bar
            float fill_ratio = static_cast<float>(cube_count) / static_cast<float>(max_cubes);
            int fill_w = static_cast<int>(bar_w * fill_ratio);
            if (fill_w < 5) fill_w = 5;
            DrawRectangle(bar_x, bar_y, fill_w, bar_h, Fade(layer_col, 0.90f));

            // Outline
            DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, layer_col);

            // Cube count label inside bar
            const char* count_str = TextFormat("%d/%d", cube_count, max_cubes);
            int count_sz = (slot_h >= 24) ? 11 : 10;
            int text_x = bar_x + (bar_w - MeasureText(count_str, count_sz)) / 2;
            int text_y = bar_y + (bar_h - count_sz) / 2;
            DrawText(count_str, text_x + 1, text_y + 1, count_sz, Fade(BLACK, 0.8f));
            DrawText(count_str, text_x, text_y, count_sz, WHITE);
        } else {
            // Empty layer: subtle outline and centered colored chip
            DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, Fade(layer_col, 0.35f));

            // Subtle colored dot in the center to highlight the layer's designated color
            int chip_w = 14;
            int chip_h = std::max(4, bar_h - 6);
            DrawRectangle(bar_x + (bar_w - chip_w) / 2, bar_y + (bar_h - chip_h) / 2, chip_w, chip_h, Fade(layer_col, 0.40f));
        }
    }
}

void Renderer::draw_left_panel(const Game& game) {
    int left_w = 180;

    // Solid background for left sidebar
    DrawRectangle(0, 0, left_w, screen_height_, theme_.hud_background);
    DrawLine(left_w, 0, left_w, screen_height_, theme_.hud_border);

    int tower_y = 16;
    if (game.preview_next_piece()) {
        int card_x = 12;
        int card_y = 14;
        int card_w = left_w - 24; // 156 px
        int card_h = 132;
        draw_next_piece_preview(game, card_x, card_y, card_w, card_h);
        tower_y = card_y + card_h + 14;
    }

    int tower_h = screen_height_ - tower_y - 12;
    draw_layer_tower(game, 0, tower_y, left_w, tower_h);
}

void Renderer::draw_hud(const Game& game, bool quit_requested, bool restart_requested) {

    int panel_x = screen_width_ - 300;
    int panel_w = 300;

    // Solid background for HUD sidebar
    DrawRectangle(panel_x, 0, panel_w, screen_height_, theme_.hud_background);
    DrawLine(panel_x, 0, panel_x, screen_height_, theme_.hud_border);

    // Title
    int title_y = (screen_height_ < 720) ? 20 : 30;
    DrawText("BLOCKOUT", panel_x + 35, title_y, 36, theme_.hud_title);
    DrawText("MODERN C++20", panel_x + 37, title_y + 40, 14, Fade(theme_.hud_label, 0.8f));
    DrawText(TextFormat("PIT: %dx%dx%d", game.pit().width(), game.pit().height(), game.pit().depth()),
             panel_x + 37, title_y + 60, 16, theme_.hud_title);

    // Score Board
    int y = (screen_height_ < 720) ? 95 : 125;
    DrawText("SCORE", panel_x + 30, y, 16, theme_.hud_label);
    DrawText(TextFormat("%08d", game.score()), panel_x + 30, y + 24, 30, theme_.hud_score);

    int step_score = (screen_height_ < 720) ? 60 : 75;
    int step_stat  = (screen_height_ < 720) ? 50 : 62;

    y += step_score;
    DrawText("HIGH SCORE", panel_x + 30, y, 16, theme_.hud_label);
    DrawText(TextFormat("%08d", game.high_score()), panel_x + 30, y + 24, 26, {200, 200, 200, 255});

    y += step_stat + 8;
    DrawText("LEVEL", panel_x + 30, y, 16, theme_.hud_label);
    DrawText(TextFormat("%d", game.level()), panel_x + 30, y + 22, 24, {100, 255, 160, 255});

    y += step_stat;
    DrawText("LAYERS CLEARED", panel_x + 30, y, 16, theme_.hud_label);
    DrawText(TextFormat("%d", game.lines_cleared()), panel_x + 30, y + 22, 24, WHITE);

    y += step_stat;
    DrawText("CUBES PLACED", panel_x + 30, y, 16, theme_.hud_label);
    DrawText(TextFormat("%d", game.cubes_placed()), panel_x + 30, y + 22, 24, WHITE);

    // Controls Legend Card
    int card_y = (screen_height_ < 720) ? y + 42 : y + 60;
    int card_h = (screen_height_ < 720) ? 228 : 246;
    DrawRectangle(panel_x + 18, card_y, panel_w - 36, card_h, Fade(theme_.hud_background, 0.7f));
    DrawRectangleLines(panel_x + 18, card_y, panel_w - 36, card_h, Fade(theme_.hud_border, 0.4f));

    int item_y = card_y + 8;
    int cstep = (screen_height_ < 720) ? 18 : 20;
    DrawText("CONTROLS", panel_x + 30, item_y, 16, theme_.hud_title); item_y += cstep + 2;
    DrawText("Move: Arrows", panel_x + 30, item_y, 14, RAYWHITE); item_y += cstep;
    DrawText("Pitch (X): Q / A", panel_x + 30, item_y, 14, RAYWHITE); item_y += cstep;
    DrawText("Yaw   (Y): W / S", panel_x + 30, item_y, 14, RAYWHITE); item_y += cstep;
    DrawText("Roll  (Z): E / D", panel_x + 30, item_y, 14, RAYWHITE); item_y += cstep;
    DrawText("Hard Drop: SPACE", panel_x + 30, item_y, 14, {255, 210, 80, 255}); item_y += cstep;
    DrawText(TextFormat("Shadow (G): %s", game.show_ghost() ? "ON" : "OFF"),
             panel_x + 30, item_y, 14, game.show_ghost() ? Color{100, 255, 160, 255} : Color{180, 180, 180, 255}); item_y += cstep;
    DrawText("Fullscreen: F11", panel_x + 30, item_y, 14, {140, 200, 255, 255}); item_y += cstep;
    DrawText("Pause: P", panel_x + 30, item_y, 14, RAYWHITE); item_y += cstep;
    DrawText("Restart: R", panel_x + 30, item_y, 14, {255, 200, 100, 255}); item_y += cstep;
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

        const char* title = "QUIT GAME?";
        DrawText(title, cx - MeasureText(title, 28) / 2, my + 22, 28, {255, 200, 60, 255});
        const char* sub = "Are you sure you want to exit?";
        DrawText(sub, cx - MeasureText(sub, 17) / 2, my + 62, 17, RAYWHITE);

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
    } else if (restart_requested) {
        int modal_w = 400;
        int modal_h = 180;
        int mx = cx - modal_w / 2;
        int my = cy - modal_h / 2;

        // Dark modal card with bright cyan border
        DrawRectangle(mx, my, modal_w, modal_h, Fade({12, 16, 26, 255}, 0.96f));
        DrawRectangleLines(mx, my, modal_w, modal_h, {0, 220, 255, 255});
        DrawRectangleLines(mx + 1, my + 1, modal_w - 2, modal_h - 2, {0, 160, 220, 180});

        const char* title = "RESTART GAME?";
        DrawText(title, cx - MeasureText(title, 28) / 2, my + 22, 28, {0, 220, 255, 255});
        const char* sub = "Are you sure you want to restart?";
        DrawText(sub, cx - MeasureText(sub, 17) / 2, my + 62, 17, RAYWHITE);

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

void Renderer::draw(const Game& game, bool quit_requested, bool restart_requested) {
    screen_width_ = GetScreenWidth();
    screen_height_ = GetScreenHeight();

    BeginDrawing();
    ClearBackground(theme_.background);

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

    // 2. Draw Left Panel (Next Piece Preview + Pit Layer Tower)
    draw_left_panel(game);

    // 3. Draw 2D HUD sidebar on the right side
    draw_hud(game, quit_requested, restart_requested);

    EndDrawing();
}

} // namespace blockout
