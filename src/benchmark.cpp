#include "benchmark.h"
#include "board.h"
#include "move_list.h"
#include "test_suite.h"

void benchmarkMoveGen() {
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

    int iterations = 1'000'000;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        MoveList<ALL> moveList(board);
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Movegen speed: " << time_span.count() / static_cast<double>(iterations) << " microseconds" << std::endl;
}

void benchmarkMakeMove() {
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

    MoveList<ALL> moveList(board);

    int iterations = 1'000'000;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        board.makeMove(moveList.get(0));
        board.unmakeMove();
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Make/unmake speed: " << time_span.count() / static_cast<double>(iterations) << " microseconds" << std::endl;
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
    std::cout << "Engine speed: " << (static_cast<double>(result) / time_span.count()) / 1000000 << " MN/sec" << std::endl;
}
