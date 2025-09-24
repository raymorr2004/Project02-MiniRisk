#include "Game.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <string>

#include "MapSpec.h"
#include "Rules.h"
#include "IO.h"
#include "RandomAI.h"

// --------- makeBoard ---------

Board Game::makeBoard() {
    std::random_device rd;
    return makeBoard(rd());
}

Board Game::makeBoard(std::uint32_t seed) {
    auto terrs = MapSpec::build20(seed);
    Board b{ std::move(terrs) };

    if (!b.validateAdjUndirected()) {
        std::cerr << "[Error] Map adjacency is not symmetric.\n";
    }
    if (!b.validateUniqueCodesAndCoords()) {
        std::cerr << "[Error] Map has duplicate codes or coordinates.\n";
    }
    return b;
}

// --------- printRules ---------

void Game::printRules() {
    std::cout
        << "=== Mini-RISK (Text) ===\n"
        << "Goal: Control all territories.\n"
        << "Turn structure:\n"
        << "  1) Reinforcements: gain max(3, owned/3). If you own a connected chain\n"
        << "     of 5+ territories (by adjacency), gain a +5 bonus in one territory of that chain.\n"
        << "  2) Attack: from a territory with >=2 armies into an adjacent enemy.\n"
        << "     Attacker rolls up to 3 dice (must leave 1 behind); defender up to 2.\n"
        << "     Compare highest-to-highest; ties favor defender.\n"
        << "     On capture, move at least 1 army into the new territory.\n"
        << "  3) Fortify (once): move armies between two adjacent owned territories,\n"
        << "     leaving at least 1 behind.\n"
        << "Win: The other player controls 0 territories.\n\n";
}

// --------- setupStartingPositions ---------

void Game::setupStartingPositions(Board& b, std::uint32_t seed) {
    std::mt19937 rng(seed);

    const int n = b.count();
    std::vector<int> idx(n);
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), rng);

    for (int k = 0; k < n; ++k) {
        auto& T = b.at(idx[k]);
        T.owner  = (k < n/2) ? PlayerId::P1 : PlayerId::P2;
        T.armies = 1;
    }
}

// --------- play (full loop, color board) ---------

GameState Game::play(Board& b, bool cpuAsP2, std::uint32_t seed) {
    using std::mt19937;
    using std::uniform_int_distribution;
    using std::to_string;

    mt19937 rng(seed);

    auto current = PlayerId::P1;
    auto nextPlayer = [](PlayerId p) { return (p == PlayerId::P1 ? PlayerId::P2 : PlayerId::P1); };

    // ---- helpers to detect if any legal moves exist ----
    auto anyLegalAttack = [&](PlayerId p) -> bool {
        auto border = Rules::borders(b, p);
        for (TerrId from : border) {
            if (b.at(from).armies < 2) continue; // must leave 1 behind
            for (TerrId to : b.neighbors(from)) {
                if (Rules::canAttack(b, from, to, p)) return true;
            }
        }
        return false;
    };

    auto anyLegalFortify = [&](PlayerId p) -> bool {
        auto ownedList = Rules::owned(b, p);
        for (TerrId from : ownedList) {
            if (b.at(from).armies < 2) continue;
            for (TerrId to : b.neighbors(from)) {
                if (Rules::canFortify(b, from, to, p)) return true;
            }
        }
        return false;
    };

    // ---- loop safety caps ----
    const int kMaxTurns = 500;      // absolute game cap
    const int kMaxStaleTurns = 60;  // consecutive turns with no captures -> draw
    const int kCpuMaxAttacks = 6;   // CPU won't endlessly attack

    int turnCount = 0;
    int staleTurns = 0;

    auto status = Rules::gameStatus(b);
    while (status == GameState::Ongoing) {
        if (++turnCount > kMaxTurns) { status = GameState::Draw; break; }

        bool capturedThisTurn = false;

        // --- Turn header ---
        IO::println(current == PlayerId::P1 ? "\n-- Player 1 turn --" : "\n-- Player 2 turn --");
        IO::printBoardColor(b, 3);

        // =========================
        // 1) REINFORCEMENTS
        // =========================
        int base = Rules::baseReinforcements(b, current);

        TerrId bonusT = -1;
        if (Rules::chainOf5BonusTarget(b, current, bonusT)) {
            b.at(bonusT).armies += 5;
            IO::println((current == PlayerId::P1 ? "P1" : "P2") + std::string(" chain bonus: +5 to ") + b.at(bonusT).name);
        }

        if (current == PlayerId::P1 || !cpuAsP2) {
            IO::println("Reinforcements: " + to_string(base));
            TerrId where = IO::readOwnedTerritory(b, current, "Place ALL reinforcements at (code): ");
            b.at(where).armies += base;
        } else {
            TerrId where = RandomAI::chooseReinforcement(b, current, base, seed++);
            if (where >= 0) b.at(where).armies += base;
        }

        IO::printBoardColor(b, 3);

        // Early win check
        status = Rules::gameStatus(b);
        if (status != GameState::Ongoing) break;

        // =========================
        // 2) ATTACK PHASE
        // =========================
        if (current == PlayerId::P1 || !cpuAsP2) {
            if (!anyLegalAttack(current)) {
                IO::println("No legal attacks. Skipping attack phase.");
            } else {
                while (true) {
                    // Stop asking if no legal moves remain
                    if (!anyLegalAttack(current)) {
                        IO::println("No more legal attacks.");
                        break;
                    }
                    char doAttack = IO::readYesNo("Attack?");
                    if (doAttack != 'y') break;

                    auto choice = IO::readAttackChoice(b, current);
                    Rules::BattleLosses last{};
                    bool captured = Rules::applyBattle(b, choice.from, choice.to, current, seed++, &last);

                    IO::println("Battle: attacker -" + to_string(last.attacker) +
                                ", defender -" + to_string(last.defender));
                    IO::printBoardColor(b, 3);

                    if (captured) {
                        capturedThisTurn = true;
                        IO::println("Captured " + b.at(choice.to).name + "!");
                        int maxMove = std::max(1, b.at(choice.from).armies - 1);
                        int amt = IO::readIntInRange("Move how many armies into the captured territory?", 1, maxMove);
                        Rules::moveAfterCapture(b, choice.from, choice.to, amt);
                        IO::printBoardColor(b, 3);
                    }

                    status = Rules::gameStatus(b);
                    if (status != GameState::Ongoing) break;
                }
            }
        } else {
            int attacks = 0;
            while (attacks < kCpuMaxAttacks) {
                auto plan = RandomAI::chooseAttack(b, current, seed++);
                if (!plan.valid) break;

                Rules::BattleLosses last{};
                bool captured = Rules::applyBattle(b, plan.from, plan.to, current, seed++, &last);
                if (captured) {
                    capturedThisTurn = true;
                    int maxMove = std::max(1, b.at(plan.from).armies - 1);
                    if (maxMove > 0) {
                        uniform_int_distribution<int> mv(1, maxMove);
                        Rules::moveAfterCapture(b, plan.from, plan.to, mv(rng));
                    }
                }
                ++attacks;

                status = Rules::gameStatus(b);
                if (status != GameState::Ongoing) break;
            }
            IO::printBoardColor(b, 3);
        }

        if (status != GameState::Ongoing) break;

        // =========================
        // 3) FORTIFY (once, optional)
        // =========================
        if (current == PlayerId::P1 || !cpuAsP2) {
            if (!anyLegalFortify(current)) {
                IO::println("No legal fortify moves. Skipping.");
            } else if (IO::readYesNo("Fortify?") == 'y') {
                auto f = IO::readFortifyChoice(b, current);
                Rules::moveAfterCapture(b, f.from, f.to, f.amount);
            }
        } else {
            auto plan = RandomAI::chooseFortify(b, current, seed++);
            if (plan.valid) {
                Rules::moveAfterCapture(b, plan.from, plan.to, plan.amount);
            }
        }

        IO::printBoardColor(b, 3);

        // End-of-turn win check
        status = Rules::gameStatus(b);
        if (status != GameState::Ongoing) break;

        // Stalemate detection
        if (capturedThisTurn) staleTurns = 0;
        else if (++staleTurns >= kMaxStaleTurns) { status = GameState::Draw; break; }

        // Next player's turn
        current = nextPlayer(current);
    }

    IO::println("\n=== Final Board ===");
    IO::printBoardColor(b, 3);
    return status;
}
