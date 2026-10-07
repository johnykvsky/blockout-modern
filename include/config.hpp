#pragma once

#include <string>

namespace blockout {

struct Config {
    int width = 7;   // Range: [3, 7]
    int length = 7;  // Range: [3, 7] (pit height)
    int depth = 12;  // Range: [6, 18]
    int window_width = 1024;   // Range: [800, 7680]
    int window_height = 768;   // Range: [600, 4320], aspect ratio [0.75, 3.6]

    static Config load(const std::string& filename = "config.json");
    void save(const std::string& filename = "config.json") const;
};

} // namespace blockout
