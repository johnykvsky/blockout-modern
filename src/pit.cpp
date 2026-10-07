#include "pit.hpp"
#include <cmath>
#include <algorithm>

namespace blockout {

Pit::Pit(int w, int h, int d) {
    resize(w, h, d);
}

void Pit::resize(int w, int h, int d) {
    width_ = w;
    height_ = h;
    depth_ = d;
    cells_.assign(width_ * height_ * depth_, 0);
}

void Pit::clear() {
    std::fill(cells_.begin(), cells_.end(), 0);
}

bool Pit::in_bounds(int x, int y, int z) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_ && z >= 0 && z < depth_;
}

int Pit::get(int x, int y, int z) const {
    if (!in_bounds(x, y, z)) {
        return 1; // Out of bounds is considered solid
    }
    return cells_[index(x, y, z)];
}

void Pit::set(int x, int y, int z, int val) {
    if (in_bounds(x, y, z)) {
        cells_[index(x, y, z)] = val;
    }
}

bool Pit::is_line_full(int z) const {
    if (z < 0 || z >= depth_) return false;
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (cells_[index(x, y, z)] == 0) return false;
        }
    }
    return true;
}

bool Pit::is_line_empty(int z) const {
    if (z < 0 || z >= depth_) return true;
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (cells_[index(x, y, z)] != 0) return false;
        }
    }
    return true;
}

bool Pit::is_empty() const {
    for (int z = 0; z < depth_; ++z) {
        if (!is_line_empty(z)) return false;
    }
    return true;
}

int Pit::remove_full_lines() {
    int removed = 0;
    int k = depth_ - 1;

    while (k >= 0) {
        if (is_line_full(k)) {
            // Drop everything above layer k down by 1
            for (int z = k; z > 0; --z) {
                for (int y = 0; y < height_; ++y) {
                    for (int x = 0; x < width_; ++x) {
                        cells_[index(x, y, z)] = cells_[index(x, y, z - 1)];
                    }
                }
            }
            // Clear top layer (z = 0)
            for (int y = 0; y < height_; ++y) {
                for (int x = 0; x < width_; ++x) {
                    cells_[index(x, y, 0)] = 0;
                }
            }
            removed++;
        } else {
            k--;
        }
    }

    return removed;
}

void Pit::get_out_of_bounds(int x, int y, int z, int& ox, int& oy, int& oz) const {
    ox = 0;
    oy = 0;
    oz = 0;

    if (x < 0) ox = -x;
    if (x >= width_) ox = width_ - x - 1;

    if (y < 0) oy = -y;
    if (y >= height_) oy = height_ - y - 1;

    if (z < 0) oz = -z;
    if (z >= depth_) oz = depth_ - z - 1;
}

bool Pit::is_overlap(const std::vector<Vec3i>& cubes,
                     Vec3i* out_first_overlap,
                     Vec3i* out_max_nudge) const {
    bool overlap = false;
    int mox = 0, moy = 0, moz = 0;

    for (const auto& c : cubes) {
        if (get(c.x, c.y, c.z) != 0) {
            if (!overlap && out_first_overlap) {
                *out_first_overlap = c;
            }
            overlap = true;
        }

        int lox = 0, loy = 0, loz = 0;
        get_out_of_bounds(c.x, c.y, c.z, lox, loy, loz);
        if (std::abs(lox) > std::abs(mox)) mox = lox;
        if (std::abs(loy) > std::abs(moy)) moy = loy;
        if (std::abs(loz) > std::abs(moz)) moz = loz;
    }

    if (out_max_nudge) {
        *out_max_nudge = {mox, moy, moz};
    }

    return overlap;
}

void Pit::add_cubes(const std::vector<Vec3i>& cubes, int val) {
    for (const auto& c : cubes) {
        set(c.x, c.y, c.z, val);
    }
}

} // namespace blockout
