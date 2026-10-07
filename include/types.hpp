#pragma once

#include <cmath>
#include <cstdint>
#include <array>
#include <vector>
#include <string>

namespace blockout {

struct Vec3i {
    int x = 0;
    int y = 0;
    int z = 0;

    constexpr Vec3i() = default;
    constexpr Vec3i(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}

    constexpr bool operator==(const Vec3i& other) const = default;
    constexpr auto operator<=>(const Vec3i& other) const = default;

    constexpr Vec3i operator+(const Vec3i& o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3i operator-(const Vec3i& o) const { return {x - o.x, y - o.y, z - o.z}; }
};

// 3x3 Integer Rotation Matrix (values are -1, 0, or 1)
struct Mat3i {
    int m[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };

    static constexpr Mat3i identity() {
        return Mat3i{};
    }

    // Multiply: this * other
    constexpr Mat3i multiply(const Mat3i& o) const {
        Mat3i res{};
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                res.m[r][c] = m[r][0] * o.m[0][c] +
                              m[r][1] * o.m[1][c] +
                              m[r][2] * o.m[2][c];
            }
        }
        return res;
    }

    constexpr Vec3i transform(const Vec3i& v) const {
        return {
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
        };
    }

    // BlockOut rotation matrices (+90 and -90 deg)
    // Ox (+90 around X): (x, y, z) -> (x, -z, y)
    static constexpr Mat3i rot_x(int dir) {
        if (dir > 0) {
            return Mat3i{{{1, 0, 0}, {0, 0, -1}, {0, 1, 0}}};
        } else {
            return Mat3i{{{1, 0, 0}, {0, 0, 1}, {0, -1, 0}}};
        }
    }

    // Oy (+90 around Y): (x, y, z) -> (-z, y, x)
    static constexpr Mat3i rot_y(int dir) {
        if (dir > 0) {
            return Mat3i{{{0, 0, -1}, {0, 1, 0}, {1, 0, 0}}};
        } else {
            return Mat3i{{{0, 0, 1}, {0, 1, 0}, {-1, 0, 0}}};
        }
    }

    // Oz (+90 around Z): (x, y, z) -> (-y, x, z)
    static constexpr Mat3i rot_z(int dir) {
        if (dir > 0) {
            return Mat3i{{{0, -1, 0}, {1, 0, 0}, {0, 0, 1}}};
        } else {
            return Mat3i{{{0, 1, 0}, {-1, 0, 0}, {0, 0, 1}}};
        }
    }
};

enum class GameState {
    Playing,
    Paused,
    GameOver
};

enum class Difficulty {
    Easy = 0,
    Normal = 1,
    Hard = 2,
    Extreme = 3
};

inline constexpr int DIFFICULTY_COUNT = 4;

inline int difficulty_start_level(Difficulty d) {
    switch (d) {
        case Difficulty::Easy: return 0;
        case Difficulty::Normal: return 2;
        case Difficulty::Hard: return 4;
        case Difficulty::Extreme: return 6;
    }
    return 0;
}

inline std::string difficulty_name(Difficulty d) {
    switch (d) {
        case Difficulty::Easy: return "Easy";
        case Difficulty::Normal: return "Normal";
        case Difficulty::Hard: return "Hard";
        case Difficulty::Extreme: return "Extreme";
    }
    return "Easy";
}

inline Difficulty parse_difficulty(const std::string& str) {
    std::string s = str;
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (s == "normal" || s == "med" || s == "medium") return Difficulty::Normal;
    if (s == "hard") return Difficulty::Hard;
    if (s == "extreme" || s == "insane") return Difficulty::Extreme;
    return Difficulty::Easy;
}

// Original BlockOut score factors
inline constexpr std::array<float, 11> P_LEVEL_FACTOR = {
    0.066990f, 0.139195f, 0.219800f, 0.308444f, 0.403897f,
    0.507822f, 0.619062f, 0.738630f, 0.865802f, 1.000000f,
    1.133333f
};

inline constexpr std::array<float, 19> DEPTH_FACTOR = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.557692f, 1.367521f, 1.217949f, 1.100427f, 1.000000f,
    0.918803f, 0.852564f, 0.788996f, 0.737714f, 0.691774f,
    0.651709f, 0.614850f, 0.583868f
};

inline constexpr std::array<float, 11> L_LEVEL_FACTOR = {
    0.096478f, 0.163873f, 0.242913f, 0.328261f, 0.422329f,
    0.518394f, 0.630405f, 0.747501f, 0.867087f, 1.000000f,
    1.131653f
};

inline constexpr std::array<float, 6> L_NUMBER_FACTOR = {
    0.0f, 1.000000f, 3.703372f, 8.104827f, 14.188325f, 22.144941f
};

inline constexpr float LINE_BASE_FLAT = 762.5f;
inline constexpr float TIME_BASE = 5.51f;
inline constexpr float TIME_LEVEL_FACTOR = 0.64f;
inline constexpr float DROP_TIME = 0.16f;
inline constexpr float SPARK_TIME = 0.5f;

} // namespace blockout
