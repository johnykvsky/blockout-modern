#include "config.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cctype>

namespace blockout {

namespace {

enum class TokenType {
    String,
    IntNumber,
    FloatNumber,
    Boolean,
    Null,
    Colon,
    Comma,
    BraceOpen,
    BraceClose,
    BracketOpen,
    BracketClose,
    Other,
    EndOfFile
};

struct Token {
    TokenType type = TokenType::EndOfFile;
    std::string text;
    long long int_value = 0;
};

class JsonLexer {
public:
    explicit JsonLexer(const std::string& src) : src_(src) {}

    Token next_token() {
        skip_whitespace();
        if (pos_ >= src_.size()) {
            return {TokenType::EndOfFile, "", 0};
        }

        char c = src_[pos_];
        if (c == '{') { pos_++; return {TokenType::BraceOpen, "{", 0}; }
        if (c == '}') { pos_++; return {TokenType::BraceClose, "}", 0}; }
        if (c == '[') { pos_++; return {TokenType::BracketOpen, "[", 0}; }
        if (c == ']') { pos_++; return {TokenType::BracketClose, "]", 0}; }
        if (c == ':') { pos_++; return {TokenType::Colon, ":", 0}; }
        if (c == ',') { pos_++; return {TokenType::Comma, ",", 0}; }

        if (c == '"') {
            return read_string();
        }

        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return read_number();
        }

        if (std::isalpha(static_cast<unsigned char>(c))) {
            return read_word();
        }

        pos_++;
        return {TokenType::Other, std::string(1, c), 0};
    }

private:
    const std::string& src_;
    size_t pos_ = 0;

    void skip_whitespace() {
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (std::isspace(static_cast<unsigned char>(c))) {
                pos_++;
            } else if (c == '/' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '/') {
                // Single-line comment
                pos_ += 2;
                while (pos_ < src_.size() && src_[pos_] != '\n') pos_++;
            } else {
                break;
            }
        }
    }

    Token read_string() {
        pos_++; // skip opening '"'
        std::string s;
        while (pos_ < src_.size() && src_[pos_] != '"') {
            if (src_[pos_] == '\\' && pos_ + 1 < src_.size()) {
                pos_++;
            }
            s += src_[pos_++];
        }
        if (pos_ < src_.size() && src_[pos_] == '"') {
            pos_++;
        }
        return {TokenType::String, s, 0};
    }

    Token read_number() {
        size_t start = pos_;
        bool is_float = false;
        if (src_[pos_] == '-') pos_++;
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            pos_++;
        }
        if (pos_ < src_.size() && src_[pos_] == '.') {
            is_float = true;
            pos_++;
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                pos_++;
            }
        }
        std::string num_str = src_.substr(start, pos_ - start);
        if (is_float) {
            return {TokenType::FloatNumber, num_str, 0};
        } else {
            long long val = 0;
            try {
                val = std::stoll(num_str);
            } catch (...) {
                return {TokenType::Other, num_str, 0};
            }
            return {TokenType::IntNumber, num_str, val};
        }
    }

    Token read_word() {
        size_t start = pos_;
        while (pos_ < src_.size() && std::isalpha(static_cast<unsigned char>(src_[pos_]))) {
            pos_++;
        }
        std::string word = src_.substr(start, pos_ - start);
        if (word == "true" || word == "false") {
            return {TokenType::Boolean, word, 0};
        }
        if (word == "null") {
            return {TokenType::Null, word, 0};
        }
        return {TokenType::Other, word, 0};
    }
};

void skip_json_value(JsonLexer& lexer, const Token& tok) {
    if (tok.type == TokenType::BraceOpen) {
        int depth = 1;
        while (depth > 0) {
            Token t = lexer.next_token();
            if (t.type == TokenType::EndOfFile) break;
            if (t.type == TokenType::BraceOpen) depth++;
            else if (t.type == TokenType::BraceClose) depth--;
        }
    } else if (tok.type == TokenType::BracketOpen) {
        int depth = 1;
        while (depth > 0) {
            Token t = lexer.next_token();
            if (t.type == TokenType::EndOfFile) break;
            if (t.type == TokenType::BracketOpen) depth++;
            else if (t.type == TokenType::BracketClose) depth--;
        }
    }
}

} // namespace

Color parse_hex_color(const std::string& str, Color fallback) {
    std::string s = str;
    if (!s.empty() && s[0] == '#') {
        s = s.substr(1);
    }
    if (s.size() == 3) {
        unsigned int r = 0, g = 0, b = 0;
        if (std::sscanf(s.c_str(), "%1x%1x%1x", &r, &g, &b) == 3) {
            return {static_cast<unsigned char>(r * 17),
                    static_cast<unsigned char>(g * 17),
                    static_cast<unsigned char>(b * 17),
                    255};
        }
    }
    if (s.size() == 6) {
        unsigned int val = 0;
        if (std::sscanf(s.c_str(), "%x", &val) == 1) {
            return {static_cast<unsigned char>((val >> 16) & 0xFF),
                    static_cast<unsigned char>((val >> 8) & 0xFF),
                    static_cast<unsigned char>(val & 0xFF),
                    255};
        }
    }
    if (s.size() == 8) {
        unsigned long long val = 0;
        if (std::sscanf(s.c_str(), "%llx", &val) == 1) {
            return {static_cast<unsigned char>((val >> 24) & 0xFF),
                    static_cast<unsigned char>((val >> 16) & 0xFF),
                    static_cast<unsigned char>((val >> 8) & 0xFF),
                    static_cast<unsigned char>(val & 0xFF)};
        }
    }
    return fallback;
}

bool is_valid_hex_color(const std::string& str) {
    if (str.empty() || str[0] != '#') return false;
    size_t len = str.size() - 1;
    if (len != 3 && len != 6 && len != 8) return false;
    for (size_t i = 1; i < str.size(); ++i) {
        if (!std::isxdigit(static_cast<unsigned char>(str[i]))) return false;
    }
    return true;
}

Color parse_validated_hex_color(const Token& tok, const std::string& theme_name, const std::string& prop_name, Color fallback) {
    if (tok.type != TokenType::String) {
        std::cerr << "Config error: Value for '" << prop_name << "' in theme \"" << theme_name
                  << "\" must be a hex color string (e.g. \"#RRGGBB\"), got ";
        if (tok.type == TokenType::IntNumber || tok.type == TokenType::FloatNumber) std::cerr << "number (" << tok.text << ")";
        else if (tok.type == TokenType::Boolean) std::cerr << "boolean (" << tok.text << ")";
        else if (tok.type == TokenType::Null) std::cerr << "null";
        else std::cerr << "'" << tok.text << "'";
        std::cerr << ". Value rejected, keeping default." << std::endl;
        return fallback;
    }

    if (!is_valid_hex_color(tok.text)) {
        std::cerr << "Config error: Invalid hex color format \"" << tok.text << "\" for '" << prop_name
                  << "' in theme \"" << theme_name
                  << "\". Colors must start with '#' followed by 3, 6, or 8 hex digits (e.g. \"#00FF00\" or \"#00FF00FF\")."
                  << " Value rejected, keeping default." << std::endl;
        return fallback;
    }

    return parse_hex_color(tok.text, fallback);
}

std::string color_to_hex(Color c) {
    char buf[16];
    if (c.a == 255) {
        std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.r, c.g, c.b);
    } else {
        std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", c.r, c.g, c.b, c.a);
    }
    return std::string(buf);
}

ColorTheme ColorTheme::default_theme() {
    ColorTheme t;
    t.name = "default";
    t.background          = {10, 12, 18, 255};      // #0A0C12
    t.pit_corner          = {50, 180, 255, 255};    // #32B4FF
    t.pit_grid            = {40, 130, 200, 140};    // #2882C88C
    t.pit_opening         = {0, 255, 200, 255};     // #00FFC8
    t.pit_depth_rings     = {30, 95, 150, 85};      // #1E5F9655
    t.pit_floor_perimeter = {0, 220, 255, 240};     // #00DCFFF0
    t.pit_floor_grid      = {40, 140, 210, 160};    // #288CD2A0
    t.cube_wireframe      = {255, 255, 255, 200};   // #FFFFFFC8
    t.ghost_wireframe     = {255, 255, 255, 102};   // #FFFFFF66
    t.ghost_alpha         = 0.22f;
    t.piece_colors = {
        {229, 192, 123, 255}, // #E5C07B (0: Gold)
        {86, 182, 194, 255},  // #56B6C2 (1: Cyan)
        {152, 195, 121, 255}, // #98C379 (2: Lime)
        {209, 154, 102, 255}, // #D19A66 (3: Orange)
        {97, 175, 239, 255},  // #61AFEF (4: Sky Blue)
        {198, 120, 221, 255}, // #C678DD (5: Purple)
        {224, 108, 117, 255}, // #E06C75 (6: Coral Red)
        {78, 201, 176, 255}   // #4EC9B0 (7: Teal)
    };
    t.layer_colors = {
        {77, 149, 214, 255},  // 0: Floor - #4D95D6
        {86, 182, 194, 255},  // 1: #56B6C2
        {152, 195, 121, 255}, // 2: #98C379
        {126, 199, 123, 255}, // 3: #7EC77B
        {229, 192, 123, 255}, // 4: #E5C07B
        {209, 154, 102, 255}, // 5: #D19A66
        {224, 108, 117, 255}, // 6: #E06C75
        {198, 120, 221, 255}, // 7: #C678DD
        {138, 99, 210, 255},  // 8: #8A63D2
        {78, 201, 176, 255},  // 9: #4EC9B0
        {255, 141, 161, 255}, // 10: #FF8DA1
        {171, 178, 191, 255}  // 11: Opening - #ABB2BF
    };
    t.hud_background      = {15, 20, 30, 255};      // #0F141E
    t.hud_border          = {40, 100, 160, 255};    // #2864A0
    t.hud_title           = {0, 220, 255, 255};     // #00DCFF
    t.hud_score           = {255, 235, 120, 255};   // #FFE578
    t.hud_label           = {140, 170, 200, 255};   // #8CAAC8
    return t;
}

ColorTheme ColorTheme::dark_theme() {
    ColorTheme t;
    t.name = "dark";
    t.background          = {5, 7, 10, 255};        // #05070A Deep midnight black
    t.pit_corner          = {60, 160, 255, 255};    // #3CA0FF
    t.pit_grid            = {25, 60, 100, 130};     // #193C6482
    t.pit_opening         = {0, 230, 180, 255};     // #00E6B4
    t.pit_depth_rings     = {20, 45, 80, 75};       // #142D504B
    t.pit_floor_perimeter = {0, 200, 255, 255};     // #00C8FFFF Bold high-contrast floor ring
    t.pit_floor_grid      = {30, 80, 130, 180};     // #1E5082B4
    t.cube_wireframe      = {255, 255, 255, 255};   // #FFFFFFFF Solid opaque white box borders
    t.ghost_wireframe     = {200, 220, 255, 120};   // #C8DCFF78
    t.ghost_alpha         = 0.18f;
    t.piece_colors = {
        {212, 168, 83, 255},  // #D4A853 (0: Deep Gold)
        {62, 156, 168, 255},  // #3E9CA8 (1: Deep Cyan)
        {125, 168, 94, 255},  // #7DA85E (2: Forest Green)
        {184, 126, 74, 255},  // #B87E4A (3: Rust Orange)
        {77, 149, 214, 255},  // #4D95D6 (4: Deep Sky Blue)
        {168, 94, 192, 255},  // #A85EC0 (5: Vivid Purple)
        {200, 78, 88, 255},   // #C84E58 (6: Crimson Red)
        {59, 168, 146, 255}   // #3BA892 (7: Deep Emerald)
    };
    t.layer_colors = {
        {45, 104, 196, 255},  // 0: Floor - #2D68C4
        {0, 168, 168, 255},   // 1: #00A8A8
        {38, 166, 91, 255},   // 2: #26A65B
        {135, 211, 124, 255}, // 3: #87D37C
        {229, 184, 66, 255},  // 4: #E5B842
        {211, 84, 0, 255},    // 5: #D35400
        {192, 57, 43, 255},   // 6: #C0392B
        {155, 89, 182, 255},  // 7: #9B59B6
        {108, 92, 231, 255},  // 8: #6C5CE7
        {9, 132, 227, 255},   // 9: #0984E3
        {232, 67, 147, 255},  // 10: #E84393
        {189, 195, 199, 255}  // 11: Opening - #BDC3C7
    };
    t.hud_background      = {10, 14, 20, 255};      // #0A0E14
    t.hud_border          = {30, 70, 110, 255};     // #1E466E
    t.hud_title           = {0, 200, 230, 255};     // #00C8E6
    t.hud_score           = {255, 215, 80, 255};    // #FFD750
    t.hud_label           = {120, 150, 180, 255};   // #7896B4
    return t;
}

ColorTheme ColorTheme::blockout2_theme() {
    ColorTheme t;
    t.name                = "blockout2";
    t.background          = {0, 0, 0, 255};          // #000000 Pure black
    t.pit_corner          = {0, 204, 0, 255};        // #00CC00 Classic BlockOut green
    t.pit_grid            = {0, 153, 0, 136};        // #00990088 Green wireframe grid
    t.pit_opening         = {0, 255, 0, 255};        // #00FF00 Bright green opening
    t.pit_depth_rings     = {0, 119, 0, 102};        // #00770066 Depth rings
    t.pit_floor_perimeter = {0, 221, 0, 255};        // #00DD00 Back perimeter
    t.pit_floor_grid      = {0, 153, 0, 176};        // #009900B0 Floor grid
    t.cube_wireframe      = {0, 0, 0, 255};          // #000000FF Black cube edges
    t.active_wireframe    = {255, 255, 255, 255};    // #FFFFFFFF Pure white falling block
    t.ghost_wireframe     = {128, 128, 128, 128};    // #80808080 Gray ghost outline
    t.ghost_alpha         = 0.25f;
    t.piece_colors = {
        {32, 64, 255, 255},   // #2040FF (Blue, Case 0)
        {0, 230, 0, 255},     // #00E600 (Green, Case 1)
        {0, 230, 230, 255},   // #00E6E6 (Cyan, Case 2)
        {230, 16, 16, 255},   // #E61010 (Red, Case 3)
        {255, 26, 204, 255},  // #FF1ACC (Magenta, Case 4)
        {230, 153, 0, 255},   // #E69900 (Orange, Case 5)
        {217, 217, 217, 255}, // #D9D9D9 (Silver, Case 6)
        {255, 230, 0, 255}    // #FFE600 (Yellow, DOS BlockOut)
    };
    t.layer_colors = {
        {32, 64, 255, 255},   // 0: Floor - #2040FF (Deep Blue)
        {0, 230, 230, 255},   // 1: #00E6E6 (Teal / Cyan)
        {0, 230, 0, 255},     // 2: #00E600 (Green)
        {255, 230, 0, 255},   // 3: #FFE600 (Yellow)
        {230, 153, 0, 255},   // 4: #E69900 (Orange)
        {230, 16, 16, 255},   // 5: #E61010 (Red)
        {255, 26, 204, 255},  // 6: #FF1ACC (Magenta)
        {155, 48, 255, 255},  // 7: #9B30FF (Purple)
        {0, 255, 200, 255},   // 8: #00FFC8 (Neon Mint)
        {255, 112, 166, 255}, // 9: #FF70A6 (Pink)
        {92, 147, 255, 255},  // 10: #5C93FF (Sky Blue)
        {217, 217, 217, 255}  // 11: Opening - #D9D9D9 (Silver / White)
    };
    t.hud_background      = {0, 0, 0, 255};          // #000000
    t.hud_border          = {0, 153, 0, 255};        // #009900
    t.hud_title           = {0, 255, 0, 255};        // #00FF00
    t.hud_score           = {255, 230, 0, 255};      // #FFE600
    t.hud_label           = {102, 204, 102, 255};    // #66CC66
    return t;
}

const ColorTheme& Config::get_theme() const {
    auto it = themes.find(theme);
    if (it != themes.end()) {
        return it->second;
    }
    static const ColorTheme def = ColorTheme::default_theme();
    return def;
}

void Config::save(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) return;

    std::map<std::string, ColorTheme> save_themes = themes;
    if (save_themes.find("default") == save_themes.end()) {
        save_themes["default"] = ColorTheme::default_theme();
    }
    if (save_themes.find("dark") == save_themes.end()) {
        save_themes["dark"] = ColorTheme::dark_theme();
    }
    if (save_themes.find("blockout2") == save_themes.end()) {
        save_themes["blockout2"] = ColorTheme::blockout2_theme();
    }

    out << "{\n";
    out << "  \"width\": " << width << ",\n";
    out << "  \"length\": " << length << ",\n";
    out << "  \"depth\": " << depth << ",\n";
    out << "  \"window_width\": " << window_width << ",\n";
    out << "  \"window_height\": " << window_height << ",\n";
    out << "  \"preview_next_piece\": " << (preview_next_piece ? "true" : "false") << ",\n";
    out << "  \"color_by_layer\": " << (color_by_layer ? "true" : "false") << ",\n";
    out << "  \"difficulty\": \"" << difficulty << "\",\n";
    out << "  \"step_times\": {\n";
    out << "    \"easy\": " << step_times.easy << ",\n";
    out << "    \"normal\": " << step_times.normal << ",\n";
    out << "    \"hard\": " << step_times.hard << ",\n";
    out << "    \"extreme\": " << step_times.extreme << "\n";
    out << "  },\n";
    out << "  \"theme\": \"" << theme << "\",\n";
    out << "  \"themes\": {\n";

    size_t theme_idx = 0;
    for (const auto& [name, th] : save_themes) {
        out << "    \"" << name << "\": {\n";
        out << "      \"background\": \"" << color_to_hex(th.background) << "\",\n";
        out << "      \"pit_corner\": \"" << color_to_hex(th.pit_corner) << "\",\n";
        out << "      \"pit_grid\": \"" << color_to_hex(th.pit_grid) << "\",\n";
        out << "      \"pit_opening\": \"" << color_to_hex(th.pit_opening) << "\",\n";
        out << "      \"pit_depth_rings\": \"" << color_to_hex(th.pit_depth_rings) << "\",\n";
        out << "      \"pit_floor_perimeter\": \"" << color_to_hex(th.pit_floor_perimeter) << "\",\n";
        out << "      \"pit_floor_grid\": \"" << color_to_hex(th.pit_floor_grid) << "\",\n";
        out << "      \"cube_wireframe\": \"" << color_to_hex(th.cube_wireframe) << "\",\n";
        out << "      \"active_wireframe\": \"" << color_to_hex(th.active_wireframe) << "\",\n";
        out << "      \"ghost_wireframe\": \"" << color_to_hex(th.ghost_wireframe) << "\",\n";
        out << "      \"ghost_alpha\": " << th.ghost_alpha << ",\n";
        out << "      \"piece_colors\": [\n";
        for (size_t i = 0; i < th.piece_colors.size(); ++i) {
            out << "        \"" << color_to_hex(th.piece_colors[i]) << "\""
                << (i + 1 < th.piece_colors.size() ? "," : "") << "\n";
        }
        out << "      ],\n";
        out << "      \"layer_colors\": [\n";
        for (size_t i = 0; i < th.layer_colors.size(); ++i) {
            out << "        \"" << color_to_hex(th.layer_colors[i]) << "\""
                << (i + 1 < th.layer_colors.size() ? "," : "") << "\n";
        }
        out << "      ],\n";
        out << "      \"hud_background\": \"" << color_to_hex(th.hud_background) << "\",\n";
        out << "      \"hud_border\": \"" << color_to_hex(th.hud_border) << "\",\n";
        out << "      \"hud_title\": \"" << color_to_hex(th.hud_title) << "\",\n";
        out << "      \"hud_score\": \"" << color_to_hex(th.hud_score) << "\",\n";
        out << "      \"hud_label\": \"" << color_to_hex(th.hud_label) << "\"\n";
        out << "    }" << (++theme_idx < save_themes.size() ? "," : "") << "\n";
    }
    out << "  }\n";
    out << "}\n";
}

Config Config::load(const std::string& filename) {
    Config cfg; // default: width=7, length=7, depth=12, preview_next_piece=false, theme="default"
    cfg.themes["default"] = ColorTheme::default_theme();
    cfg.themes["dark"] = ColorTheme::dark_theme();
    cfg.themes["blockout2"] = ColorTheme::blockout2_theme();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Notice: '" << filename << "' not found. Creating default configuration (7x7x12)." << std::endl;
        cfg.save(filename);
        return cfg;
    }

    std::stringstream buf;
    buf << file.rdbuf();
    std::string content = buf.str();

    JsonLexer lexer(content);
    Token tok = lexer.next_token();

    if (tok.type != TokenType::BraceOpen) {
        std::cerr << "Config error: '" << filename << "' must start with '{'. Using default 7x7x12." << std::endl;
        return cfg;
    }

    while (true) {
        tok = lexer.next_token();
        if (tok.type == TokenType::BraceClose || tok.type == TokenType::EndOfFile) {
            break;
        }

        if (tok.type == TokenType::Comma) {
            continue;
        }

        if (tok.type != TokenType::String) {
            std::cerr << "Config warning: Expected string key, got '" << tok.text << "'." << std::endl;
            continue;
        }

        std::string key = tok.text;
        Token colon = lexer.next_token();
        if (colon.type != TokenType::Colon) {
            std::cerr << "Config error: Expected ':' after key \"" << key << "\"." << std::endl;
            continue;
        }

        Token val = lexer.next_token();

        // Boolean setting: preview_next_piece
        // Boolean setting: preview_next_piece
        if (key == "preview_next_piece" || key == "preview_next_block" || key == "show_next_piece") {
            if (val.type == TokenType::Boolean) {
                cfg.preview_next_piece = (val.text == "true");
            } else {
                std::cerr << "Config error: Value for \"" << key
                          << "\" must be a boolean (true/false), but got ";
                if (val.type == TokenType::String) std::cerr << "string (\"" << val.text << "\")";
                else if (val.type == TokenType::FloatNumber) std::cerr << "floating-point number (" << val.text << ")";
                else if (val.type == TokenType::IntNumber) std::cerr << "integer (" << val.text << ")";
                else if (val.type == TokenType::Null) std::cerr << "null";
                else std::cerr << "'" << val.text << "'";
                std::cerr << ". Value rejected, keeping default "
                          << (cfg.preview_next_piece ? "true" : "false") << "." << std::endl;
            }
            continue;
        }

        // Boolean setting: color_by_layer
        if (key == "color_by_layer" || key == "layer_coloring") {
            if (val.type == TokenType::Boolean) {
                cfg.color_by_layer = (val.text == "true");
            } else {
                std::cerr << "Config error: Value for \"" << key
                          << "\" must be a boolean (true/false), but got ";
                if (val.type == TokenType::String) std::cerr << "string (\"" << val.text << "\")";
                else if (val.type == TokenType::FloatNumber) std::cerr << "floating-point number (" << val.text << ")";
                else if (val.type == TokenType::IntNumber) std::cerr << "integer (" << val.text << ")";
                else if (val.type == TokenType::Null) std::cerr << "null";
                else std::cerr << "'" << val.text << "'";
                std::cerr << ". Value rejected, keeping default "
                          << (cfg.color_by_layer ? "true" : "false") << "." << std::endl;
            }
            continue;
        }

        // Difficulty setting: easy, normal, hard, extreme
        if (key == "difficulty") {
            if (val.type == TokenType::String) {
                std::string d_str = val.text;
                for (char& c : d_str) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (d_str == "easy" || d_str == "normal" || d_str == "hard" || d_str == "extreme") {
                    cfg.difficulty = d_str;
                } else {
                    std::cerr << "Config error: Unknown difficulty \"" << val.text
                              << "\". Valid options: easy, normal, hard, extreme. Defaulting to easy." << std::endl;
                    cfg.difficulty = "easy";
                }
            } else {
                std::cerr << "Config error: Value for 'difficulty' must be a string name, got ";
                if (val.type == TokenType::IntNumber || val.type == TokenType::FloatNumber) std::cerr << "number (" << val.text << ")";
                else if (val.type == TokenType::Boolean) std::cerr << "boolean (" << val.text << ")";
                else if (val.type == TokenType::Null) std::cerr << "null";
                else std::cerr << "'" << val.text << "'";
                std::cerr << ". Value rejected, keeping default \"easy\"." << std::endl;
            }
            continue;
        }

        // Step times configuration: object { "easy": 5.51, "normal": 2.26, "hard": 0.92, "extreme": 0.38 }
        if (key == "step_times" || key == "difficulty_step_times" || key == "initial_step_times") {
            if (val.type != TokenType::BraceOpen) {
                std::cerr << "Config error: '" << key << "' must be an object { \"easy\": ..., \"normal\": ..., ... }." << std::endl;
                skip_json_value(lexer, val);
                continue;
            }

            while (true) {
                Token sub_tok = lexer.next_token();
                if (sub_tok.type == TokenType::BraceClose || sub_tok.type == TokenType::EndOfFile) {
                    break;
                }
                if (sub_tok.type == TokenType::Comma) continue;
                if (sub_tok.type != TokenType::String) {
                    std::cerr << "Config warning: Expected difficulty tier name in '" << key << "', got '" << sub_tok.text << "'." << std::endl;
                    continue;
                }

                std::string sub_key = sub_tok.text;
                for (char& c : sub_key) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

                Token sub_colon = lexer.next_token();
                if (sub_colon.type != TokenType::Colon) continue;

                Token sub_val = lexer.next_token();
                if (sub_val.type != TokenType::FloatNumber && sub_val.type != TokenType::IntNumber) {
                    std::cerr << "Config error: Step time for '" << sub_key << "' must be a number, but got ";
                    if (sub_val.type == TokenType::String) std::cerr << "string (\"" << sub_val.text << "\")";
                    else if (sub_val.type == TokenType::Boolean) std::cerr << "boolean (" << sub_val.text << ")";
                    else if (sub_val.type == TokenType::Null) std::cerr << "null";
                    else std::cerr << "'" << sub_val.text << "'";
                    std::cerr << ". Value rejected, keeping default." << std::endl;
                    continue;
                }

                float fval = 0.0f;
                try {
                    fval = std::stof(sub_val.text);
                } catch (...) {
                    std::cerr << "Config error: Failed to parse step time for '" << sub_key << "' from '" << sub_val.text << "'." << std::endl;
                    continue;
                }

                if (fval < 0.1f || fval > 20.0f) {
                    std::cerr << "Config error: Step time for '" << sub_key << "' must be between 0.1 and 20.0 seconds, got "
                              << fval << "s. Value rejected, keeping default." << std::endl;
                    continue;
                }

                if (sub_key == "easy") cfg.step_times.easy = fval;
                else if (sub_key == "normal" || sub_key == "medium" || sub_key == "med") cfg.step_times.normal = fval;
                else if (sub_key == "hard") cfg.step_times.hard = fval;
                else if (sub_key == "extreme" || sub_key == "insane") cfg.step_times.extreme = fval;
                else {
                    std::cerr << "Config warning: Unknown difficulty tier '" << sub_key
                              << "' in '" << key << "'. Valid options: easy, normal, hard, extreme." << std::endl;
                }
            }
            continue;
        }

        // Active Theme setting: string name
        if (key == "theme") {
            if (val.type == TokenType::String) {
                cfg.theme = val.text;
            } else {
                std::cerr << "Config error: Value for 'theme' must be a string name, got ";
                if (val.type == TokenType::IntNumber || val.type == TokenType::FloatNumber) std::cerr << "number (" << val.text << ")";
                else if (val.type == TokenType::Boolean) std::cerr << "boolean (" << val.text << ")";
                else if (val.type == TokenType::Null) std::cerr << "null";
                else std::cerr << "'" << val.text << "'";
                std::cerr << ". Value rejected, keeping default \"default\"." << std::endl;
            }
            continue;
        }

        // Themes dictionary
        if (key == "themes") {
            if (val.type != TokenType::BraceOpen) {
                std::cerr << "Config error: 'themes' must be an object { ... }." << std::endl;
                skip_json_value(lexer, val);
                continue;
            }

            while (true) {
                Token theme_tok = lexer.next_token();
                if (theme_tok.type == TokenType::BraceClose || theme_tok.type == TokenType::EndOfFile) {
                    break;
                }
                if (theme_tok.type == TokenType::Comma) continue;
                if (theme_tok.type != TokenType::String) {
                    std::cerr << "Config warning: Expected theme name string, got '" << theme_tok.text << "'." << std::endl;
                    continue;
                }

                std::string theme_name = theme_tok.text;
                Token theme_colon = lexer.next_token();
                if (theme_colon.type != TokenType::Colon) continue;

                Token theme_brace = lexer.next_token();
                if (theme_brace.type != TokenType::BraceOpen) {
                    std::cerr << "Config error: Expected '{' for theme \"" << theme_name << "\"." << std::endl;
                    skip_json_value(lexer, theme_brace);
                    continue;
                }

                ColorTheme th = (theme_name == "dark") ? ColorTheme::dark_theme() :
                                (theme_name == "blockout2") ? ColorTheme::blockout2_theme() :
                                ColorTheme::default_theme();
                th.name = theme_name;

                while (true) {
                    Token prop_tok = lexer.next_token();
                    if (prop_tok.type == TokenType::BraceClose || prop_tok.type == TokenType::EndOfFile) {
                        break;
                    }
                    if (prop_tok.type == TokenType::Comma) continue;
                    if (prop_tok.type != TokenType::String) continue;

                    std::string prop_name = prop_tok.text;
                    Token prop_colon = lexer.next_token();
                    if (prop_colon.type != TokenType::Colon) continue;

                    Token prop_val = lexer.next_token();
                    if (prop_name == "background") th.background = parse_validated_hex_color(prop_val, theme_name, prop_name, th.background);
                    else if (prop_name == "pit_corner") th.pit_corner = parse_validated_hex_color(prop_val, theme_name, prop_name, th.pit_corner);
                    else if (prop_name == "pit_grid") th.pit_grid = parse_validated_hex_color(prop_val, theme_name, prop_name, th.pit_grid);
                    else if (prop_name == "pit_opening") th.pit_opening = parse_validated_hex_color(prop_val, theme_name, prop_name, th.pit_opening);
                    else if (prop_name == "pit_depth_rings") th.pit_depth_rings = parse_validated_hex_color(prop_val, theme_name, prop_name, th.pit_depth_rings);
                    else if (prop_name == "pit_floor_perimeter") th.pit_floor_perimeter = parse_validated_hex_color(prop_val, theme_name, prop_name, th.pit_floor_perimeter);
                    else if (prop_name == "pit_floor_grid") th.pit_floor_grid = parse_validated_hex_color(prop_val, theme_name, prop_name, th.pit_floor_grid);
                    else if (prop_name == "cube_wireframe") th.cube_wireframe = parse_validated_hex_color(prop_val, theme_name, prop_name, th.cube_wireframe);
                    else if (prop_name == "active_wireframe") th.active_wireframe = parse_validated_hex_color(prop_val, theme_name, prop_name, th.active_wireframe);
                    else if (prop_name == "ghost_wireframe") th.ghost_wireframe = parse_validated_hex_color(prop_val, theme_name, prop_name, th.ghost_wireframe);
                    else if (prop_name == "hud_background") th.hud_background = parse_validated_hex_color(prop_val, theme_name, prop_name, th.hud_background);
                    else if (prop_name == "hud_border") th.hud_border = parse_validated_hex_color(prop_val, theme_name, prop_name, th.hud_border);
                    else if (prop_name == "hud_title") th.hud_title = parse_validated_hex_color(prop_val, theme_name, prop_name, th.hud_title);
                    else if (prop_name == "hud_score") th.hud_score = parse_validated_hex_color(prop_val, theme_name, prop_name, th.hud_score);
                    else if (prop_name == "hud_label") th.hud_label = parse_validated_hex_color(prop_val, theme_name, prop_name, th.hud_label);
                    else if (prop_name == "ghost_alpha") {
                        if (prop_val.type == TokenType::FloatNumber || prop_val.type == TokenType::IntNumber) {
                            try {
                                float a = std::stof(prop_val.text);
                                if (a < 0.0f || a > 1.0f) {
                                    std::cerr << "Config error: 'ghost_alpha' in theme \"" << theme_name
                                              << "\" must be between 0.0 and 1.0, got " << a
                                              << ". Keeping default " << th.ghost_alpha << "." << std::endl;
                                } else {
                                    th.ghost_alpha = a;
                                }
                            } catch (...) {
                                std::cerr << "Config error: Invalid number for 'ghost_alpha': '" << prop_val.text << "'." << std::endl;
                            }
                        } else {
                            std::cerr << "Config error: 'ghost_alpha' in theme \"" << theme_name
                                      << "\" must be a number between 0.0 and 1.0, got '" << prop_val.text << "'." << std::endl;
                        }
                    }
                    else if (prop_name == "piece_colors") {
                        if (prop_val.type == TokenType::BracketOpen) {
                            std::vector<Color> colors;
                            size_t color_idx = 0;
                            while (true) {
                                Token arr_tok = lexer.next_token();
                                if (arr_tok.type == TokenType::BracketClose || arr_tok.type == TokenType::EndOfFile) break;
                                if (arr_tok.type == TokenType::Comma) continue;

                                Color default_col = (color_idx < th.piece_colors.size()) ? th.piece_colors[color_idx] : WHITE;
                                if (arr_tok.type != TokenType::String || !is_valid_hex_color(arr_tok.text)) {
                                    std::cerr << "Config error: Invalid hex color format \"" << arr_tok.text
                                              << "\" at element " << color_idx << " of 'piece_colors' in theme \""
                                              << theme_name << "\". Using default." << std::endl;
                                    colors.push_back(default_col);
                                } else {
                                    colors.push_back(parse_hex_color(arr_tok.text, default_col));
                                }
                                color_idx++;
                            }
                            if (!colors.empty()) {
                                if (colors.size() < 8) {
                                    std::cerr << "Config warning: 'piece_colors' in theme \"" << theme_name
                                              << "\" contains " << colors.size() << " colors (8 required for Flat set). Padding with defaults." << std::endl;
                                    while (colors.size() < 8 && colors.size() < th.piece_colors.size()) {
                                        colors.push_back(th.piece_colors[colors.size()]);
                                    }
                                }
                                th.piece_colors = std::move(colors);
                            }
                        } else {
                            std::cerr << "Config error: 'piece_colors' in theme \"" << theme_name
                                      << "\" must be an array of hex colors [ \"#...\", ... ]." << std::endl;
                            skip_json_value(lexer, prop_val);
                        }
                    } else if (prop_name == "layer_colors") {
                        if (prop_val.type == TokenType::BracketOpen) {
                            std::vector<Color> colors;
                            size_t color_idx = 0;
                            while (true) {
                                Token arr_tok = lexer.next_token();
                                if (arr_tok.type == TokenType::BracketClose || arr_tok.type == TokenType::EndOfFile) break;
                                if (arr_tok.type == TokenType::Comma) continue;

                                Color default_col = (color_idx < th.layer_colors.size()) ? th.layer_colors[color_idx] : WHITE;
                                if (arr_tok.type != TokenType::String || !is_valid_hex_color(arr_tok.text)) {
                                    std::cerr << "Config error: Invalid hex color format \"" << arr_tok.text
                                              << "\" at element " << color_idx << " of 'layer_colors' in theme \""
                                              << theme_name << "\". Using default." << std::endl;
                                    colors.push_back(default_col);
                                } else {
                                    colors.push_back(parse_hex_color(arr_tok.text, default_col));
                                }
                                color_idx++;
                            }
                            if (!colors.empty()) {
                                th.layer_colors = std::move(colors);
                            }
                        } else {
                            std::cerr << "Config error: 'layer_colors' in theme \"" << theme_name
                                      << "\" must be an array of hex colors [ \"#...\", ... ]." << std::endl;
                            skip_json_value(lexer, prop_val);
                        }
                    } else {
                        skip_json_value(lexer, prop_val);
                    }
                }
                cfg.themes[theme_name] = th;
            }
            continue;
        }

        // Type Verification for integer settings
        if (val.type != TokenType::IntNumber) {
            std::cerr << "Config error: Value for \"" << key
                      << "\" must be an integer, but got ";
            if (val.type == TokenType::String) std::cerr << "string (\"" << val.text << "\")";
            else if (val.type == TokenType::FloatNumber) std::cerr << "floating-point number (" << val.text << ")";
            else if (val.type == TokenType::Boolean) std::cerr << "boolean (" << val.text << ")";
            else if (val.type == TokenType::Null) std::cerr << "null";
            else std::cerr << "'" << val.text << "'";
            std::cerr << ". Value rejected, keeping default." << std::endl;
            continue;
        }

        int int_val = static_cast<int>(val.int_value);

        // Range Verification
        if (key == "width") {
            if (int_val < 3 || int_val > 7) {
                std::cerr << "Config error: 'width' must be between 3 and 7 (inclusive), got "
                          << int_val << ". Keeping default " << cfg.width << "." << std::endl;
            } else {
                cfg.width = int_val;
            }
        } else if (key == "length" || key == "height") {
            if (int_val < 3 || int_val > 7) {
                std::cerr << "Config error: 'length' must be between 3 and 7 (inclusive), got "
                          << int_val << ". Keeping default " << cfg.length << "." << std::endl;
            } else {
                cfg.length = int_val;
            }
        } else if (key == "depth") {
            if (int_val < 6 || int_val > 18) {
                std::cerr << "Config error: 'depth' must be between 6 and 18 (inclusive), got "
                          << int_val << ". Keeping default " << cfg.depth << "." << std::endl;
            } else {
                cfg.depth = int_val;
            }
        } else if (key == "window_width") {
            if (int_val < 800 || int_val > 7680) {
                std::cerr << "Config error: 'window_width' must be between 800 and 7680, got "
                          << int_val << ". Keeping default " << cfg.window_width << "." << std::endl;
            } else {
                cfg.window_width = int_val;
            }
        } else if (key == "window_height") {
            if (int_val < 600 || int_val > 4320) {
                std::cerr << "Config error: 'window_height' must be between 600 and 4320, got "
                          << int_val << ". Keeping default " << cfg.window_height << "." << std::endl;
            } else {
                cfg.window_height = int_val;
            }
        } else {
            std::cerr << "Config warning: Unknown setting \"" << key << "\" ignored." << std::endl;
        }
    }

    // Verify proportions / aspect ratio to prevent unusable distorted windows (e.g. 12x500 or extreme slits)
    float aspect = static_cast<float>(cfg.window_width) / static_cast<float>(cfg.window_height);
    if (aspect < 0.75f || aspect > 3.6f) {
        std::cerr << "Config error: Window proportions " << cfg.window_width << "x" << cfg.window_height
                  << " (aspect ratio " << aspect << ") are invalid. Sensible range is [0.75, 3.6]. "
                  << "Resetting window to 1024x768." << std::endl;
        cfg.window_width = 1024;
        cfg.window_height = 768;
    }

    // Ensure default themes exist
    if (cfg.themes.find("default") == cfg.themes.end()) {
        cfg.themes["default"] = ColorTheme::default_theme();
    }
    if (cfg.themes.find("dark") == cfg.themes.end()) {
        cfg.themes["dark"] = ColorTheme::dark_theme();
    }

    // Verify active theme
    if (cfg.themes.find(cfg.theme) == cfg.themes.end()) {
        std::cerr << "Config warning: Theme \"" << cfg.theme
                  << "\" not found in themes list. Falling back to default colors." << std::endl;
        cfg.theme = "default";
    }

    std::cout << "Configuration loaded: Pit " << cfg.width << "x" << cfg.length << " (length) x " << cfg.depth
              << " (depth), Window " << cfg.window_width << "x" << cfg.window_height
              << ", Difficulty: \"" << cfg.difficulty << "\""
              << " (" << cfg.step_times.get(parse_difficulty(cfg.difficulty)) << "s)"
              << ", Theme: \"" << cfg.theme << "\", Layer Coloring: "
              << (cfg.color_by_layer ? "Enabled" : "Disabled")
              << ", Next Piece Preview: "
              << (cfg.preview_next_piece ? "Enabled" : "Disabled") << std::endl;
    return cfg;
}

} // namespace blockout
