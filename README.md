# BlockOut Modern (Flat Set)

A modern, streamlined 3D Tetris puzzle game rewritten in **C++20** using **Raylib**, based on the classic **BlockOut II**.

![Screenshot](https://raw.githubusercontent.com/johnykvsky/blockout-modern/master/blockout_modern.jpg)

---

## Highlights of this Modern Rewrite

* **Lightweight & Clean**: Down from 16,000+ lines of legacy C++ / SDL 1.2 / OpenGL 1.1 to **~1,400 lines of modern C++20**.
* **Zero Obsolete Dependencies**: No SDL 1.2, no SDL_mixer 1.2, no bundled ancient libpng/giflib. Raylib is statically linked.
* **Pure Flat Set**: Features the authentic 8 planar polyomino pieces (Monomino, Domino, Trominoes, Tetrominoes) rotating freely in 3D.
* **Seamless Wireframe Rendering**: Active pieces and ghost shadows render as clean, unified 3D wireframe polycubes showing **only exterior outline borders** — internal seams and dividing lines between touching unit cubes are automatically eliminated.
* **Dynamic & Resizable Window**:
  * Freely resizable window with minimum size enforcement ($800 \times 600$).
  * Responsive 3D camera that dynamically frames and centers the pit in the play area across any resolution or aspect ratio.
  * Toggle borderless Fullscreen anytime with <kbd>F11</kbd> or <kbd>Alt</kbd> + <kbd>Enter</kbd>.
  * Window size is automatically remembered and saved to `config.json` when resized.
* **Authentic Physics & Scoring**:
  * 3D Pit grid (default $7 \times 7 \times 12$, fully customizable).
  * Pitch ($X$), Yaw ($Y$), and Roll ($Z$) 3D rotations with wall-kick compensation.
  * Layer collapse and authentic BlockOut multi-line scoring formulas.
  * Empty Pit ("Flush") clearing bonuses.
  * Original retro sound effects (`blub.wav`, `wozz.wav`, `tchh.wav`, `line.wav`, `empty.wav`, `hit.wav`, `level.wav`).
  * Toggleable Ghost piece shadow on the pit floor (<kbd>G</kbd>).
  * Local persistent high score (`highscore.txt`).
  * Full JSON configuration with comprehensive validation (`config.json`).

---

## Configuration (`config.json`)

You can customize the pit dimensions and window resolution by editing `config.json` in the application directory:

```json
{
  "width": 7,
  "length": 7,
  "depth": 12,
  "window_width": 1024,
  "window_height": 768
}
```

### Pit Settings:
* **`width`**: Pit width (horizontal columns), range **[3, 7]** (default: `7`).
* **`length`**: Pit length/height (vertical rows), range **[3, 7]** (default: `7`).
* **`depth`**: Pit depth (layers into the screen), range **[6, 18]** (default: `12`).

### Window Settings:
* **`window_width`**: Initial window width in pixels, range **[800, 7680]** (default: `1024`).
* **`window_height`**: Initial window height in pixels, range **[600, 4320]** (default: `768`).

### Validation & Safety:
* **Strict Typing**: All values must be integer numbers. Strings, floats, booleans, and null are rejected.
* **Range Checks**: Out-of-bounds dimensions (e.g. `width: 12` or `window_width: 500`) are rejected with console warnings, keeping safe defaults.
* **Proportions & Aspect Ratio**: Validates that $\text{aspect} = \text{window\_width} / \text{window\_height}$ is within the sensible range **[0.75, 3.6]**. Extreme slit resolutions (e.g. `4000x600` or `12x500`) are rejected and reset to `1024x768`.
* **Auto-Creation**: If `config.json` is missing, the game creates it automatically with defaults.
* **Auto-Persistence**: Resizing the window with your mouse updates `config.json` automatically, preserving your layout for subsequent sessions.

---

## Controls

| Key | Action |
| :--- | :--- |
| **Arrow Keys** (or Numpad 4/8/6/2) | Move Left / Up / Right / Down |
| **Q / A** (or Numpad 7/1) | Pitch rotation ($\pm 90^\circ$ around $X$-axis) |
| **W / S** (or Numpad 9/3) | Yaw rotation ($\pm 90^\circ$ around $Y$-axis) |
| **E / D** (or Numpad / and *) | Roll rotation ($\pm 90^\circ$ around $Z$-axis) |
| **Space** (or Numpad 0) | Hard Drop (instantly drops and locks piece) |
| **G** | Toggle Shadow / Ghost Piece (ON / OFF) |
| **F11** or **Alt + Enter** | Toggle Fullscreen |
| **P** | Pause / Resume |
| **Escape** or **Window Close (X)** | Quit Game (opens confirmation dialog) |
| **Y** or **Enter** / Click **YES** | Confirm Exit |
| **N** or **Escape** / Click **NO** | Cancel Exit & Resume Game |
| **R** or **Enter** | Restart (when Game Over) |

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

