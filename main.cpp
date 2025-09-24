#include <iostream>
#include <random>

// Note the "src/" prefixes since main.cpp is not inside the src folder
#include "src/Game.h"
#include "src/IO.h"

int main() {
    // Print rules/controls up front
    Game::printRules();

    // Simple “play again” loop
    for (;;) {
        // Fresh random seed each game (you can replace with a fixed number for reproducible debugging)
        std::random_device rd;
        std::uint32_t seed = rd();

        // Build board and set up starting positions (10 territories each, 1 army each)
        auto board = Game::makeBoard(seed);
        Game::setupStartingPositions(board, seed);

        // Player 2 as CPU by default
        bool cpuAsP2 = true;

        // Run one full game (currently prints initial map and status; you’ll flesh out turn flow next)
        GameState result = Game::play(board, cpuAsP2, seed);

        // Quick end message (optional)
        switch (result) {
            case GameState::Player1Wins: IO::println("Result: Player 1 wins!"); break;
            case GameState::Player2Wins: IO::println("Result: Player 2 wins!"); break;
            case GameState::Draw:        IO::println("Result: Draw."); break;
            case GameState::Ongoing:
            default:                     IO::println("Game ended (stub)."); break;
        }

        // Ask to play again
        char again = IO::readYesNo("Play again?");
        if (again != 'y') break;
        IO::println(""); // spacer line
    }

    IO::println("Thanks for playing!");
    return 0;
}
