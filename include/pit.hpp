#pragma once

#include "types.hpp"
#include <vector>

namespace blockout {

class Pit {
public:
    Pit(int w = 7, int h = 7, int d = 12);

    int width() const { return width_; }
    int height() const { return height_; }
    int depth() const { return depth_; }

    void resize(int w, int h, int d);
    void clear();

    bool in_bounds(int x, int y, int z) const;
    int get(int x, int y, int z) const;
    void set(int x, int y, int z, int val);

    bool is_line_full(int z) const;
    bool is_line_empty(int z) const;
    bool is_empty() const;

    int remove_full_lines();

    void get_out_of_bounds(int x, int y, int z, int& ox, int& oy, int& oz) const;

    bool is_overlap(const std::vector<Vec3i>& cubes,
                    Vec3i* out_first_overlap = nullptr,
                    Vec3i* out_max_nudge = nullptr) const;

    void add_cubes(const std::vector<Vec3i>& cubes, int val);

private:
    int width_;
    int height_;
    int depth_;
    std::vector<int> cells_;

    int index(int x, int y, int z) const {
        return x + y * width_ + z * (width_ * height_);
    }
};

} // namespace blockout
