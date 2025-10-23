#pragma once
#include <random>
#include <string>
#include <cctype>

// ------------------------------------------------------------
// RNG — reusable random number generator wrapper
// ------------------------------------------------------------
struct RNG {
    std::mt19937 gen;
    explicit RNG(unsigned seed = std::random_device{}()) : gen(seed) {}

    template <class Int>
    Int irand(Int lo, Int hi) {
        std::uniform_int_distribution<Int> d(lo, hi);
        return d(gen);
    }

    bool coin(double p = 0.5) {
        std::bernoulli_distribution d(p);
        return d(gen);
    }
};

// ------------------------------------------------------------
// String helpers
// ------------------------------------------------------------
inline std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

inline char toUpperChar(char c) {
    return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
}
