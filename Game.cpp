#include "Game.h"
#include "src/Rules.h"
#include "src/IO.h"
#include "src/MapSpec.h"
#include "src/RandomAI.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>

// ---------- Constructors ----------
Game::Game() : Game(std::random_device{}()) {}
Game::Game(uint32_t seed) : seed_(seed), rng_(seed) {
    board_ = makeBoard(seed_);
}

// ---------- Accessors ----------
Board& Game::board() { return board_; }
const Board& Game::board() const { return board_; }

// ---------- Setup ----------
void Game::resetBoard(uint32_t seed) {
    seed_ = seed;
    rng_.seed(seed_);
    board_ = makeBoard(seed_);
}

void Game::setupStartingPositions(uint32_t seed) {
    if (!seed) seed = seed_;  // default to current seed
    std::mt19937 localRng(seed);
    int n = board_.count();

    std::vector<int> ids(n);
    std::iota(ids.begin(), ids.end(), 0);
    std::shuffle(ids.begin(), ids.end(), localRng);

    for (int k = 0; k < n; ++k) {
        auto& T = board_.at(ids[k]);
        T.owner  = (k < n / 2) ? PlayerId::P1 : PlayerId::P2;
        T.armies = 1;
    }
}

// ---------- Board creation ----------
Board Game::makeBoard(uint32_t seed) {
    auto terrs = MapSpec::build20(seed);
    Board b(std::move(terrs));

    if (!b.validateAdjUndirected())
        std::cerr << "[Error] Map adjacency is not symmetric.\n";
    if (!b.validateUniqueCodesAndCoords())
        std::cerr << "[Error] Map has duplicate codes or coordinates.\n";
    return b;
}

// ---------- Rules print ----------
void Game::printRules() const {
    std::cout
        << "=== Mini-RISK (Text) ===\n"
        << "Goal: Control all territories.\n"
        << "Turn structure:\n"
        << "  1) Reinforcements: gain max(3, owned/3). "
        << "Chains of 5+ give +5 bonus.\n"
        << "  2) Attack: from ≥2 armies into adjacent enemy. "
        << "Dice compare; ties defend.\n"
        << "  3) Fortify once per turn between adjacent owned territories.\n"
        << "Win: Opponent controls 0 territories.\n\n";
}

// ---------- Helpers ----------
bool Game::anyLegalAttack(PlayerId p) const {
    auto borders = Rules::borders(board_, p);
    for (TerrId from : borders) {
        if (board_.at(from).armies < 2) continue;
        for (TerrId to : board_.neighbors(from))
            if (Rules::canAttack(board_, from, to, p)) return true;
    }
    return false;
}

bool Game::anyLegalFortify(PlayerId p) const {
    auto owned = Rules::owned(board_, p);
    for (TerrId from : owned) {
        if (board_.at(from).armies < 2) continue;
        for (TerrId to : board_.neighbors(from))
            if (Rules::canFortify(board_, from, to, p)) return true;
    }
    return false;
}

// ---------- Main game loop ----------
GameState Game::play(bool cpuAsP2) {
    using std::to_string;
    PlayerId current = PlayerId::P1;
    auto nextPlayer = [](PlayerId p){ return p == PlayerId::P1 ? PlayerId::P2 : PlayerId::P1; };

    constexpr int MAX_TURNS = 500;
    constexpr int MAX_STALE = 60;
    constexpr int CPU_MAX_ATTACKS = 6;

    int turns = 0;
    int stale = 0;
    GameState status = Rules::gameStatus(board_);

    while (status == GameState::Ongoing) {
        if (++turns > MAX_TURNS) { status = GameState::Draw; break; }

        bool captured = false;
        IO::println(current == PlayerId::P1 ? "\n-- Player 1 turn --" : "\n-- Player 2 turn --");
        IO::printBoardColor(board_, 3);

        // ---------- Reinforcement phase ----------
        int base = Rules::baseReinforcements(board_, current);
        TerrId bonusT = -1;
        if (Rules::chainOf5BonusTarget(board_, current, bonusT)) {
            board_.at(bonusT).armies += 5;
            IO::println((current == PlayerId::P1 ? "P1" : "P2") +
                        std::string(" chain bonus: +5 to ") + board_.at(bonusT).name);
        }

        if (current == PlayerId::P1 || !cpuAsP2) {
            IO::println("Reinforcements: " + to_string(base));
            TerrId where = IO::readOwnedTerritory(board_, current, "Place ALL reinforcements at (code): ");
            board_.at(where).armies += base;
        } else {
            TerrId where = RandomAI::chooseReinforcement(board_, current, base, seed_++);
            if (where >= 0) board_.at(where).armies += base;
        }

        IO::printBoardColor(board_, 3);
        status = Rules::gameStatus(board_);
        if (status != GameState::Ongoing) break;

        // ---------- Attack phase ----------
        if (current == PlayerId::P1 || !cpuAsP2) {
            if (!anyLegalAttack(current)) {
                IO::println("No legal attacks. Skipping.");
            } else {
                while (true) {
                    if (!anyLegalAttack(current)) { IO::println("No more attacks."); break; }
                    if (IO::readYesNo("Attack?") != 'y') break;

                    auto choice = IO::readAttackChoice(board_, current);
                    Rules::BattleLosses loss{};
                    bool took = Rules::applyBattle(board_, choice.from, choice.to, current, seed_++, &loss);

                    IO::println("Battle: attacker -" + to_string(loss.attacker) +
                                ", defender -" + to_string(loss.defender));
                    IO::printBoardColor(board_, 3);

                    if (took) {
                        captured = true;
                        IO::println("Captured " + board_.at(choice.to).name + "!");
                        int maxMove = std::max(1, board_.at(choice.from).armies - 1);
                        int amt = IO::readIntInRange("Move how many armies?", 1, maxMove);
                        Rules::moveAfterCapture(board_, choice.from, choice.to, amt);
                        IO::printBoardColor(board_, 3);
                    }

                    status = Rules::gameStatus(board_);
                    if (status != GameState::Ongoing) break;
                }
            }
        } else {
            int attacks = 0;
            while (attacks < CPU_MAX_ATTACKS) {
                auto plan = RandomAI::chooseAttack(board_, current, seed_++);
                if (!plan.valid) break;

                Rules::BattleLosses loss{};
                bool took = Rules::applyBattle(board_, plan.from, plan.to, current, seed_++, &loss);
                if (took) {
                    captured = true;
                    int maxMove = std::max(1, board_.at(plan.from).armies - 1);
                    if (maxMove > 0) {
                        std::uniform_int_distribution<int> mv(1, maxMove);
                        Rules::moveAfterCapture(board_, plan.from, plan.to, mv(rng_));
                    }
                }
                ++attacks;
                status = Rules::gameStatus(board_);
                if (status != GameState::Ongoing) break;
            }
            IO::printBoardColor(board_, 3);
        }

        if (status != GameState::Ongoing) break;

        // ---------- Fortify phase ----------
        if (current == PlayerId::P1 || !cpuAsP2) {
            if (!anyLegalFortify(current)) IO::println("No legal fortify moves.");
            else if (IO::readYesNo("Fortify?") == 'y') {
                auto f = IO::readFortifyChoice(board_, current);
                Rules::moveAfterCapture(board_, f.from, f.to, f.amount);
            }
        } else {
            auto plan = RandomAI::chooseFortify(board_, current, seed_++);
            if (plan.valid) Rules::moveAfterCapture(board_, plan.from, plan.to, plan.amount);
        }

        IO::printBoardColor(board_, 3);

        // ---------- End of turn ----------
        status = Rules::gameStatus(board_);
        if (status != GameState::Ongoing) break;

        stale = captured ? 0 : stale + 1;
        if (stale >= MAX_STALE) { status = GameState::Draw; break; }

        current = nextPlayer(current);
    }

    IO::println("\n=== Final Board ===");
    IO::printBoardColor(board_, 3);
    return status;
}
