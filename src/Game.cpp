#include "Game.h"

#include <algorithm>
#include <queue>
#include <unordered_set>
#include <sstream>
#include <iomanip>

// ---- ctor & map ----
Game::Game(unsigned seed) { build20(seed); current_ = P1; turnCount_ = 0; staleTurns_ = 0; }

void Game::build20(unsigned seed) {
    board_.clear();
    board_.resize(20);
    // Place 20 territories on a 6x6-ish canvas with scattered coords
    // Codes A..T
    const char codes[20] = {
        'A','B','C','D','E','F','G','H','I','J',
        'K','L','M','N','O','P','Q','R','S','T'
    };
    // Hand-picked coordinates for a nice organic layout (0-based rows/cols)
    const std::pair<int,int> coords[20] = {
        {0,1},{0,3},{0,5},
        {1,0},{1,2},{1,4},
        {2,1},{2,3},{2,5},
        {3,0},{3,2},{3,4},
        {4,1},{4,3},{4,5},
        {5,2},{5,4},{5,0},{5,1},{5,3}
    };
    for (int i=0;i<20;++i) {
        board_[i].code = codes[i];
        board_[i].name = std::string("Terr ") + codes[i];
        board_[i].r = coords[i].first;
        board_[i].c = coords[i].second;
        board_[i].owner = NONE;
        board_[i].armies = 0;
    }
    // Build adjacency: 4-neighbors (up/down/left/right) if a territory exists at that coord
    auto findAt = [&](int r, int c)->int{
        for (int i=0;i<20;++i) if (board_[i].r==r && board_[i].c==c) return i;
        return -1;
    };
    const int dr[4] = {-1,1,0,0};
    const int dc[4] = {0,0,-1,1};
    for (int i=0;i<20;++i) {
        board_[i].adj.clear();
        for (int k=0;k<4;++k) {
            int nr = board_[i].r + dr[k];
            int nc = board_[i].c + dc[k];
            int j = findAt(nr,nc);
            if (j>=0) board_[i].adj.push_back(j);
        }
    }
    (void)seed; // seed not strictly needed for static layout
}

// ---- status / ownership ----
Status Game::status() const {
    int p1=0, p2=0;
    for (const auto& t: board_) {
        if (t.owner==P1) ++p1;
        else if (t.owner==P2) ++p2;
    }
    if (p1>0 && p2==0) return PLAYER_1_WINS;
    if (p2>0 && p1==0) return PLAYER_2_WINS;
    if (turnCount_ > maxTurnsCap_) return DRAW;
    if (staleTurns_ >= staleCap_) return DRAW;
    return ONGOING;
}

std::vector<TerrId> Game::owned(Player p) const {
    std::vector<TerrId> out;
    for (int i=0;i<(int)board_.size();++i) if (board_[i].owner==p) out.push_back(i);
    return out;
}

std::vector<TerrId> Game::borders(Player p) const {
    std::vector<TerrId> out;
    for (int i=0;i<(int)board_.size();++i) {
        const auto& t=board_[i];
        if (t.owner!=p) continue;
        bool touch=false;
        for (auto n: t.adj) if (board_[n].owner!=p) { touch=true; break; }
        if (touch) out.push_back(i);
    }
    return out;
}

// ---- setup / turns ----
void Game::dealStartingPositions() {
    // Randomly assign half P1, half P2; armies=1
    std::vector<int> idx(board_.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::mt19937 rng(std::random_device{}());
    std::shuffle(idx.begin(), idx.end(), rng);
    for (int k=0;k<(int)board_.size();++k) {
        board_[idx[k]].owner = (k < (int)board_.size()/2) ? P1 : P2;
        board_[idx[k]].armies = 1;
    }
}

int Game::baseReinforcements(Player p) const {
    int n = (int)owned(p).size();
    return std::max(3, n/3);
}

int Game::componentSizeFrom(TerrId start, Player p) const {
    if (start<0 || start>=(int)board_.size()) return 0;
    if (board_[start].owner!=p) return 0;
    std::vector<char> vis(board_.size(),0);
    std::queue<int> q;
    q.push(start); vis[start]=1;
    int cnt=0;
    while(!q.empty()){
        int u=q.front(); q.pop();
        ++cnt;
        for (auto v: board_[u].adj) if (!vis[v] && board_[v].owner==p) { vis[v]=1; q.push(v); }
    }
    return cnt;
}

bool Game::chainBonusTarget(Player p, TerrId& tIdx) const {
    int bestSize=0; int pick=-1;
    for (int i=0;i<(int)board_.size();++i) {
        if (board_[i].owner!=p) continue;
        int sz = componentSizeFrom(i,p);
        if (sz>=5 && sz>bestSize) { bestSize=sz; pick=i; }
    }
    if (bestSize>=5) { tIdx=pick; return true; }
    return false;
}

void Game::addArmies(TerrId t, int amount) {
    if (t<0 || t>=(int)board_.size() || amount<=0) return;
    board_[t].armies += amount;
}

void Game::nextPlayer() { current_ = (current_==P1 ? P2 : P1); }

void Game::incrementTurnCount(bool capturedThisTurn) {
    ++turnCount_;
    if (capturedThisTurn) staleTurns_ = 0; else ++staleTurns_;
}

// ---- mapping & adjacency ----
TerrId Game::findByCode(char code) const {
    char u = (char)std::toupper((unsigned char)code);
    for (int i=0;i<(int)board_.size();++i) if ((char)std::toupper((unsigned char)board_[i].code)==u) return i;
    return -1;
}
bool Game::areAdjacent(TerrId a, TerrId b) const {
    if (a<0||b<0||a>=(int)board_.size()||b>=(int)board_.size()) return false;
    const auto& nb = board_[a].adj;
    return std::find(nb.begin(), nb.end(), b)!=nb.end();
}

// ---- attacks & dice ----
int Game::attackerDice(int armiesAtFrom) { return (armiesAtFrom<=1) ? 0 : std::min(3, armiesAtFrom-1); }
int Game::defenderDice(int armiesAtTo)   { return (armiesAtTo<=0) ? 0 : std::min(2, armiesAtTo); }

std::pair<int,int> Game::simulateBattleOnce(int attDice, int defDice, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(1,6);
    std::vector<int> A(attDice), D(defDice);
    for (int i=0;i<attDice;++i) A[i]=d(rng);
    for (int j=0;j<defDice;++j) D[j]=d(rng);
    std::sort(A.begin(),A.end(),std::greater<int>());
    std::sort(D.begin(),D.end(),std::greater<int>());
    int pairs = std::min(attDice, defDice);
    int aLoss=0, dLoss=0;
    for (int i=0;i<pairs;++i) {
        if (A[i] > D[i]) ++dLoss; else ++aLoss; // ties to defender
    }
    return {aLoss,dLoss};
}

bool Game::canAttack(TerrId from, TerrId to, Player attacker) const {
    if (from<0||to<0||from>=(int)board_.size()||to>=(int)board_.size()) return false;
    if (from==to) return false;
    const auto& A = board_[from];
    const auto& D = board_[to];
    if (A.owner!=attacker) return false;
    if (D.owner==attacker || D.owner==NONE) return false;
    if (!areAdjacent(from,to)) return false;
    if (A.armies<2) return false;
    return true;
}

bool Game::applyBattle(TerrId from, TerrId to, Player attacker, unsigned seed,
                       int& outAttackerLoss, int& outDefenderLoss, bool& outCaptured) {
    outAttackerLoss=0; outDefenderLoss=0; outCaptured=false;
    if (!areAdjacent(from,to)) return false;
    auto &A = board_[from], &D = board_[to];
    if (A.owner!=attacker || D.owner==attacker) return false;

    int aDice = attackerDice(A.armies);
    int dDice = defenderDice(D.armies);
    if (aDice<=0 || dDice<=0) return false;

    auto loss = simulateBattleOnce(aDice, dDice, seed);
    A.armies -= loss.first;
    D.armies -= loss.second;
    outAttackerLoss = loss.first;
    outDefenderLoss = loss.second;

    if (D.armies<=0) {
        D.owner = attacker;
        D.armies = 0;
        outCaptured = true;
        return true;
    }
    return true;
}

void Game::moveAfterCapture(TerrId from, TerrId to, int armiesToMove) {
    auto &A = board_[from], &T = board_[to];
    if (armiesToMove<=0) return;
    if (A.armies - armiesToMove < 1) armiesToMove = std::max(0, A.armies-1);
    if (armiesToMove<=0) return;
    A.armies -= armiesToMove;
    T.armies += armiesToMove;
}

// ---- fortify path ----
bool Game::reachableOwned(TerrId from, TerrId to, Player p) const {
    if (from<0||to<0||from>=(int)board_.size()||to>=(int)board_.size()) return false;
    if (board_[from].owner!=p || board_[to].owner!=p) return false;
    std::queue<int> q; std::vector<char> vis(board_.size(),0);
    q.push(from); vis[from]=1;
    while(!q.empty()){
        int u=q.front(); q.pop();
        if (u==to) return true;
        for (auto v: board_[u].adj) if (!vis[v] && board_[v].owner==p) { vis[v]=1; q.push(v); }
    }
    return false;
}
bool Game::canFortifyPath(TerrId from, TerrId to, Player p) const {
    if (from==to) return false;
    if (board_[from].owner!=p || board_[to].owner!=p) return false;
    if (board_[from].armies<2) return false;
    return reachableOwned(from,to,p);
}
void Game::fortify(TerrId from, TerrId to, int amount) {
    auto &A=board_[from], &B=board_[to];
    if (amount<=0) return;
    if (A.armies - amount < 1) amount = std::max(0, A.armies-1);
    if (amount<=0) return;
    A.armies -= amount;
    B.armies += amount;
}

// ---- colored rendering (2 lines per row: top=code, bottom=armies) ----
std::string Game::renderColor(int cellWidth) const {
    int maxR=0, maxC=0;
    for (const auto& t: board_) { maxR=std::max(maxR,t.r); maxC=std::max(maxC,t.c); }
    int rows=maxR+1, cols=maxC+1;

    auto terrAt = [&](int r,int c)->const Territory* {
        for (const auto& t: board_) if (t.r==r && t.c==c) return &t;
        return nullptr;
    };

    auto bg = [](const char* code){ return std::string("\x1b[")+code+"m"; };
    const std::string RESET="\x1b[0m";
    const std::string FG_WHITE="\x1b[97m";
    const std::string BG_BLACK=bg("40");
    const std::string BG_GREY =bg("100");
    const std::string BG_BLUE =bg("44");
    const std::string BG_RED  =bg("41");

    cellWidth = std::max(2, cellWidth);
    std::string out;

    for (int r=0;r<rows;++r) {
        // top (code)
        for (int c=0;c<cols;++c) {
            auto T = terrAt(r,c);
            std::string cell(cellWidth,' ');
            if (T) cell[0]=T->code;

            if (!T) out+=BG_BLACK;
            else if (T->owner==P1) out+=BG_BLUE;
            else if (T->owner==P2) out+=BG_RED;
            else out+=BG_GREY;

            out += FG_WHITE + cell + RESET;
        }
        out += '\n';
        // bottom (armies)
        for (int c=0;c<cols;++c) {
            auto T = terrAt(r,c);
            std::string cell(cellWidth,' ');
            if (T) {
                std::string a = std::to_string(std::max(0, T->armies));
                if ((int)a.size()>cellWidth) a = a.substr((int)a.size()-cellWidth);
                int start = cellWidth - (int)a.size();
                for (size_t i=0;i<a.size();++i)
                    if (start+(int)i<cellWidth) cell[start+(int)i]=a[i];
            }

            if (!T) out+=BG_BLACK;
            else if (T->owner==P1) out+=BG_BLUE;
            else if (T->owner==P2) out+=BG_RED;
            else out+=BG_GREY;

            out += FG_WHITE + cell + RESET;
        }
        out += '\n';
    }
    return out;
}

// ---- AI helpers ----
bool Game::aiChooseReinforcement(TerrId& where, int /*reinforcements*/, unsigned seed) const {
    auto mine = owned(P2);
    if (mine.empty()) return false;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0,(int)mine.size()-1);
    where = mine[dist(rng)];
    return true;
}

// Estimate chance of capturing 'to' by repeatedly simulating a full fight from current armies
double Game::estimateCaptureProb(TerrId from, TerrId to, int trials, unsigned seed) const {
    int atk = board_[from].armies;
    int def = board_[to].armies;
    if (atk < 2) return 0.0;
    if (def <= 0) return 1.0;

    int wins = 0;
for (int t = 0; t < trials; ++t) {
    int a = atk;
    int d = def;

    unsigned s = seed + static_cast<unsigned>(t * 7919 + from * 97 + to * 131);

    while (a > 1 && d > 0) {
    int aDice = attackerDice(a);
    int dDice = defenderDice(d);

    if (aDice <= 0 || dDice <= 0) {
        break;
    } else {
        auto loss = simulateBattleOnce(aDice, dDice, s++);
        a -= loss.first;
        d -= loss.second;

        if (a < 0) {
            a = 0;
        }
        if (d < 0) {
            d = 0;
        }
    }
}


    if (d <= 0 && a > 0) {
        ++wins;
    }
}


    return static_cast<double>(wins) / static_cast<double>(trials);
}


Game::AIPlanAtt Game::aiChooseAttack(unsigned seed) const {
    AIPlanAtt plan;
    auto bdr = borders(P2);
    struct Pair{TerrId f; TerrId t;};
    std::vector<Pair> cand;
    for (auto f: bdr) {
        if (board_[f].armies<2) continue;
        for (auto t: board_[f].adj) if (canAttack(f,t,P2)) cand.push_back({f,t});
    }
    if (cand.empty()) { plan.valid=false; return plan; }

    const int kTrials = 80;         // quality vs speed
    const double minAccept = 0.40;  // conservative threshold
    double bestScore=-1.0; Pair best{-1,-1};

    for (size_t i=0;i<cand.size();++i){
        const auto& pr=cand[i];
        double prob = estimateCaptureProb(pr.f, pr.t, kTrials, seed + (unsigned)i*31);
        int diff = board_[pr.f].armies - board_[pr.t].armies;
        double score = prob + 0.001*diff; // tie-break
        if (score>bestScore) { bestScore=score; best=pr; }
    }
    if (bestScore < (minAccept - 0.5)) { plan.valid=false; return plan; } // raw prob approx

    plan.from=best.f; plan.to=best.t; plan.valid=true;
    return plan;
}

Game::AIPlanFort Game::aiChooseFortify(unsigned seed) const {
    AIPlanFort plan;
    auto mine = owned(P2);
    if (mine.empty()) { plan.valid=false; return plan; }
    // simple: look for adjacent friendly to move a few troops toward border
    std::mt19937 rng(seed);
    for (auto from: mine) {
        if (board_[from].armies<2) continue;
        for (auto to: mine) {
            if (from==to) continue;
            if (!canFortifyPath(from,to,P2)) continue;
            int maxMove = board_[from].armies-1;
            if (maxMove<=0) continue;
            std::uniform_int_distribution<int> d(1, maxMove);
            plan.from=from; plan.to=to; plan.amount=d(rng); plan.valid=true;
            return plan;
        }
    }
    plan.valid=false; return plan;
}
