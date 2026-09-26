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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct SelfPlayConfig {
    int         games       = 10;
    int         depth       = 4;
    int         tempPlies   = 12;
    float       temp        = 150.f;
    int         maxPly      = 120;
    int         resignScore = 600;
    int         resignPlies = 3;
    int         drawScore   = 50;
    int         drawPlies   = 8;
    int         workers     = 1;
    uint64_t    seed        = 0;
    bool        hasSeed     = false;
    std::string out         = "train/data/games.jsonl";
    std::string starts      = "train/data/starts.fen";
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
        } else if (arg == "--resign-score") {
            const char* v = next("--resign-score");
            if (!v) return false;
            cfg.resignScore = std::stoi(v);
        } else if (arg == "--draw-score") {
            const char* v = next("--draw-score");
            if (!v) return false;
            cfg.drawScore = std::stoi(v);
        } else if (arg == "--draw-plies") {
            const char* v = next("--draw-plies");
            if (!v) return false;
            cfg.drawPlies = std::stoi(v);
        } else if (arg == "--seed") {
            const char* v = next("--seed");
            if (!v) return false;
            cfg.seed    = std::stoull(v);
            cfg.hasSeed = true;
        } else if (arg == "--workers") {
            const char* v = next("--workers");
            if (!v) return false;
            cfg.workers = std::stoi(v);
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
    return cfg.games > 0 && cfg.depth > 0 && cfg.workers > 0;
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
    searcher.newSearch();

    int resignCount = 0;
    int drawCount   = 0;

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

        const bool collectRoot = ply < cfg.tempPlies;
        auto [bestMove, stmScore] = searcher.searchDepth(board, cfg.depth, collectRoot);
        if (bestMove.isNull()) {
            return 0;
        }

        records.push_back({board.toFen(), stmScore});

        const int absScore = std::abs(stmScore);
        if (absScore >= cfg.resignScore) {
            resignCount++;
            drawCount = 0;
            if (resignCount >= cfg.resignPlies) {
                const int whiteScore = board.blackToMove() ? -stmScore : stmScore;
                return whiteScore > 0 ? 1 : -1;
            }
        } else if (absScore < cfg.drawScore) {
            drawCount++;
            resignCount = 0;
            if (drawCount >= cfg.drawPlies) {
                return 0;
            }
        } else {
            resignCount = 0;
            drawCount   = 0;
        }

        Move chosen = bestMove;
        if (collectRoot) {
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
                  << "                     [--max-ply N] [--resign-score N] [--draw-score N] [--draw-plies N]\n"
                  << "                     [--seed N] [--workers K]\n"
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

    uint64_t seed = cfg.hasSeed
        ? cfg.seed
        : (static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()) | 1ull);
    if (seed == 0) {
        seed = 1;
    }

    int nWorkers = cfg.workers;
    if (nWorkers > cfg.games) {
        nWorkers = cfg.games;
    }
    std::cerr << "selfplay: seed " << seed << " workers " << nWorkers << '\n';

    std::mutex outMutex;

    auto workerFn = [&](int workerId) {
        Searcher searcher;
        PRNG     rng((seed + static_cast<uint64_t>(workerId)) | 1ull);
        std::vector<PositionRecord> records;

        for (int g = workerId; g < cfg.games; g += nWorkers) {
            const std::string& fen = starts[static_cast<size_t>(g) % starts.size()];
            Board              board(fen);
            int whiteResult = playGame(board, cfg, searcher, rng, records);

            std::lock_guard<std::mutex> lock(outMutex);
            for (const auto& rec : records) {
                out << "{\"fen\":\"" << rec.fen << "\",\"stm_score\":" << rec.stmScore
                    << ",\"white_result\":" << whiteResult << "}\n";
            }
            std::cerr << "selfplay: game " << (g + 1) << '/' << cfg.games << " positions " << records.size()
                      << " result " << whiteResult << '\n';
        }
    };

    if (nWorkers == 1) {
        workerFn(0);
    } else {
        std::vector<std::thread> threads;
        threads.reserve(static_cast<size_t>(nWorkers));
        for (int w = 0; w < nWorkers; w++) {
            threads.emplace_back(workerFn, w);
        }
        for (auto& t : threads) {
            t.join();
        }
    }

    std::cerr << "selfplay: wrote " << cfg.out << '\n';
    return 0;
}
