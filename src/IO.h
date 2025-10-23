#pragma once
#include <string>
#include "Board.h"
#include "Types.h"

namespace IO {

    // ----- Display -----
    // Prints the ASCII version of the board
    void printBoard(const Board& b, bool showOwner = true, bool showArmies = true, int cellWidth = 4);

    // Prints colorized board (top row = code, bottom row = armies)
    void printBoardColor(const Board& b, int cellWidth = 3);

    // Prints a message with newline
    void println(const std::string& s);

    // ----- Simple inputs -----
    char readYesNo(const std::string& prompt);
    int  readIntInRange(const std::string& prompt, int min, int max);

    // ----- Territory selection -----
    TerrId readTerritoryByCode(const Board& b, const std::string& prompt);
    TerrId readOwnedTerritory(const Board& b, PlayerId p, const std::string& prompt);

    // ----- Structured choices -----
    struct AttackChoice {
        TerrId from{-1};
        TerrId to{-1};
    };
    AttackChoice readAttackChoice(const Board& b, PlayerId attacker,
                                  const std::string& fromPrompt = "Attack FROM (code): ",
                                  const std::string& toPrompt   = "Attack TO (adjacent enemy code): ");

    struct FortifyChoice {
        TerrId from{-1};
        TerrId to{-1};
        int amount{0};
    };
    FortifyChoice readFortifyChoice(const Board& b, PlayerId p,
                                    const std::string& fromPrompt = "Fortify FROM (code): ",
                                    const std::string& toPrompt   = "Fortify TO (adjacent owned code): ",
                                    const std::string& amtPrompt  = "How many armies to move: ");

} // namespace IO
