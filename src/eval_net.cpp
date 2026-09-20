#include "eval_net.h"
#include "bit_manip.h"
#include "types.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace EvalNet {
namespace {

constexpr char kMagic[8] = {'M', 'U', 'A', 'D', 'N', 'E', 'T', '1'};

struct Weights {
    std::vector<float> w1;  // [kHidden, kInput]
    std::vector<float> b1;
    std::vector<float> w2;  // [kHidden, kHidden]
    std::vector<float> b2;
    std::vector<float> w3;  // [1, kHidden]
    float              b3 = 0.f;
};

Weights g_weights;
bool        g_loaded = false;
std::string g_path;

float relu(float x) { return x > 0.f ? x : 0.f; }

void collectFeatures(const Board& board, int* idx, int& n) {
    n                = 0;
    const bool black = board.blackToMove();
    const int  stm   = black ? 1 : 0;
    const int  opp   = stm ^ 1;
    const int  flip  = black ? 56 : 0;

    for (int pt = 0; pt < 6; pt++) {
        uint64_t stmBB = board.getBB(static_cast<Color>(stm), static_cast<PieceType>(pt * 2));
        uint64_t oppBB = board.getBB(static_cast<Color>(opp), static_cast<PieceType>(pt * 2));

        while (stmBB) {
            int sq     = tz_count(stmBB) ^ flip;
            idx[n++]   = pt * 64 + sq;
            pop_lsb(stmBB);
        }
        while (oppBB) {
            int sq     = tz_count(oppBB) ^ flip;
            idx[n++]   = (pt + 6) * 64 + sq;
            pop_lsb(oppBB);
        }
    }
}

int forward(const int* idx, int n) {
    float h1[kHidden];
    for (int o = 0; o < kHidden; o++) {
        float s = g_weights.b1[static_cast<size_t>(o)];
        const float* row = g_weights.w1.data() + o * kInput;
        for (int i = 0; i < n; i++) {
            s += row[idx[i]];
        }
        h1[o] = relu(s);
    }

    float h2[kHidden];
    for (int o = 0; o < kHidden; o++) {
        float s = g_weights.b2[static_cast<size_t>(o)];
        const float* row = g_weights.w2.data() + o * kHidden;
        for (int i = 0; i < kHidden; i++) {
            s += row[i] * h1[i];
        }
        h2[o] = relu(s);
    }

    float out = g_weights.b3;
    for (int i = 0; i < kHidden; i++) {
        out += g_weights.w3[static_cast<size_t>(i)] * h2[i];
    }

    // Network units are pawns from STM's view. Convert to white-positive centipawns.
    const int stmCp = static_cast<int>(std::lround(out * 100.f));
    return stmCp;
}

}  // namespace

bool load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }

    char magic[8];
    in.read(magic, 8);
    if (!in || std::string(magic, 8) != std::string(kMagic, 8)) {
        return false;
    }

    uint32_t input = 0, h1 = 0, h2 = 0, out = 0;
    in.read(reinterpret_cast<char*>(&input), sizeof(input));
    in.read(reinterpret_cast<char*>(&h1), sizeof(h1));
    in.read(reinterpret_cast<char*>(&h2), sizeof(h2));
    in.read(reinterpret_cast<char*>(&out), sizeof(out));
    if (!in || input != kInput || h1 != kHidden || h2 != kHidden || out != 1) {
        return false;
    }

    Weights w;
    w.w1.resize(kHidden * kInput);
    w.b1.resize(kHidden);
    w.w2.resize(kHidden * kHidden);
    w.b2.resize(kHidden);
    w.w3.resize(kHidden);

    auto readF = [&in](std::vector<float>& v) {
        in.read(reinterpret_cast<char*>(v.data()), static_cast<std::streamsize>(v.size() * sizeof(float)));
    };
    readF(w.w1);
    readF(w.b1);
    readF(w.w2);
    readF(w.b2);
    readF(w.w3);
    in.read(reinterpret_cast<char*>(&w.b3), sizeof(w.b3));
    if (!in) {
        return false;
    }

    g_weights = std::move(w);
    g_path    = path;
    g_loaded  = true;
    return true;
}

bool loaded() { return g_loaded; }

const std::string& path() { return g_path; }

int evaluate(const Board& board) {
    int idx[32];
    int n = 0;
    collectFeatures(board, idx, n);
    int stmCp = forward(idx, n);
    return board.blackToMove() ? -stmCp : stmCp;
}

}  // namespace EvalNet
