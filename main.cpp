#include <iostream>
#include <random>
#include "Game.h"
#include "src/IO.h"

int main() {
    // Create a Game object — this will manage its own board and RNG
    Game game;

    // Print the rules (non-static call)
    game.printRules();

    // Play-again loop
    for (;;) {
        std::random_device rd;
        std::uint32_t seed = rd();

        // Reset the game board for a new match
        game.resetBoard(seed);
        game.setupStartingPositions();

        bool cpuAsP2 = true;

        GameState result = game.play(cpuAsP2);

        switch (result) {
            case GameState::Player1Wins: IO::println("Result: Player 1 wins!"); break;
            case GameState::Player2Wins: IO::println("Result: Player 2 wins!"); break;
            case GameState::Draw:        IO::println("Result: Draw."); break;
            default:                     IO::println("Game ended (incomplete)."); break;
        }

        char again = IO::readYesNo("Play again?");
        if (again != 'y') break;
        IO::println("");
    }

    IO::println("Thanks for playing!");
    return 0;
}
