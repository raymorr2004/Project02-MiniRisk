#include "RandomAI.h"
#include "Rules.h"
#include "Board.h"
#include <algorithm>
#include <random>
#include <vector>

namespace RandomAI {

// ---------- Reinforcement ----------
TerrId chooseReinforcement(const Board& b, PlayerId p,
                           int /*reinforcements*/, std::uint32_t seed) {
    auto ownedList = Rules::owned(b, p);
    if (ownedList.empty()) return -1;

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, static_cast<int>(ownedList.size()) - 1);
    return ownedList[dist(rng)];
}

// ---------- Helpers ----------
static bool simulateFullBattleOnce(int atkStart, int defStart, unsigned seed) {
    std::mt19937 rng(seed);
    int a = atkStart, d = defStart;
    unsigned localSeed = seed;

    while (a > 1 && d > 0) {
        int aDice = Rules::attackerDice(a);
        int dDice = Rules::defenderDice(d);
        if (aDice <= 0 || dDice <= 0) break;

        auto losses = Rules::simulateBattleOnce(aDice, dDice, localSeed++);
        a -= losses.attacker;
        d -= losses.defender;

        if (a < 0) a = 0;
        if (d < 0) d = 0;
    }
    return (d <= 0 && a > 0);
}

static double estimateCaptureProb(const Board& b, TerrId from, TerrId to,
                                  int trials, std::uint32_t baseSeed) {
    if (from < 0 || to < 0) return 0.0;

    int atk = b.at(from).armies;
    int def = b.at(to).armies;
    if (atk < 2) return 0.0;
    if (def <= 0) return 1.0;

    int wins = 0;
    for (int t = 0; t < trials; ++t) {
        unsigned s = baseSeed + static_cast<unsigned>(t * 7919 + from * 97 + to * 131);
        if (simulateFullBattleOnce(atk, def, s)) ++wins;
    }
    return static_cast<double>(wins) / static_cast<double>(trials);
}

// ---------- Attack ----------
AttackPlan chooseAttack(const Board& b, PlayerId p, std::uint32_t seed) {
    AttackPlan plan;

    auto borders = Rules::borders(b, p);
    if (borders.empty()) return plan;

    struct Pair { TerrId f; TerrId t; };
    std::vector<Pair> legalPairs;

    for (TerrId from : borders) {
        if (b.at(from).armies < 2) continue;
        for (TerrId to : b.neighbors(from))
            if (Rules::canAttack(b, from, to, p))
                legalPairs.push_back({from, to});
    }

    if (legalPairs.empty()) return plan;

    const int trials = 80;
    const double minAccept = 0.40;

    double bestScore = -1.0;
    Pair best{-1, -1};

    for (size_t i = 0; i < legalPairs.size(); ++i) {
        const auto& pr = legalPairs[i];
        double prob = estimateCaptureProb(b, pr.f, pr.t, trials, seed + static_cast<unsigned>(i * 31));
        int diff = b.at(pr.f).armies - b.at(pr.t).armies;
        double score = prob + 0.001 * diff;
        if (score > bestScore) { bestScore = score; best = pr; }
    }

    if (bestScore < (minAccept - 0.001 * 1000)) return plan;

    plan.from = best.f;
    plan.to = best.t;
    plan.valid = true;
    return plan;
}

// ---------- Fortify ----------
FortifyPlan chooseFortify(const Board& b, PlayerId p, std::uint32_t seed) {
    FortifyPlan plan;

    auto ownedList = Rules::owned(b, p);
    if (ownedList.empty()) return plan;

    std::mt19937 rng(seed);
    std::vector<std::tuple<TerrId, TerrId, int>> opts;

    for (TerrId from : ownedList) {
        if (b.at(from).armies < 2) continue;
        for (TerrId to : b.neighbors(from)) {
            if (Rules::canFortify(b, from, to, p)) {
                int maxMove = b.at(from).armies - 1;
                if (maxMove > 0) {
                    std::uniform_int_distribution<int> amtDist(1, maxMove);
                    int amt = amtDist(rng);
                    opts.emplace_back(from, to, amt);
                }
            }
        }
    }

    if (opts.empty()) return plan;

    std::uniform_int_distribution<int> pick(0, static_cast<int>(opts.size()) - 1);
    auto [f, t, amt] = opts[pick(rng)];
    plan.from = f;
    plan.to = t;
    plan.amount = amt;
    plan.valid = true;
    return plan;
}

} // namespace RandomAI
