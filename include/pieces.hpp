#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <random>

namespace blockout {

struct Piece {
    int id;
    std::string name;
    std::vector<Vec3i> cubes;
    Vec3i center;
    int width = 0;
    int height = 0;
    int depth = 0;
    int low_score = 0;
    int high_score = 0;
    uint32_t color_hex = 0xFFFFFFFF; // RGBA hex

    std::vector<Vec3i> get_transformed(const Mat3i& rot, const Vec3i& offset) const;
};

class PieceManager {
public:
    PieceManager();

    const Piece& get_piece(int id) const;
    size_t count() const { return pieces_.size(); }

    // Bag randomizer: draws each piece once per cycle
    const Piece& draw_next();
    void reset_bag();

private:
    std::vector<Piece> pieces_;
    std::vector<int> bag_;
    size_t bag_index_ = 0;
    std::mt19937 rng_;
};

} // namespace blockout
