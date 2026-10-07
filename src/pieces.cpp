#include "pieces.hpp"
#include <algorithm>
#include <chrono>

namespace blockout {

std::vector<Vec3i> Piece::get_transformed(const Mat3i& rot, const Vec3i& offset) const {
    std::vector<Vec3i> res;
    res.reserve(cubes.size());
    for (const auto& c : cubes) {
        float rx = rot.m[0][0] * (c.x - center.x + 0.5f) +
                   rot.m[0][1] * (c.y - center.y + 0.5f) +
                   rot.m[0][2] * (c.z - center.z + 0.5f);
        float ry = rot.m[1][0] * (c.x - center.x + 0.5f) +
                   rot.m[1][1] * (c.y - center.y + 0.5f) +
                   rot.m[1][2] * (c.z - center.z + 0.5f);
        float rz = rot.m[2][0] * (c.x - center.x + 0.5f) +
                   rot.m[2][1] * (c.y - center.y + 0.5f) +
                   rot.m[2][2] * (c.z - center.z + 0.5f);

        res.push_back({
            static_cast<int>(std::round(rx - 0.5f)) + center.x + offset.x,
            static_cast<int>(std::round(ry - 0.5f)) + center.y + offset.y,
            static_cast<int>(std::round(rz - 0.5f)) + center.z + offset.z
        });
    }
    return res;
}

PieceManager::PieceManager()
    : rng_(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count())) {

    auto init_piece = [this](int id, const std::string& name,
                             std::vector<Vec3i> cubes,
                             int low_score, int high_score, uint32_t color) {
        Piece p;
        p.id = id;
        p.name = name;
        p.cubes = std::move(cubes);
        p.low_score = low_score;
        p.high_score = high_score;
        p.color_hex = color;

        int max_w = 0, max_h = 0, max_d = 0;
        for (const auto& c : p.cubes) {
            if (c.x > max_w) max_w = c.x;
            if (c.y > max_h) max_h = c.y;
            if (c.z > max_d) max_d = c.z;
        }
        p.width = max_w + 1;
        p.height = max_h + 1;
        p.depth = max_d + 1;

        // BlockOut original rotation center
        p.center = {p.width - 1, 1, p.depth - 1};

        pieces_.push_back(std::move(p));
    };

    // 8 Original Flat pieces
    // Piece 0: Monomino (1 cube)
    init_piece(0, "Monomino",
               {{0, 0, 0}},
               14, 156, 0xE5C07BFF); // Gold / Warm Yellow

    // Piece 1: Domino (2 cubes)
    init_piece(1, "Domino",
               {{0, 0, 0}, {0, 1, 0}},
               14, 156, 0x56B6C2FF); // Cyan

    // Piece 2: I-Tromino (3 cubes)
    init_piece(2, "I-Tromino",
               {{0, 0, 0}, {0, 1, 0}, {0, 2, 0}},
               27, 307, 0x98C379FF); // Lime Green

    // Piece 3: L-Tromino / Corner (3 cubes)
    init_piece(3, "L-Tromino",
               {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}},
               27, 307, 0xD19A66FF); // Orange

    // Piece 4: Square (4 cubes)
    init_piece(4, "Square",
               {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}},
               14, 156, 0x61AFEFFF); // Sky Blue

    // Piece 5: T-Tetromino (4 cubes)
    init_piece(5, "T-Tetromino",
               {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {1, 1, 0}},
               40, 461, 0xC678DDFF); // Purple / Magenta

    // Piece 6: Z-Tetromino (4 cubes)
    init_piece(6, "Z-Tetromino",
               {{1, 0, 0}, {2, 0, 0}, {0, 1, 0}, {1, 1, 0}},
               40, 461, 0xE06C75FF); // Coral Red

    // Piece 7: L-Tetromino (4 cubes)
    init_piece(7, "L-Tetromino",
               {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {2, 1, 0}},
               27, 307, 0x4EC9B0FF); // Teal / Turquoise

    reset_bag();
}

const Piece& PieceManager::get_piece(int id) const {
    return pieces_.at(static_cast<size_t>(id));
}

void PieceManager::reset_bag() {
    bag_.resize(pieces_.size());
    for (size_t i = 0; i < pieces_.size(); ++i) {
        bag_[i] = static_cast<int>(i);
    }
    std::shuffle(bag_.begin(), bag_.end(), rng_);
    bag_index_ = 0;
}

const Piece& PieceManager::draw_next() {
    if (bag_index_ >= bag_.size()) {
        reset_bag();
    }
    int id = bag_[bag_index_++];
    return get_piece(id);
}

} // namespace blockout
