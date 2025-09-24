#include "Rules.h"

#include <algorithm>
#include <queue>
#include <random>
#include <unordered_set>

// -------- Ownership helpers --------

std::vector<TerrId> Rules::owned(const Board& b, PlayerId p) {
    std::vector<TerrId> out;
    out.reserve(b.count());
    for (int i = 0; i < b.count(); ++i) {
        if (b.at(i).owner == p) out.push_back(i);
    }
    return out;
}

std::vector<TerrId> Rules::borders(const Board& b, PlayerId p) {
    std::vector<TerrId> out;
    for (int i = 0; i < b.count(); ++i) {
        const auto& t = b.at(i);
        if (t.owner != p) continue;
        bool touchesEnemy = false;
        for (TerrId n : t.adj) {
            if (b.at(n).owner != p) { touchesEnemy = true; break; }
        }
        if (touchesEnemy) out.push_back(i);
    }
    return out;
}

// -------- Game state / victory --------

GameState Rules::gameStatus(const Board& b) {
    int p1 = 0, p2 = 0;
    for (int i = 0; i < b.count(); ++i) {
        auto o = b.at(i).owner;
        if (o == PlayerId::P1) ++p1;
        else if (o == PlayerId::P2) ++p2;
    }
    if (p1 > 0 && p2 == 0) return GameState::Player1Wins;
    if (p2 > 0 && p1 == 0) return GameState::Player2Wins;
    return GameState::Ongoing;
}

// -------- Reinforcements --------

int Rules::baseReinforcements(const Board& b, PlayerId p) {
    int n = static_cast<int>(owned(b, p).size());
    return std::max(3, n / 3);
}

bool Rules::chainOf5BonusTarget(const Board& b, PlayerId p, TerrId& tIdx) {
    const int n = b.count();
    std::vector<char> vis(n, 0);
    int bestSize = 0;
    TerrId bestPick = -1;

    for (int i = 0; i < n; ++i) {
        if (vis[i] || b.at(i).owner != p) continue;
        // BFS for component
        std::queue<int> q;
        std::vector<int> comp;
        q.push(i);
        vis[i] = 1;
        while (!q.empty()) {
            int u = q.front(); q.pop();
            comp.push_back(u);
            for (TerrId v : b.at(u).adj) {
                if (!vis[v] && b.at(v).owner == p) {
                    vis[v] = 1; q.push(v);
                }
            }
        }
        if (static_cast<int>(comp.size()) >= 5) {
            if (static_cast<int>(comp.size()) > bestSize) {
                bestSize = static_cast<int>(comp.size());
                bestPick = *std::min_element(comp.begin(), comp.end());
            }
        }
    }
    if (bestSize >= 5) {
        tIdx = bestPick;
        return true;
    }
    return false;
}

// -------- Legality checks --------

bool Rules::canAttack(const Board& b, TerrId from, TerrId to, PlayerId attacker) {
    if (from < 0 || to < 0 || from >= b.count() || to >= b.count()) return false;
    if (from == to) return false;
    const auto& A = b.at(from);
    const auto& D = b.at(to);
    if (A.owner != attacker) return false;
    if (D.owner == attacker || D.owner == PlayerId::None) return false; // must be enemy-held
    if (!b.areAdjacent(from, to)) return false;
    if (A.armies < 2) return false; // must leave at least 1 behind
    return true;
}

bool Rules::canFortify(const Board& b, TerrId from, TerrId to, PlayerId p) {
    if (from < 0 || to < 0 || from >= b.count() || to >= b.count()) return false;
    if (from == to) return false;
    const auto& A = b.at(from);
    const auto& B = b.at(to);
    if (A.owner != p || B.owner != p) return false;
    if (!b.areAdjacent(from, to)) return false; // single-hop fortify
    if (A.armies < 2) return false; // must leave 1 behind
    return true;
}

// NEW: path-based (multi-hop) fortify legality
bool Rules::canFortifyPath(const Board& b, TerrId from, TerrId to, PlayerId p) {
    if (from < 0 || to < 0 || from >= b.count() || to >= b.count()) return false;
    if (from == to) return false;
    if (b.at(from).owner != p || b.at(to).owner != p) return false;
    if (b.at(from).armies < 2) return false; // must leave at least 1 behind

    std::unordered_set<TerrId> vis;
    std::queue<TerrId> q;
    vis.insert(from);
    q.push(from);

    while (!q.empty()) {
        TerrId u = q.front(); q.pop();
        if (u == to) return true;
        for (TerrId v : b.neighbors(u)) {
            if (vis.count(v)) continue;
            if (b.at(v).owner != p) continue; // only traverse own territories
            vis.insert(v);
            q.push(v);
        }
    }
    return false;
}

// -------- Battle resolution (dice math only) --------

int Rules::attackerDice(int armiesAtFrom) {
    if (armiesAtFrom <= 1) return 0;
    return std::min(3, armiesAtFrom - 1);
}

int Rules::defenderDice(int armiesAtTo) {
    if (armiesAtTo <= 0) return 0;
    return std::min(2, armiesAtTo);
}

static void rollAndSort(std::mt19937& rng, int k, std::vector<int>& out) {
    std::uniform_int_distribution<int> d(1, 6);
    out.clear();
    out.reserve(k);
    for (int i = 0; i < k; ++i) out.push_back(d(rng));
    std::sort(out.begin(), out.end(), std::greater<int>());
}

Rules::BattleLosses Rules::simulateBattleOnce(int attDice, int defDice, unsigned seed) {
    std::mt19937 rng(seed);
    std::vector<int> A, D;
    rollAndSort(rng, attDice, A);
    rollAndSort(rng, defDice, D);
    const int pairs = std::min(attDice, defDice);
    BattleLosses losses{0,0};
    for (int i = 0; i < pairs; ++i) {
        if (A[i] > D[i]) ++losses.defender;
        else ++losses.attacker; // ties go to defender (attacker loses one)
    }
    return losses;
}

// -------- State updates --------

bool Rules::applyBattle(Board& b, TerrId from, TerrId to, PlayerId attacker, unsigned seed, BattleLosses* last) {
    if (!b.areAdjacent(from, to)) return false;
    auto& A = b.at(from);
    auto& D = b.at(to);
    if (A.owner != attacker) return false;
    if (D.owner == attacker) return false;
    int aDice = attackerDice(A.armies);
    int dDice = defenderDice(D.armies);
    if (aDice <= 0 || dDice <= 0) return false;

    auto losses = simulateBattleOnce(aDice, dDice, seed);
    if (last) *last = losses;

    // Apply losses
    A.armies -= losses.attacker;
    D.armies -= losses.defender;
    if (D.armies <= 0) {
        // Capture: set new owner, zero armies for now.
        // Caller MUST call moveAfterCapture to move >=1 from A to D (and leave >=1 in A).
        D.owner = attacker;
        D.armies = 0;
        return true;
    }
    return false;
}

void Rules::moveAfterCapture(Board& b, TerrId from, TerrId to, int armiesToMove) {
    auto& A = b.at(from);
    auto& T = b.at(to);
    if (armiesToMove <= 0) return;
    if (A.armies - armiesToMove < 1) armiesToMove = std::max(0, A.armies - 1);
    if (armiesToMove <= 0) return;
    A.armies -= armiesToMove;
    T.armies += armiesToMove;
}
