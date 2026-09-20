#include "benchmark.h"
#include "board.h"
#include "eval_net.h"
#include "evaluate.h"
#include "move_list.h"
#include "test_suite.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

#ifdef __APPLE__
#include <pthread.h>
#include <time.h>
#endif

void benchmarkMoveGen() {
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

    int                                            iterations = 1'000'000;
    std::chrono::high_resolution_clock::time_point start      = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        MoveList<ALL> moveList(board);
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Movegen speed: " << time_span.count() / static_cast<double>(iterations) << " microseconds"
              << std::endl;
}

void benchmarkMakeMove() {
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

    MoveList<ALL> moveList(board);

    int                                            iterations = 1'000'000;
    std::chrono::high_resolution_clock::time_point start      = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        board.makeMove(moveList.get(0));
        board.undoMove();
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Make/unmake speed: " << time_span.count() / static_cast<double>(iterations) << " microseconds"
              << std::endl;
}

void benchmarkPerft() {
    // uint64_t answer = 3'195'901'860;

    Board board;
    board.setStartPos();

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    uint64_t result = moveGenTest(6, board);

    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    // assert(answer == result);

    auto time_span = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Engine speed: " << (static_cast<double>(result) / time_span.count()) / 1000000 << " MN/sec"
              << std::endl;
}

namespace {

uint64_t nowNs() {
#ifdef __APPLE__
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
#else
    using Clock = std::chrono::steady_clock;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count());
#endif
}

void clobber() { asm volatile("" ::: "memory"); }

double median(std::array<double, 7>& trials) {
    std::sort(trials.begin(), trials.end());
    return trials[3];
}

template<typename Fn>
double benchOnce(const Board& board, Fn fn, int warmup, int iters) {
    volatile int sink = 0;
    for (int i = 0; i < warmup; i++) {
        sink += fn(board);
        clobber();
    }
    const uint64_t t0 = nowNs();
    for (int i = 0; i < iters; i++) {
        sink += fn(board);
        clobber();
    }
    const uint64_t t1 = nowNs();
    if (sink == 0x7fffffff) {
        std::cout << "sink\n";
    }
    return static_cast<double>(t1 - t0) / static_cast<double>(iters);
}

}  // namespace

int benchmarkEval(const char* evalFile) {
#ifdef __APPLE__
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif
    if (!EvalNet::load(evalFile)) {
        std::cerr << "bench-eval: failed to load " << evalFile << '\n';
        return 1;
    }

    struct Case {
        const char* name;
        const char* fen;
    };
    const Case cases[] = {
        {"startpos", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"},
        {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"},
        {"krk", "8/8/8/4k3/8/8/8/4K2R w K - 0 1"},
    };

    constexpr int    kWarmup = 50'000;
    constexpr int    kIters  = 1'000'000;
    constexpr int    kTrials = 7;
    constexpr double kGHz    = 3.49;

    std::cout << "eval microbench  warmup=" << kWarmup << " iters=" << kIters << " trials=" << kTrials
              << " (median)\n";
    std::cout << "net: " << evalFile << '\n';
#ifdef __APPLE__
    std::cout << "clock: CLOCK_UPTIME_RAW, QoS USER_INTERACTIVE\n";
#endif
    std::cout << "cycles estimated at " << kGHz << " GHz\n\n";

    std::cout << std::left << std::setw(10) << "pos" << std::right << std::setw(12) << "pst ns" << std::setw(12)
              << "net ns" << std::setw(10) << "x pst" << std::setw(12) << "pst cyc" << std::setw(12) << "net cyc"
              << '\n';

    for (const auto& c : cases) {
        Board board(c.fen);
        std::array<double, kTrials> pst{};
        std::array<double, kTrials> net{};
        for (int t = 0; t < kTrials; t++) {
            pst[t] = benchOnce(board, pstEvaluation, kWarmup, kIters);
            net[t] = benchOnce(board, evaluation, kWarmup, kIters);
        }
        const double p = median(pst);
        const double n = median(net);
        std::cout << std::left << std::setw(10) << c.name << std::right << std::fixed << std::setprecision(2)
                  << std::setw(12) << p << std::setw(12) << n << std::setw(10) << (n / p) << std::setw(12)
                  << (p * kGHz) << std::setw(12) << (n * kGHz) << '\n';
    }
    return 0;
}
