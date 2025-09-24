#include "RandomAI.h"
#include "Rules.h"
#include "Board.h"

#include <algorithm>
#include <random>
#include <vector>
#include <limits>

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

// ---------- Helpers for attack evaluation ----------

// Simulate a single full battle (attacker keeps attacking from 'afake' until defender captured
// or attacker can't attack). Uses Rules::simulateBattleOnce for individual dice rounds.
// Returns true if attacker captured 'to' in this simulated full fight.
static bool simulateFullBattleOnce(int atkArmiesStart, int defArmiesStart, unsigned seed) {
    std::mt19937 rng(seed);
    int a = atkArmiesStart;
    int d = defArmiesStart;
    // keep rolling until attacker can't attack (a <= 1) or defender is zero
    unsigned localSeed = seed;
    while (a > 1 && d > 0) {
        int aDice = Rules::attackerDice(a);
        int dDice = Rules::defenderDice(d);
        if (aDice <= 0 || dDice <= 0) break;
        // use a changing seed for each roll to vary RNG deterministically
        auto losses = Rules::simulateBattleOnce(aDice, dDice, localSeed++);
        a -= losses.attacker;
        d -= losses.defender;
        // guard against infinite loops (shouldn't happen)
        if (a < 0) a = 0;
        if (d < 0) d = 0;
    }
    return (d <= 0 && a > 0);
}

// Estimate probability attacker captures `to` if starting with current armies.
// Runs `trials` independent simulated full battles, seeded from baseSeed.
// Returns probability in [0,1].
static double estimateCaptureProb(const Board& b, TerrId from, TerrId to, int trials, std::uint32_t baseSeed) {
    if (from < 0 || to < 0) return 0.0;
    int atk = b.at(from).armies;
    int def = b.at(to).armies;
    if (atk < 2) return 0.0;
    if (def <= 0) return 1.0;
    int wins = 0;
    // Keep trials modest (e.g., 50-200). Use varying seeds.
    for (int t = 0; t < trials; ++t) {
        unsigned s = baseSeed + static_cast<unsigned>(t * 7919 + from * 97 + to * 131);
        if (simulateFullBattleOnce(atk, def, s)) ++wins;
    }
    return static_cast<double>(wins) / static_cast<double>(trials);
}

// ---------- Attack ----------
AttackPlan chooseAttack(const Board& b, PlayerId p, std::uint32_t seed) {
    AttackPlan plan;
    // find border territories for player p
    auto borders = Rules::borders(b, p);
    if (borders.empty()) {
        plan.valid = false;
        return plan;
    }

    // build list of legal (from,to)
    struct Pair { TerrId f; TerrId t; };
    std::vector<Pair> legalPairs;
    for (TerrId from : borders) {
        if (b.at(from).armies < 2) continue; // must leave 1 behind
        for (TerrId to : b.neighbors(from)) {
            if (Rules::canAttack(b, from, to, p)) {
                legalPairs.push_back({from, to});
            }
        }
    }
    if (legalPairs.empty()) {
        plan.valid = false;
        return plan;
    }

    // Evaluate each candidate with a small Monte-Carlo to estimate capture probability.
    // Tweak these to adjust quality vs speed.
    const int kTrials = 80;      // number of full-battle simulations per candidate
    const double minAccept = 0.40; // minimum probability to accept an attack (if none reach this, skip)

    double bestProb = -1.0;
    Pair bestPair{ -1, -1 };

    for (size_t i = 0; i < legalPairs.size(); ++i) {
        const auto &pr = legalPairs[i];
        double prob = estimateCaptureProb(b, pr.f, pr.t, kTrials, seed + static_cast<unsigned>(i * 31));
        // Slight tie-breaker favor bigger army advantage if probabilities similar
        int diff = b.at(pr.f).armies - b.at(pr.t).armies;
        double score = prob + 0.001 * static_cast<double>(diff); // small bias
        if (score > bestProb) {
            bestProb = score;
            bestPair = pr;
        }
    }

    // If best estimated raw probability is below minAccept, skip attacking
    // (use bestProb minus tiny bias to approximate raw prob)
    if (bestProb < (minAccept - 0.001 * 1000)) { // avoid accidental bias issues
        plan.valid = false;
        return plan;
    }

    plan.from = bestPair.f;
    plan.to   = bestPair.t;
    plan.valid = true;
    return plan;
}

// ---------- Fortify ----------
FortifyPlan chooseFortify(const Board& b, PlayerId p, std::uint32_t seed) {
    FortifyPlan plan;

    auto ownedList = Rules::owned(b, p);
    if (ownedList.empty()) {
        plan.valid = false;
        return plan;
    }

    std::mt19937 rng(seed);
    std::vector<std::tuple<TerrId,TerrId,int>> options;

    for (TerrId from : ownedList) {
        if (b.at(from).armies < 2) continue; // must leave 1 behind
        for (TerrId to : b.neighbors(from)) {
            if (Rules::canFortify(b, from, to, p)) {
                int maxMove = b.at(from).armies - 1;
                if (maxMove > 0) {
                    std::uniform_int_distribution<int> amtDist(1, maxMove);
                    int amt = amtDist(rng);
                    options.emplace_back(from, to, amt);
                }
            }
        }
    }

    if (options.empty()) {
        plan.valid = false;
        return plan;
    }

    std::uniform_int_distribution<int> dist(0, static_cast<int>(options.size()) - 1);
    auto [f, t, amt] = options[dist(rng)];
    plan.from = f;
    plan.to   = t;
    plan.amount = amt;
    plan.valid = true;
    return plan;
}

} // namespace RandomAI
