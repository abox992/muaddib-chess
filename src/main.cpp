#include <iostream>
#include "board.h"
#include "helpers.h"
#include "precompute_masks.h"
#include "movegen.h"
#include "test_suite.h"
#include "zobrist.h"
#include "search.h"
#include "evaluate.h"

//#include "gui/game.h"

#include <bit>
#include <sstream>
#include <chrono>

void asciiGameLoop();
void benchmarkMoveGen();
void benchmarkPerft();
void benchmarkMakeMove();

int main() {
    Board board;
    board.setStartPos();
    initMasks();
    initZobrist();

    std::vector<Move> moveList;
    moveList.reserve(256);

    std::cout << board << std::endl;

    if (board.curState->blackToMove) {
        generateMoves<MoveFilter::ALL, Color::BLACK>(board, moveList);
    } else {
        generateMoves<MoveFilter::ALL, Color::WHITE>(board, moveList);
    }

    //benchmarkMoveGen();
    //benchmarkMakeMove();
    //benchmarkPerft();

    //runTests();

    // board = generateBoardFromFen("rnb1kbn1/ppp5/4p3/8/7q/1PP1P3/P2K3p/RNBQ1B1R b q - 0 14");
    // getBestMove(board, 7);

    //asciiGameLoop();
    board.set("rn3bnr/p2q4/4k3/2pb1p2/P1N4P/1Q2PB2/5N2/R1B1K2R b KQ - 3 23");
    Move move = getBestMove(board, 5);
    std::cout << move << '\n';

    //Game game;
    //game.runGameLoop();

    return 0;
}

void benchmarkMoveGen() {
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    std::vector<Move> moveList;
    moveList.reserve(256);

    int iterations = 1'000'000;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        generateMoves<MoveFilter::ALL, Color::WHITE>(board, moveList);
        moveList.clear();
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Movegen speed: " << time_span.count() / static_cast<double>(iterations) << " microseconds" << std::endl;
}

void benchmarkMakeMove() {
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    std::vector<Move> moveList;
    moveList.reserve(256);
    generateMoves<MoveFilter::ALL, Color::WHITE>(board, moveList);

    int iterations = 1'000'000;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        board.makeMove(moveList[0]);
        board.unmakeMove();
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Make/unmake speed: " << time_span.count() / static_cast<double>(iterations) << " microseconds" << std::endl;
}

void benchmarkPerft() {
    //uint64_t answer = 3'195'901'860;

    Board board;
    board.setStartPos();

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    uint64_t result = moveGenTest(6, board);

    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    //assert(answer == result);

    auto time_span = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Engine speed: " << (static_cast<double>(result) / time_span.count()) / 1000000 << " MN/sec" << std::endl;
}

void asciiGameLoop() {
    Board board;
    board.setStartPos();

    //board = generateBoardFromFen("r1bqk2r/1pp2ppp/p1np1n2/2b1p3/B3P3/2NP1N2/PPP2PPP/R1BQK2R w KQkq - 0 7");

    // ascii console game loop
    std::stringstream pgn;
    int moveNum = 1;
    while (board.curState->halfMoves < 100 && board.curState->highestRepeat < 3) {
        Move bestMove = getBestMove(board, 5);

        if (bestMove.isNull()) { // no moves available
            break;
        }

        std::cout << moveNum << ". " << bestMove << std::endl;
        std::cout << evaluation(board) << '\n';

        std::vector<Move> moveList;
        moveList.reserve(256);

        if (board.curState->blackToMove) {
            generateMoves<ALL, Color::BLACK>(board, moveList);
        } else {
            generateMoves<ALL, Color::WHITE>(board, moveList);
        }

        // for (auto& move : moveList) {
        //     std::cout << move << '\n';
        // }

        // update pgn
        if (!board.curState->blackToMove) {
            pgn << " " << moveNum << ". ";
        }

        pgn << movePretty(board, bestMove);

        // uint64_t fromMask = maskForPos(bestMove.from());
        // uint64_t toMask = maskForPos(bestMove.from());
        // const Color color = static_cast<Color>(board.curState->blackToMove);
        // const Color opColor = static_cast<Color>(!color);

        // // figure out piece
        // int pieceNum = 0;
        // for (int i = 0; i < 12; i += 2) {
        //     if (board.curState->pieces[i + color] & fromMask) {
        //         pieceNum = i;
        //     }
        // }
        // pieceNum /= 2;

        // const std::string capture = toMask & board.curState->allPieces[opColor] ? "x" : "";

        // const char file[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

        // int fromFile = 7 - unsigned(bestMove.from()) % 8;

        // int toRank = unsigned(bestMove.to()) / 8;
        // int toFile = 7 - unsigned(bestMove.to()) % 8;

        // const std::string pieceChars[] = {"", "N", "B", "R", "Q", "K"};
        // if (bestMove.moveType() != MoveType::CASTLE) {
        //     pgn << " " << pieceChars[pieceNum] << file[fromFile] << capture << file[toFile] << toRank + 1;
        // } else {
        //     pgn << " " << "O-O";

        //     if (toFile == 0) { // queen side
        //         pgn << "-O";
        //     }
        // }

        // if ((bestMove.moveType() == MoveType::PROMOTION)) {
        //     pgn << "=" << pieceChars[bestMove.promotionPiece() + 1];
        // }

        if (board.curState->blackToMove) {
            moveNum++;
        }

        board.makeMove(bestMove);

        std::cout << "\x1B[2J\x1B[H"; // clear screen
        std::cout << board << std::endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // std::string x;
        // std::cin >> x;

        // std::cout << board.curState->halfMoves << '\n';
        // std::cout << board.curState->highestRepeat << '\n';

        if (board.inCheck()) {
            pgn << "+";
        }
    }

    std::cout << pgn.str() << std::endl;
}