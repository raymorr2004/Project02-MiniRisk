#include "Board.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>

// ---------- Construction ----------
Board::Board(std::vector<Territory> territories)
    : territories_(std::move(territories)) {}

// ---------- Accessors ----------
const std::vector<Territory>& Board::getTerritories() const { return territories_; }
std::vector<Territory>& Board::getTerritories() { return territories_; }
int Board::count() const { return static_cast<int>(territories_.size()); }
const Territory& Board::at(TerrId id) const { return territories_.at(id); }
Territory& Board::at(TerrId id) { return territories_.at(id); }

// ---------- Adjacency Helpers ----------
const std::vector<TerrId>& Board::neighbors(TerrId id) const { return territories_.at(id).adj; }

bool Board::areAdjacent(TerrId a, TerrId b) const {
    const auto& nb = neighbors(a);
    return std::find(nb.begin(), nb.end(), b) != nb.end();
}

// ---------- ASCII Rendering ----------
std::string Board::render(bool showOwner, bool showArmies, int cellWidth) const {
    int maxR = 0, maxC = 0;
    for (const auto& x : territories_) {
        maxR = std::max(maxR, x.r);
        maxC = std::max(maxC, x.c);
    }

    const int rows = maxR + 1;
    const int cols = maxC + 1;
    std::vector<std::string> canvas(rows, std::string(cols * cellWidth, '.'));

    auto ownerChar = [&](PlayerId p) -> char {
        switch (p) {
            case PlayerId::P1:  return '1';
            case PlayerId::P2:  return '2';
            default:            return ' ';
        }
    };

    for (const auto& x : territories_) {
        if (x.r < 0 || x.c < 0 || x.r >= rows || x.c >= cols) continue;
        const int colOffset = x.c * cellWidth;

        std::string cell(cellWidth, ' ');
        cell[0] = x.code;
        int writePos = 1;

        if (showOwner && writePos < cellWidth)
            cell[writePos++] = ownerChar(x.owner);

        if (showArmies && writePos < cellWidth) {
            std::string a = std::to_string(std::max(0, x.armies));
            int remain = cellWidth - writePos;
            if ((int)a.size() <= remain) {
                int start = writePos + (remain - static_cast<int>(a.size()));
                for (size_t i = 0; i < a.size() && (start + (int)i) < cellWidth; ++i)
                    cell[start + (int)i] = a[i];
            } else {
                for (int i = 0; i < remain; ++i)
                    cell[writePos + i] = a[(int)a.size() - remain + i];
            }
        }

        for (int k = 0; k < cellWidth && (colOffset + k) < (int)canvas[x.r].size(); ++k)
            canvas[x.r][colOffset + k] = cell[k];
    }

    std::ostringstream out;
    for (const auto& line : canvas)
        out << line << '\n';
    return out.str();
}

// ---------- Color Rendering ----------
std::string Board::renderColor(int cellWidth) const {
    int maxR = 0, maxC = 0;
    for (const auto& x : territories_) {
        maxR = std::max(maxR, x.r);
        maxC = std::max(maxC, x.c);
    }

    const int rows = maxR + 1, cols = maxC + 1;

    auto terrAt = [&](int r, int c) -> const Territory* {
        for (const auto& t : territories_) if (t.r == r && t.c == c) return &t;
        return nullptr;
    };

    auto bg = [](const char* code){ return std::string("\x1b[") + code + "m"; };
    const std::string RESET    = "\x1b[0m";
    const std::string FG_WHITE = "\x1b[97m";
    const std::string BG_BLACK = bg("40");
    const std::string BG_GREY  = bg("100");
    const std::string BG_BLUE  = bg("44");
    const std::string BG_RED   = bg("41");

    cellWidth = std::max(2, cellWidth);
    std::string out;

    for (int r = 0; r < rows; ++r) {
        // Top line
        for (int c = 0; c < cols; ++c) {
            const Territory* T = terrAt(r, c);
            std::string cell(cellWidth, ' ');
            if (T) cell[0] = T->code;

            if (!T) out += BG_BLACK;
            else if (T->owner == PlayerId::P1) out += BG_BLUE;
            else if (T->owner == PlayerId::P2) out += BG_RED;
            else out += BG_GREY;

            out += FG_WHITE + cell + RESET;
        }
        out += '\n';

        // Bottom line (armies)
        for (int c = 0; c < cols; ++c) {
            const Territory* T = terrAt(r, c);
            std::string cell(cellWidth, ' ');
            if (T) {
                std::string a = std::to_string(std::max(0, T->armies));
                if ((int)a.size() > cellWidth) a = a.substr(a.size() - cellWidth);
                int start = cellWidth - (int)a.size();
                for (size_t i = 0; i < a.size(); ++i)
                    if (start + (int)i < cellWidth)
                        cell[start + (int)i] = a[i];
            }

            if (!T) out += BG_BLACK;
            else if (T->owner == PlayerId::P1) out += BG_BLUE;
            else if (T->owner == PlayerId::P2) out += BG_RED;
            else out += BG_GREY;

            out += FG_WHITE + cell + RESET;
        }
        out += '\n';
    }

    return out;
}

// ---------- Validators ----------
bool Board::validateAdjUndirected() const {
    for (int a = 0; a < (int)territories_.size(); ++a) {
        for (TerrId b : territories_[a].adj) {
            if (b < 0 || b >= (int)territories_.size()) return false;
            const auto& back = territories_[b].adj;
            if (std::find(back.begin(), back.end(), a) == back.end()) return false;
        }
    }
    return true;
}

bool Board::validateUniqueCodesAndCoords() const {
    std::unordered_set<char> codes;
    struct PairHash {
        std::size_t operator()(const std::pair<int,int>& p) const noexcept {
            return std::hash<long long>{}((static_cast<long long>(p.first) << 32) ^ (unsigned)p.second);
        }
    };
    std::unordered_set<std::pair<int,int>, PairHash> coords;

    for (const auto& x : territories_) {
        if (!codes.insert(x.code).second) return false;
        if (!coords.insert({x.r, x.c}).second) return false;
    }
    return true;
}
