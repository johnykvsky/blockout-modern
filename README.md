# BlockOut Modern (Flat Set)

A modern, streamlined 3D Tetris puzzle game rewritten in **C++20** using **Raylib**, based on the classic **BlockOut II**.

![Screenshot](https://raw.githubusercontent.com/johnykvsky/blockout-modern/master/blockout-modern.jpg)

---

## Highlights of this Modern Rewrite

* **Lightweight & Clean**: Down from 16,000+ lines of legacy C++ / SDL 1.2 / OpenGL 1.1 to **~1,400 lines of modern C++20**.
* **Zero Obsolete Dependencies**: No SDL 1.2, no SDL_mixer 1.2, no bundled ancient libpng/giflib. Raylib is statically linked.
* **Pure Flat Set**: Features the authentic 8 planar polyomino pieces (Monomino, Domino, Trominoes, Tetrominoes) rotating freely in 3D.
* **Seamless Wireframe Rendering**: Active pieces and ghost shadows render as clean, unified 3D wireframe polycubes showing **only exterior outline borders** — internal seams and dividing lines between touching unit cubes are automatically eliminated.
* **Dynamic & Resizable Window**:
  * Freely resizable window with minimum size enforcement (800x600).
  * Responsive 3D camera that dynamically frames and centers the pit in the play area across any resolution or aspect ratio.
  * Toggle borderless Fullscreen anytime with <kbd>F11</kbd> or <kbd>Alt</kbd> + <kbd>Enter</kbd>.
  * Window size is automatically remembered and saved to `config.json` when resized.
* **Authentic Physics & Scoring**:
  * 3D Pit grid (default 7x7x12, fully customizable).
  * Pitch (X), Yaw (Y), and Roll (Z) 3D rotations with wall-kick compensation.
  * Layer collapse and authentic BlockOut multi-line scoring formulas.
  * Empty Pit ("Flush") clearing bonuses.
  * Original retro sound effects (`blub.wav`, `wozz.wav`, `tchh.wav`, `line.wav`, `empty.wav`, `hit.wav`, `level.wav`).
  * Toggleable Ghost piece shadow on the pit floor (<kbd>G</kbd>).
  * Local persistent high score (`highscore.txt`).
  * Full JSON configuration with comprehensive validation (`config.json`).
* **Confirmation Dialogs**: Built-in modal confirmation dialogs for both **Quitting** (<kbd>ESC</kbd> / Window Close) and **Restarting** (<kbd>R</kbd> / <kbd>F2</kbd>) prevent accidental game interruption, with interactive mouse buttons and keyboard (<kbd>Y</kbd>/<kbd>Enter</kbd>/<kbd>N</kbd>/<kbd>ESC</kbd>) support.
* **Optional Next Block Preview**: Sleek upcoming piece preview card on the left side of the pit, configurable via `config.json` (disabled by default).

---

## Configuration (`config.json`)

You can customize the pit dimensions, window resolution, and next block preview by editing `config.json` in the application directory:

```json
{
  "width": 7,
  "length": 7,
  "depth": 12,
  "window_width": 1024,
  "window_height": 768,
  "preview_next_piece": false
}
```

### Pit Settings:
* **`width`**: Pit width (horizontal columns), range **[3, 7]** (default: `7`).
* **`length`**: Pit length/height (vertical rows), range **[3, 7]** (default: `7`).
* **`depth`**: Pit depth (layers into the screen), range **[6, 18]** (default: `12`).

### Window Settings:
* **`window_width`**: Initial window width in pixels, range **[800, 7680]** (default: `1024`).
* **`window_height`**: Initial window height in pixels, range **[600, 4320]** (default: `768`).

### Preview Settings:
* **`preview_next_piece`**: Toggle preview of the incoming block on the left side of the pit (`true` / `false`, default: `false`).

### Validation & Safety:
* **Strict Typing**: Dimensions and resolutions must be integer numbers; `preview_next_piece` must be a boolean (`true`/`false`). Unrecognized types (strings, floats, null) are rejected with console warnings, keeping safe defaults.
* **Range Checks**: Out-of-bounds dimensions (e.g. `width: 12` or `window_width: 500`) are rejected with console warnings, keeping safe defaults.
* **Proportions & Aspect Ratio**: Validates that `aspect = window_width / window_height` is within the sensible range **[0.75, 3.6]**. Extreme slit resolutions (e.g. `4000x600` or `12x500`) are rejected and reset to `1024x768`.
* **Auto-Creation**: If `config.json` is missing, the game creates it automatically with defaults.
* **Auto-Persistence**: Resizing the window with your mouse updates `config.json` automatically, preserving your layout for subsequent sessions.

---

## Controls

| Key | Action |
| :--- | :--- |
| **Arrow Keys** (or Numpad 4/8/6/2) | Move Left / Up / Right / Down |
| **Q / A** (or Numpad 7/1) | Pitch rotation (±90° around X-axis) |
| **W / S** (or Numpad 9/3) | Yaw rotation (±90° around Y-axis) |
| **E / D** (or Numpad / and *) | Roll rotation (±90° around Z-axis) |
| **Space** (or Numpad 0) | Hard Drop (instantly drops and locks piece) |
| **G** | Toggle Shadow / Ghost Piece (ON / OFF) |
| **F11** or **Alt + Enter** | Toggle Fullscreen |
| **P** | Pause / Resume |
| **R** or **F2** | Restart Game (opens confirmation dialog during gameplay) |
| **Escape** or **Window Close (X)** | Quit Game (opens confirmation dialog) |
| **Y** or **Enter** / Click **YES** | Confirm Restart / Exit |
| **N** or **Escape** / Click **NO** | Cancel Dialog & Resume Game |
| **R** or **Enter** or **Space** | Instant Restart (when Game Over) |

---

## Building and Running

### Prerequisites (Linux)
* GCC with C++20 support (`g++ >= 11`)
* Make & Git (for fetching dependencies)
* X11 / OpenGL development libraries (`libx11-dev`, `libgl1-mesa-dev` on Debian/Ubuntu)
* **Raylib**: Required graphics/audio dependency. The empty directory structure `third_party/raylib/` is tracked in the repository, while library files are downloaded locally via Make.

### Dependencies
Fetch and build Raylib locally into `third_party/raylib`:
```bash
make deps
```

### Build
Compile the game (automatically runs `make deps` if Raylib has not been fetched yet):
```bash
make -j$(nproc)
```

### Run
```bash
make run
# or
./bin/blockout
```

### Clean
Removes game build artifacts (preserves Raylib and `.gitkeep` files):
```bash
make clean
```

---

## Configuration & Custom Themes

Game settings can be customized in [`config.json`](config.json):

```json
{
  "width": 7,
  "length": 7,
  "depth": 12,
  "window_width": 1024,
  "window_height": 768,
  "preview_next_piece": false,
  "color_by_layer": true,
  "theme": "blockout2",
  "themes": {
    "blockout2": { ... },
    "contrast": { ... },
    "dark": { ... },
    "default": { ... }
  }
}
```

* **Pit Dimensions**: `width` (3–9), `length` (3–9), `depth` (6–18). Defaults to classic Flat Fun `7x7x12`.
* **Window Size**: `window_width` and `window_height`. The game automatically adapts its 3D viewport and aspect ratio.
* **Layer-Based Coloring**:
  * `"color_by_layer": true`: When enabled (default), all blocks placed in the pit share the same color per depth layer ($z$). For a pit of depth 12, 12 distinct colors are used from floor to opening.
  * `"color_by_layer": false`: Placed blocks retain their individual polyomino piece colors.
* **Left Sidebar Panel (Preview & Pit Layer Tower)**:
  * **Next Piece Preview**: Set `"preview_next_piece": true` to display the upcoming piece in the top card of the left panel (cleanly separated from the pit).
  * **Pit Layer Tower**: Displays a vertical ladder representing every depth layer with its designated color, layer number, and live occupancy status (cube count and fill bar). Allows quick identification of block depths at a glance.
  * **Centered 3D Pit**: The 3D tunnel is automatically centered in the play area between the left panel (180px) and the right HUD (300px), eliminating visual overlap.
* **Color Themes**:
  * Set `"theme"` to `"blockout2"` (original BlockOut II authentic palette), `"contrast"`, `"dark"`, `"default"`, or define your own custom theme name inside `"themes"`.
  * If the specified theme or any property is omitted or malformed, the game safely falls back to hardcoded defaults.
  * Customizable theme properties:
    * `background`: Window background color (`#RRGGBB` or `#RRGGBBAA`).
    * `cube_wireframe`: Outlines of placed cubes at the bottom (e.g. `#000000FF` for classic black contours or `#808890FF` for sleek gray borders).
    * `active_wireframe`: Wireframe outline of the falling piece going down (e.g. `#FFFFFFFF` for pure white borders on all piece types; leave omitted or transparent to use per-piece colors).
    * `pit_corner`, `pit_grid`, `pit_opening`, `pit_depth_rings`, `pit_floor_perimeter`, `pit_floor_grid`: Tunnel wireframe styling.
    * `ghost_wireframe`, `ghost_alpha`: Landing silhouette outlines and opacity.
    * `piece_colors`: Array of 8 hex colors for the polyomino pieces.
    * `layer_colors`: Array of 12 hex colors for pit depth layers (ordered from floor up to opening).
    * `hud_background`, `hud_border`, `hud_title`, `hud_score`, `hud_label`: UI panel colors.

---

## Architecture

```
blockout-modern/
├── assets/
│   └── sounds/       # Authentic WAV sound effects
├── include/
│   ├── types.hpp     # Vec3i, Mat3i, GameState, scoring constants & factors
│   ├── config.hpp    # Config struct definition
│   ├── pieces.hpp    # 8 Flat pieces definitions & bag randomizer
│   ├── pit.hpp       # 3D pit grid, layer clearing & collision detection
│   ├── game.hpp      # Game loop state machine, scoring & gravity timer
│   ├── renderer.hpp  # Dynamic 3D camera, wireframe tunnel, outline extraction, HUD
│   └── audio.hpp     # Raylib sound effects manager
├── src/
│   ├── config.cpp    # JSON parser with strict typing & range validation
│   ├── pieces.cpp
│   ├── pit.cpp
│   ├── game.cpp
│   ├── renderer.cpp  # Responsive camera framing & unified outline geometry
│   ├── audio.cpp
│   └── main.cpp      # Window setup, resizability, DAS input handling
├── third_party/
│   └── raylib/       # Raylib dependency (empty in repo, fetched via 'make deps')
├── Makefile
├── config.json       # User settings (pit size & window resolution)
├── highscore.txt     # Local persistent high score
├── .gitignore        # Git ignore rules for build artifacts
└── LICENSE.md        # GNU General Public License v2.0 or later
```

---

## License & Attributions

This project is licensed under the **GNU General Public License v2.0 or later (GPL-2.0-or-later)**, fully aligned with the licensing terms of the original **BlockOut II**. See the complete license text and notices in [LICENSE.md](LICENSE.md).

### Credits & Trademarks
* **Original Game**: *BlockOut* DOS game created by California Dreams in 1989.
* **BlockOut II**: Created by Jean-Luc Pons (2007–2014) and released under the GNU General Public License v2 or later.
  * Official Website: [http://www.blockout.net/blockout2](http://www.blockout.net/blockout2)
  * SourceForge: [http://sourceforge.net/projects/blockout](http://sourceforge.net/projects/blockout)
* **Trademark Notice**: *Blockout®* is a registered trademark of Kadon Enterprises, Inc., used by permission. Kadon Enterprises produces physical sets of polycubes since 1980 ([www.gamepuzzles.com](http://www.gamepuzzles.com)).
* **Third-Party Libraries**: [Raylib](https://www.raylib.com) is licensed under the permissive zlib/libpng license (Copyright © 2013–2025 Ramon Santamaria).

