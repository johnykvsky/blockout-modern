#pragma once

#include "raylib.h"
#include "types.hpp"
#include <string>
#include <vector>
#include <map>

namespace blockout {

struct StepTimes {
    float easy = 5.51f;    // Range: [0.1, 20.0]
    float normal = 2.26f;  // Range: [0.1, 20.0]
    float hard = 0.92f;    // Range: [0.1, 20.0]
    float extreme = 0.38f; // Range: [0.1, 20.0]

    float get(Difficulty d) const {
        switch (d) {
            case Difficulty::Easy: return easy;
            case Difficulty::Normal: return normal;
            case Difficulty::Hard: return hard;
            case Difficulty::Extreme: return extreme;
        }
        return easy;
    }
};

struct ColorTheme {
    std::string name = "default";

    // 3D Scene & Pit Wireframe
    Color background           = {10, 12, 18, 255};
    Color pit_corner           = {50, 180, 255, 255};
    Color pit_grid             = {40, 130, 200, 140};
    Color pit_opening          = {0, 255, 200, 255};
    Color pit_depth_rings      = {30, 95, 150, 85};
    Color pit_floor_perimeter  = {0, 220, 255, 240};
    Color pit_floor_grid       = {40, 140, 210, 160};

    // Placed & Active Cubes / Pieces
    Color cube_wireframe       = {255, 255, 255, 200}; // Outline of cubes placed in pit
    Color active_wireframe     = {0, 0, 0, 0};         // Outline of active falling piece (alpha 0 = use piece color)
    Color ghost_wireframe      = {255, 255, 255, 102}; // Ghost piece outline
    float ghost_alpha          = 0.22f;                // Ghost piece fill opacity

    // 8 Piece Colors (Flat Set)
    std::vector<Color> piece_colors;

    // Layer Colors (one per pit depth layer, e.g. 12 levels)
    std::vector<Color> layer_colors;

    // HUD & Sidebar
    Color hud_background       = {15, 20, 30, 255};
    Color hud_border           = {40, 100, 160, 255};
    Color hud_title            = {0, 220, 255, 255};
    Color hud_score            = {255, 235, 120, 255};
    Color hud_label            = {140, 170, 200, 255};

    static ColorTheme default_theme();
    static ColorTheme dark_theme();
    static ColorTheme blockout2_theme();
};

struct Config {
    int width = 7;   // Range: [3, 7]
    int length = 7;  // Range: [3, 7] (pit height)
    int depth = 12;  // Range: [6, 18]
    int window_width = 1024;   // Range: [800, 7680]
    int window_height = 768;   // Range: [600, 4320], aspect ratio [0.75, 3.6]
    bool preview_next_piece = false; // Next block preview on the left side (default: false)
    bool color_by_layer = true;      // Color placed cubes by layer depth (default: true)
    std::string difficulty = "easy"; // Difficulty: easy, normal, hard, extreme (default: easy)
    StepTimes step_times;            // Initial fall step times in seconds per tier [0.1, 20.0]

    std::string theme = "default";
    std::map<std::string, ColorTheme> themes;

    // Resolves active theme (falls back to default_theme if theme name not found)
    const ColorTheme& get_theme() const;

    static Config load(const std::string& filename = "config.json");
    void save(const std::string& filename = "config.json") const;
};

// Validation & Color helpers
bool is_valid_hex_color(const std::string& str);
Color parse_hex_color(const std::string& str, Color fallback = WHITE);
std::string color_to_hex(Color c);

} // namespace blockout
