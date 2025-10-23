#include "Rules.h"
#include <algorithm>
#include <queue>
#include <random>
#include <unordered_set>

// ---------- Ownership ----------
std::vector<TerrId> Rules::owned(const Board& b, PlayerId p) {
    std::vector<TerrId> out;
    out.reserve(b.count());
    for (int i = 0; i < b.count(); ++i)
        if (b.at(i).owner == p) out.push_back(i);
    return out;
}

std::vector<TerrId> Rules::borders(const Board& b, PlayerId p) {
    std::vector<TerrId> out;
    for (int i = 0; i < b.count(); ++i) {
        const auto& t = b.at(i);
        if (t.owner != p) continue;
        for (TerrId n : t.adj)
            if (b.at(n).owner != p) { out.push_back(i); break; }
    }
    return out;
}

// ---------- Game state ----------
GameState Rules::gameStatus(const Board& b) {
    int p1 = 0, p2 = 0;
    for (int i = 0; i < b.count(); ++i) {
        switch (b.at(i).owner) {
            case PlayerId::P1: ++p1; break;
            case PlayerId::P2: ++p2; break;
            default: break;
        }
    }
    if (p1 && !p2) return GameState::Player1Wins;
    if (p2 && !p1) return GameState::Player2Wins;
    return GameState::Ongoing;
}

// ---------- Reinforcements ----------
int Rules::baseReinforcements(const Board& b, PlayerId p) {
    int n = static_cast<int>(owned(b, p).size());
    return std::max(3, n / 3);
}

bool Rules::chainOf5BonusTarget(const Board& b, PlayerId p, TerrId& tIdx) {
    const int n = b.count();
    std::vector<char> vis(n, 0);
    int bestSize = 0; TerrId bestPick = -1;

    for (int i = 0; i < n; ++i) {
        if (vis[i] || b.at(i).owner != p) continue;
        std::queue<int> q; std::vector<int> comp;
        q.push(i); vis[i] = 1;
        while (!q.empty()) {
            int u = q.front(); q.pop();
            comp.push_back(u);
            for (TerrId v : b.at(u).adj)
                if (!vis[v] && b.at(v).owner == p)
                    { vis[v] = 1; q.push(v); }
        }
        if (static_cast<int>(comp.size()) >= 5 &&
            static_cast<int>(comp.size()) > bestSize) {
            bestSize = static_cast<int>(comp.size());
            bestPick = *std::min_element(comp.begin(), comp.end());
        }
    }
    if (bestSize >= 5) { tIdx = bestPick; return true; }
    return false;
}

// ---------- Legality ----------
bool Rules::canAttack(const Board& b, TerrId from, TerrId to, PlayerId attacker) {
    if (from < 0 || to < 0 || from >= b.count() || to >= b.count() || from == to)
        return false;
    const auto& A = b.at(from);
    const auto& D = b.at(to);
    if (A.owner != attacker || D.owner == attacker || D.owner == PlayerId::None)
        return false;
    if (!b.areAdjacent(from, to) || A.armies < 2) return false;
    return true;
}

bool Rules::canFortify(const Board& b, TerrId from, TerrId to, PlayerId p) {
    if (from < 0 || to < 0 || from >= b.count() || to >= b.count() || from == to)
        return false;
    const auto& A = b.at(from);
    const auto& B = b.at(to);
    return (A.owner == p && B.owner == p &&
            b.areAdjacent(from, to) && A.armies >= 2);
}

bool Rules::canFortifyPath(const Board& b, TerrId from, TerrId to, PlayerId p) {
    if (from < 0 || to < 0 || from >= b.count() || to >= b.count() || from == to)
        return false;
    if (b.at(from).owner != p || b.at(to).owner != p || b.at(from).armies < 2)
        return false;

    std::unordered_set<TerrId> vis{from};
    std::queue<TerrId> q; q.push(from);
    while (!q.empty()) {
        TerrId u = q.front(); q.pop();
        if (u == to) return true;
        for (TerrId v : b.neighbors(u))
            if (!vis.count(v) && b.at(v).owner == p)
                { vis.insert(v); q.push(v); }
    }
    return false;
}

// ---------- Battle mechanics ----------
int Rules::attackerDice(int armiesAtFrom) {
    return (armiesAtFrom > 1) ? std::min(3, armiesAtFrom - 1) : 0;
}

int Rules::defenderDice(int armiesAtTo) {
    return (armiesAtTo > 0) ? std::min(2, armiesAtTo) : 0;
}

static void rollAndSort(std::mt19937& rng, int k, std::vector<int>& out) {
    std::uniform_int_distribution<int> d(1, 6);
    out.clear(); out.reserve(k);
    for (int i = 0; i < k; ++i) out.push_back(d(rng));
    std::sort(out.begin(), out.end(), std::greater<int>());
}

Rules::BattleLosses Rules::simulateBattleOnce(int attDice, int defDice, unsigned seed) {
    std::mt19937 rng(seed);
    std::vector<int> A, D;
    rollAndSort(rng, attDice, A);
    rollAndSort(rng, defDice, D);

    BattleLosses losses{0,0};
    for (int i = 0; i < std::min(attDice, defDice); ++i)
        (A[i] > D[i]) ? ++losses.defender : ++losses.attacker;
    return losses;
}

// ---------- State updates ----------
bool Rules::applyBattle(Board& b, TerrId from, TerrId to, PlayerId attacker,
                        unsigned seed, BattleLosses* last) {
    if (!b.areAdjacent(from, to)) return false;
    auto& A = b.at(from);
    auto& D = b.at(to);
    if (A.owner != attacker || D.owner == attacker) return false;

    int aDice = attackerDice(A.armies);
    int dDice = defenderDice(D.armies);
    if (aDice <= 0 || dDice <= 0) return false;

    auto losses = simulateBattleOnce(aDice, dDice, seed);
    if (last) *last = losses;

    A.armies -= losses.attacker;
    D.armies -= losses.defender;

    if (D.armies <= 0) {
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
    if (A.armies - armiesToMove < 1)
        armiesToMove = std::max(0, A.armies - 1);
    if (armiesToMove <= 0) return;
    A.armies -= armiesToMove;
    T.armies += armiesToMove;
}
