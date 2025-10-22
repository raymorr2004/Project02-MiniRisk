#pragma once
#include <vector>
#include <string>
#include <random>
#include <iostream>
#include <utility>

enum Status { ONGOING, PLAYER_1_WINS, PLAYER_2_WINS, DRAW };
enum Player { NONE = 0, P1 = 1, P2 = 2 };
using TerrId = int;

struct Territory {
    char code{'?'};
    std::string name;
    int r{0}, c{0};              // grid position
    Player owner{NONE};
    int armies{0};
    std::vector<TerrId> adj;     // neighbor indices
};

class Game {
public:
    Game(unsigned seed = std::random_device{}());

    // High-level API (no I/O inside these)
    Status status() const;
    Player currentPlayer() const { return current_; }

    // Setup/start phases
    void dealStartingPositions();                   // randomly assign owners, set armies=1
    int  baseReinforcements(Player p) const;        // max(3, floor(owned/3))
    bool chainBonusTarget(Player p, TerrId& tIdx) const; // connected component >=5 → pick any one

    // Mutations (no printing)
    void addArmies(TerrId t, int amount);           // reinforcement placement (all to one territory)
    bool canAttack(TerrId from, TerrId to, Player attacker) const;
    bool applyBattle(TerrId from, TerrId to, Player attacker, unsigned seed,
                     int& outAttackerLoss, int& outDefenderLoss, bool& outCaptured);
    void moveAfterCapture(TerrId from, TerrId to, int armiesToMove);
    bool canFortifyPath(TerrId from, TerrId to, Player p) const;
    void fortify(TerrId from, TerrId to, int amount); // assumes legality & leave >=1

    // Turn/bookkeeping (no I/O)
    void nextPlayer();
    void incrementTurnCount(bool capturedThisTurn);
    int  turnCount() const { return turnCount_; }

    // Helpers for input mapping in main.cpp
    TerrId findByCode(char code) const;
    bool   areAdjacent(TerrId a, TerrId b) const;
    const std::vector<TerrId>& neighbors(TerrId id) const { return board_[id].adj; }
    const Territory& at(TerrId i) const { return board_[i]; }
    Territory& at(TerrId i) { return board_[i]; }
    int  territoryCount() const { return static_cast<int>(board_.size()); }
    std::vector<TerrId> owned(Player p) const;
    std::vector<TerrId> borders(Player p) const;

    // Colored board rendering (returns string; printing occurs outside the class)
    std::string renderColor(int cellWidth = 3) const;

    // AI (kept internal; called from main)
    bool aiChooseReinforcement(TerrId& where, int reinforcements, unsigned seed) const;
    struct AIPlanAtt { bool valid{false}; TerrId from{-1}; TerrId to{-1}; };
    AIPlanAtt aiChooseAttack(unsigned seed) const;
    struct AIPlanFort { bool valid{false}; TerrId from{-1}; TerrId to{-1}; int amount{0}; };
    AIPlanFort aiChooseFortify(unsigned seed) const;

    friend std::ostream& operator<<(std::ostream& os, const Game& g) {
        return os << g.renderColor(3);
    }

private:
    // Map construction
    void build20(unsigned seed);

    // Dice helpers
    static int attackerDice(int armiesAtFrom);
    static int defenderDice(int armiesAtTo);
    static std::pair<int,int> simulateBattleOnce(int attDice, int defDice, unsigned seed);

    // Connectivity for bonus & fortify
    int componentSizeFrom(TerrId start, Player p) const;
    bool reachableOwned(TerrId from, TerrId to, Player p) const;

    // AI estimators
    double estimateCaptureProb(TerrId from, TerrId to, int trials, unsigned seed) const;

private:
    std::vector<Territory> board_;
    Player current_{P1};
    int turnCount_{0};
    int staleTurns_{0};
    int maxTurnsCap_{500};
    int staleCap_{60};
};
