#include "selfplay.h"
#include "board.h"
#include "eval_net.h"
#include "helpers.h"
#include "move.h"
#include "move_list.h"
#include "search.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct SelfPlayConfig {
    int         games     = 10;
    int         depth     = 4;
    int         tempPlies = 12;
    float       temp      = 150.f;
    int         maxPly    = 512;
    std::string out       = "train/data/games.jsonl";
    std::string starts    = "train/data/starts.fen";
    std::string evalFile;
};

struct PositionRecord {
    std::string fen;
    int         stmScore = 0;
};

bool parseArgs(int argc, char* argv[], SelfPlayConfig& cfg) {
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        auto next = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                std::cerr << "selfplay: missing value for " << name << '\n';
                return nullptr;
            }
            return argv[++i];
        };

        if (arg == "--games") {
            const char* v = next("--games");
            if (!v) return false;
            cfg.games = std::stoi(v);
        } else if (arg == "--depth") {
            const char* v = next("--depth");
            if (!v) return false;
            cfg.depth = std::stoi(v);
        } else if (arg == "--temp") {
            const char* v = next("--temp");
            if (!v) return false;
            cfg.temp = std::stof(v);
        } else if (arg == "--temp-plies") {
            const char* v = next("--temp-plies");
            if (!v) return false;
            cfg.tempPlies = std::stoi(v);
        } else if (arg == "--max-ply") {
            const char* v = next("--max-ply");
            if (!v) return false;
            cfg.maxPly = std::stoi(v);
        } else if (arg == "--out") {
            const char* v = next("--out");
            if (!v) return false;
            cfg.out = v;
        } else if (arg == "--starts") {
            const char* v = next("--starts");
            if (!v) return false;
            cfg.starts = v;
        } else if (arg == "--eval") {
            const char* v = next("--eval");
            if (!v) return false;
            cfg.evalFile = v;
        } else {
            std::cerr << "selfplay: unknown arg " << arg << '\n';
            return false;
        }
    }
    return cfg.games > 0 && cfg.depth > 0;
}

std::string extractFen(const std::string& line) {
    if (line.empty() || line[0] != '{') {
        return line;
    }
    const std::string key = "\"fen\"";
    auto pos = line.find(key);
    if (pos == std::string::npos) {
        return {};
    }
    pos = line.find('"', pos + key.size());
    if (pos == std::string::npos) {
        return {};
    }
    auto end = line.find('"', pos + 1);
    if (end == std::string::npos) {
        return {};
    }
    return line.substr(pos + 1, end - pos - 1);
}

std::vector<std::string> loadStarts(const std::string& path) {
    std::vector<std::string> fens;
    if (path.empty()) {
        fens.emplace_back("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        return fens;
    }

    std::ifstream in(path);
    std::string   line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        std::string fen = extractFen(line);
        if (!fen.empty()) {
            fens.push_back(fen);
        }
    }
    if (fens.empty()) {
        std::cerr << "selfplay: no start positions in " << path << '\n';
    }
    return fens;
}

Move sampleMove(const std::vector<std::pair<Move, int>>& roots, float temp, PRNG& rng) {
    if (roots.empty()) {
        return Move(0);
    }
    if (temp <= 0.f || roots.size() == 1) {
        return roots.front().first;
    }

    double maxScore = roots.front().second;
    for (const auto& entry : roots) {
        maxScore = std::max(maxScore, static_cast<double>(entry.second));
    }

    std::vector<double> weights;
    weights.reserve(roots.size());
    double sum = 0.0;
    for (const auto& entry : roots) {
        double w = std::exp((static_cast<double>(entry.second) - maxScore) / static_cast<double>(temp));
        weights.push_back(w);
        sum += w;
    }

    double draw = (static_cast<double>(rng.rand<uint64_t>()) / static_cast<double>(UINT64_MAX)) * sum;
    double acc  = 0.0;
    for (size_t i = 0; i < roots.size(); i++) {
        acc += weights[i];
        if (draw <= acc) {
            return roots[i].first;
        }
    }
    return roots.back().first;
}

int playGame(Board& board, const SelfPlayConfig& cfg, Searcher& searcher, PRNG& rng,
             std::vector<PositionRecord>& records) {
    records.clear();

    for (int ply = 0; ply < cfg.maxPly; ply++) {
        MoveList<ALL> legal(board);
        if (legal.size() == 0) {
            if (board.inCheck()) {
                return board.blackToMove() ? 1 : -1;
            }
            return 0;
        }
        if (board.getHalfMoves() >= 100 || board.getRepeats(board.hash()) >= 3) {
            return 0;
        }

        auto [bestMove, stmScore] = searcher.searchDepth(board, cfg.depth);
        if (bestMove.isNull()) {
            return 0;
        }

        records.push_back({board.toFen(), stmScore});

        Move chosen = bestMove;
        if (ply < cfg.tempPlies) {
            const auto& roots = searcher.getRootMoves();
            if (!roots.empty()) {
                chosen = sampleMove(roots, cfg.temp, rng);
            }
        }
        board.makeMove(chosen);
    }
    return 0;
}

}  // namespace

int runSelfPlay(int argc, char* argv[]) {
    SelfPlayConfig cfg;
    if (!parseArgs(argc, argv, cfg)) {
        std::cerr << "usage: chess selfplay [--games N] [--depth N] [--temp T] [--temp-plies N]\n"
                  << "                     [--out train/data/games.jsonl] [--starts train/data/starts.fen] [--eval net.muadnet]\n";
        return 1;
    }

    if (!cfg.evalFile.empty()) {
        if (!EvalNet::load(cfg.evalFile)) {
            std::cerr << "selfplay: failed to load eval net " << cfg.evalFile << '\n';
            return 1;
        }
        std::cerr << "selfplay: using eval net " << cfg.evalFile << '\n';
    } else {
        std::cerr << "selfplay: using PST eval\n";
    }

    auto starts = loadStarts(cfg.starts);
    if (starts.empty()) {
        std::cerr << "selfplay: no start positions (tried " << cfg.starts << ")\n";
        return 1;
    }

    std::filesystem::path outPath(cfg.out);
    if (outPath.has_parent_path()) {
        std::filesystem::create_directories(outPath.parent_path());
    }
    std::ofstream out(cfg.out);
    if (!out) {
        std::cerr << "selfplay: cannot write " << cfg.out << '\n';
        return 1;
    }

    Searcher searcher;
    PRNG     rng(static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()) | 1ull);
    std::vector<PositionRecord> records;

    for (int g = 0; g < cfg.games; g++) {
        const std::string& fen = starts[static_cast<size_t>(g) % starts.size()];
        Board board(fen);
        int   whiteResult = playGame(board, cfg, searcher, rng, records);

        for (const auto& rec : records) {
            out << "{\"fen\":\"" << rec.fen << "\",\"stm_score\":" << rec.stmScore
                << ",\"white_result\":" << whiteResult << "}\n";
        }
        std::cerr << "selfplay: game " << (g + 1) << '/' << cfg.games << " positions " << records.size()
                  << " result " << whiteResult << '\n';
    }

    std::cerr << "selfplay: wrote " << cfg.out << '\n';
    return 0;
}
