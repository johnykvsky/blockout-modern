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

} // namespace

void Config::save(const std::string& filename) const {
    std::ofstream out(filename);
    if (out.is_open()) {
        out << "{\n";
        out << "  \"width\": " << width << ",\n";
        out << "  \"length\": " << length << ",\n";
        out << "  \"depth\": " << depth << ",\n";
        out << "  \"window_width\": " << window_width << ",\n";
        out << "  \"window_height\": " << window_height << "\n";
        out << "}\n";
    }
}

Config Config::load(const std::string& filename) {
    Config cfg; // default: width=7, length=7, depth=12

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

        // Type Verification
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

    std::cout << "Configuration loaded: Pit " << cfg.width << "x" << cfg.length << " (length) x " << cfg.depth
              << " (depth), Window " << cfg.window_width << "x" << cfg.window_height << std::endl;
    return cfg;
}

} // namespace blockout
