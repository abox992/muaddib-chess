#include "benchmark.h"
#include "cli.h"
#include "eval_net.h"
#include "selfplay.h"
#include "uci.h"
#include "zobrist.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    Zobrist::initZobrist();

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--eval") {
            if (i + 1 >= argc) {
                std::cerr << "missing path after --eval\n";
                return 1;
            }
            const char* evalFile = argv[++i];
            if (!EvalNet::load(evalFile)) {
                std::cerr << "failed to load eval net " << evalFile << '\n';
                return 1;
            }
            std::cerr << "using eval net " << evalFile << '\n';
        }
    }

    if (argc >= 2 && std::string(argv[1]) == "selfplay") {
        return runSelfPlay(argc, argv);
    }
    if (argc >= 2 && std::string(argv[1]) == "uci") {
        return runUCI();
    }
    if (argc >= 2 && std::string(argv[1]) == "bench-eval") {
        const char* evalFile = argc >= 3 && std::string(argv[2]) != "--eval" ? argv[2] : "train/data/eval.muadnet";
        return benchmarkEval(evalFile);
    }
    if (argc >= 2 && std::string(argv[1]) == "play") {
        cli::asciiGameLoop();
        return 0;
    }

    cli::loop();
    return 0;
}
