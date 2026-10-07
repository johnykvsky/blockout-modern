#include "raylib.h"
#include "audio.hpp"
#include "game.hpp"
#include "renderer.hpp"
#include "config.hpp"
#include "icon_data.hpp"
#include <vector>

using namespace blockout;

int main() {
    Config config = Config::load("config.json");

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(config.window_width, config.window_height, "BlockOut 3D (Flat)");
    SetWindowMinSize(800, 600);
    SetExitKey(KEY_NULL); // Disable automatic ESC exit to allow confirmation dialog
    SetTargetFPS(60);

    // Set application window icons (multi-resolution for crisp titlebar, dock & taskbar display)
    std::vector<Image> icons;
    const char* icon_paths[] = {
        "assets/icon.png",
        "assets/icon_128.png",
        "assets/icon_64.png",
        "assets/icon_48.png",
        "assets/icon_32.png",
        "assets/icon_16.png"
    };
    for (const char* path : icon_paths) {
        if (FileExists(path)) {
            Image img = LoadImage(path);
            if (img.data != nullptr) {
                icons.push_back(img);
            }
        }
    }
    if (!icons.empty()) {
        SetWindowIcons(icons.data(), static_cast<int>(icons.size()));
        for (auto& img : icons) {
            UnloadImage(img);
        }
    } else {
        // Fallback: embedded 32x32 RGBA icon ensures an authentic icon even without assets folder
        Image fallback_img = LoadImageFromMemory(".png", EMBEDDED_ICON_PNG, static_cast<int>(EMBEDDED_ICON_PNG_LEN));
        if (fallback_img.data != nullptr) {
            SetWindowIcon(fallback_img);
            UnloadImage(fallback_img);
        }
    }

    AudioManager audio;
    Game game(audio, config);
    Renderer renderer(config.window_width, config.window_height, config.get_theme(), config.color_by_layer);

    // DAS (Delayed Auto Shift) timers for smooth movement
    struct DasState {
        float timer = 0.0f;
        float repeat_timer = 0.0f;
    };
    DasState das_left, das_right, das_up, das_down;

    auto update_das = [](bool is_down, bool is_pressed, DasState& das, auto action) {
        if (is_pressed) {
            action();
            das.timer = 0.0f;
            das.repeat_timer = 0.0f;
        } else if (is_down) {
            das.timer += GetFrameTime();
            if (das.timer >= 0.18f) { // DAS delay
                das.repeat_timer += GetFrameTime();
                if (das.repeat_timer >= 0.06f) { // DAS repeat rate
                    das.repeat_timer = 0.0f;
                    action();
                }
            }
        } else {
            das.timer = 0.0f;
            das.repeat_timer = 0.0f;
        }
    };

    bool quit_requested = false;
    bool restart_requested = false;
    bool should_exit = false;

    while (!should_exit) {
        float dt = GetFrameTime();

        // Detect window close request (clicking X on window title bar)
        if (WindowShouldClose()) {
            if (restart_requested) restart_requested = false;
            quit_requested = true;
        }

        // Fullscreen Toggle (F11 or Alt+Enter)
        if (IsKeyPressed(KEY_F11) || (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) {
            ToggleFullscreen();
        }

        // Remember resized window dimensions in config
        if (IsWindowResized() && !IsWindowFullscreen()) {
            config.window_width = GetScreenWidth();
            config.window_height = GetScreenHeight();
            config.save("config.json");
        }

        // ESC Key: Cancel restart dialog if open, else toggle quit confirmation
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (restart_requested) {
                restart_requested = false;
            } else {
                quit_requested = !quit_requested;
            }
        }

        // Restart Key (R or F2): If playing/paused and quit dialog is not active, toggle restart confirmation
        if (!quit_requested && game.state() != GameState::GameOver && (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_F2))) {
            restart_requested = !restart_requested;
        }

        if (quit_requested) {
            // Confirm Quit: Y or Enter
            if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_ENTER)) {
                should_exit = true;
            }
            // Cancel Quit: N
            if (IsKeyPressed(KEY_N)) {
                quit_requested = false;
            }

            // Mouse button interaction for [YES] / [NO] buttons
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                int play_area_w = GetScreenWidth() - 300;
                int cx = play_area_w / 2;
                int cy = GetScreenHeight() / 2;
                int my = cy - 90;

                Rectangle btn_yes = {static_cast<float>(cx - 135), static_cast<float>(my + 108), 120.0f, 44.0f};
                Rectangle btn_no  = {static_cast<float>(cx + 15),  static_cast<float>(my + 108), 120.0f, 44.0f};

                Vector2 mouse = GetMousePosition();
                if (CheckCollisionPointRec(mouse, btn_yes)) {
                    should_exit = true;
                } else if (CheckCollisionPointRec(mouse, btn_no)) {
                    quit_requested = false;
                }
            }
        } else if (restart_requested) {
            // Confirm Restart: Y or Enter
            if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_ENTER)) {
                game.reset();
                restart_requested = false;
            }
            // Cancel Restart: N
            if (IsKeyPressed(KEY_N)) {
                restart_requested = false;
            }

            // Mouse button interaction for [YES] / [NO] buttons
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                int play_area_w = GetScreenWidth() - 300;
                int cx = play_area_w / 2;
                int cy = GetScreenHeight() / 2;
                int my = cy - 90;

                Rectangle btn_yes = {static_cast<float>(cx - 135), static_cast<float>(my + 108), 120.0f, 44.0f};
                Rectangle btn_no  = {static_cast<float>(cx + 15),  static_cast<float>(my + 108), 120.0f, 44.0f};

                Vector2 mouse = GetMousePosition();
                if (CheckCollisionPointRec(mouse, btn_yes)) {
                    game.reset();
                    restart_requested = false;
                } else if (CheckCollisionPointRec(mouse, btn_no)) {
                    restart_requested = false;
                }
            }
        } else {
            // Pause Toggle (P key)
            if (IsKeyPressed(KEY_P)) {
                game.toggle_pause();
            }

            // Restart on Game Over
            if (game.state() == GameState::GameOver) {
                if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_F2)) {
                    game.reset();
                }
            }

            if (game.state() == GameState::Playing) {
                // Translations (Arrows / Numpad)
                update_das(IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_KP_4),
                           IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_KP_4),
                           das_left, [&]() { game.move(-1, 0); });

                update_das(IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_KP_6),
                           IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_KP_6),
                           das_right, [&]() { game.move(1, 0); });

                update_das(IsKeyDown(KEY_UP) || IsKeyDown(KEY_KP_8),
                           IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_KP_8),
                           das_up, [&]() { game.move(0, -1); });

                update_das(IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_KP_2),
                           IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_KP_2),
                           das_down, [&]() { game.move(0, 1); });

                // 3D Rotations (Q/A = Pitch, W/S = Yaw, E/D = Roll)
                // Pitch (X axis)
                if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_KP_7)) game.rotate_x(1);
                if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_KP_1)) game.rotate_x(-1);

                // Yaw (Y axis)
                if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_KP_9)) game.rotate_y(1);
                if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_KP_3)) game.rotate_y(-1);

                // Roll (Z axis)
                if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_KP_DIVIDE)) game.rotate_z(1);
                if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_KP_MULTIPLY)) game.rotate_z(-1);

                // Hard Drop
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_KP_0)) {
                    game.hard_drop();
                }

                // Toggle Shadow / Ghost Piece
                if (IsKeyPressed(KEY_G)) {
                    game.toggle_ghost();
                }
            }

            // Update physics & gravity (only when not in a modal dialog)
            game.update(dt);
        }

        // Render frame
        renderer.draw(game, quit_requested, restart_requested);
    }

    CloseWindow();
    return 0;
}
