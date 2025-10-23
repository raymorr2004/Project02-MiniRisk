#include "IO.h"
#include "Rules.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>

// ---------- Local helpers ----------
namespace {
    std::string readLine() {
        std::string s;
        std::getline(std::cin, s);
        return s;
    }

    std::string trim(const std::string& s) {
        size_t start = 0, end = s.size();
        while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
        return s.substr(start, end - start);
    }

    char toUpper(char c) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    // Finds territory index by letter (case-insensitive)
    TerrId findByCode(const Board& b, char code) {
        char up = toUpper(code);
        for (int i = 0; i < b.count(); ++i)
            if (toUpper(b.at(i).code) == up)
                return i;
        return -1;
    }
} // namespace

// ---------- Display ----------
void IO::printBoard(const Board& b, bool showOwner, bool showArmies, int cellWidth) {
    std::cout << b.render(showOwner, showArmies, cellWidth) << std::flush;
}

void IO::printBoardColor(const Board& b, int cellWidth) {
    std::cout << b.renderColor(cellWidth) << std::flush;
}

void IO::println(const std::string& s) {
    std::cout << s << '\n';
}

// ---------- Simple inputs ----------
char IO::readYesNo(const std::string& prompt) {
    while (true) {
        std::cout << prompt << " (y/n): " << std::flush;
        std::string line = trim(readLine());
        if (line.empty()) continue;

        char c = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
        if (c == 'y' || c == 'n') return c;
        std::cout << "Please enter 'y' or 'n'.\n";
    }
}

int IO::readIntInRange(const std::string& prompt, int min, int max) {
    while (true) {
        std::cout << prompt << " [" << min << "-" << max << "]: " << std::flush;
        std::string line = trim(readLine());
        if (line.empty()) {
            std::cout << "Please enter a number.\n";
            continue;
        }

        bool ok = true;
        int sign = 1;
        size_t i = 0;
        if (line[0] == '+' || line[0] == '-') {
            sign = (line[0] == '-') ? -1 : 1;
            i = 1;
        }

        long long val = 0;
        for (; i < line.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(line[i]))) { ok = false; break; }
            val = val * 10 + (line[i] - '0');
            if (val * sign < std::numeric_limits<int>::min() ||
                val * sign > std::numeric_limits<int>::max()) {
                ok = false; break;
            }
        }

        if (!ok) { std::cout << "Invalid number. Try again.\n"; continue; }
        int x = static_cast<int>(val * sign);
        if (x < min || x > max) { std::cout << "Out of range. Try again.\n"; continue; }
        return x;
    }
}

// ---------- Territory selection ----------
TerrId IO::readTerritoryByCode(const Board& b, const std::string& prompt) {
    while (true) {
        std::cout << prompt << std::flush;
        std::string line = trim(readLine());
        if (line.empty()) {
            std::cout << "Enter a territory code (e.g., A, B, C...)\n";
            continue;
        }

        TerrId id = findByCode(b, line[0]);
        if (id >= 0) return id;

        std::cout << "No such territory code. Try again.\n";
    }
}

TerrId IO::readOwnedTerritory(const Board& b, PlayerId p, const std::string& prompt) {
    while (true) {
        TerrId id = readTerritoryByCode(b, prompt);
        if (b.at(id).owner == p) return id;
        std::cout << "You do not own that territory. Try again.\n";
    }
}

// ---------- Structured choices ----------
IO::AttackChoice IO::readAttackChoice(const Board& b, PlayerId attacker,
                                      const std::string& fromPrompt,
                                      const std::string& toPrompt) {
    while (true) {
        TerrId from = readOwnedTerritory(b, attacker, fromPrompt);
        if (b.at(from).armies < 2) {
            std::cout << "You must have at least 2 armies to attack.\n";
            continue;
        }

        TerrId to = readTerritoryByCode(b, toPrompt);
        if (!b.areAdjacent(from, to)) {
            std::cout << "Those territories are not adjacent.\n";
            continue;
        }

        if (b.at(to).owner == attacker || b.at(to).owner == PlayerId::None) {
            std::cout << "You can only attack enemy-held territories.\n";
            continue;
        }

        if (!Rules::canAttack(b, from, to, attacker)) {
            std::cout << "That attack is not legal. Try again.\n";
            continue;
        }

        return {from, to};
    }
}

IO::FortifyChoice IO::readFortifyChoice(const Board& b, PlayerId p,
                                        const std::string& fromPrompt,
                                        const std::string& toPrompt,
                                        const std::string& amtPrompt) {
    while (true) {
        TerrId from = readOwnedTerritory(b, p, fromPrompt);
        TerrId to = readOwnedTerritory(b, p, toPrompt);

        if (!Rules::canFortifyPath(b, from, to, p)) {
            std::cout << "Invalid fortify path. Must be connected through your owned territories.\n";
            continue;
        }

        int maxMove = std::max(0, b.at(from).armies - 1);
        if (maxMove <= 0) {
            std::cout << "Not enough armies to move (must leave 1 behind).\n";
            continue;
        }

        int amt = readIntInRange(amtPrompt, 1, maxMove);
        return {from, to, amt};
    }
}
