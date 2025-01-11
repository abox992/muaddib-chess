#include "search.h"
#include "board.h"
#include "evaluate.h"
#include "move_list.h"
#include "transpose_table.h"
#include "zobrist.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <thread>

#define DEBUG 0

#define INF (20000)
#define WIN (10000)
// -1 because if it has to choose between an even position and drawing,
// it should choose the even position. (I want play to continue)
#define DRAW (-1)

std::tuple<Move, int> Searcher::getBestMove(Board& board, int depth) {
    ttable.NewSearch();
    for (int i = 1; i < depth; i++) {
        search(board, i, 0, -INF, INF);
    }
    auto result = search(board, depth, 0, -INF, INF);

    pv.clear();
    pv.push_back(std::get<0>(result));

    board.makeMove(pv[0]);
    int count = 1;

    std::vector<uint64_t> pvHashes;

    while (this->ttable.contains(board.hash()) && this->ttable.get(board.hash()).flag == TTEntry::EXACT) {
        std::cout << "found pv move\n";
        auto entry = this->ttable.get(board.hash());
        if (entry.move.isNull() || std::find(pvHashes.begin(), pvHashes.end(), board.hash()) != pvHashes.end()) {
            break;
        }
        pv.push_back(entry.move);
        pvHashes.push_back(board.hash());
        count++;
        board.makeMove(entry.move);
    }

    for (int i = 0; i < count; i++) {
        board.undoMove();
    }

    for (const auto& m : pv) {
        std::cout << m << ' ';
    }
    std::cout << std::endl;

    std::cout << "size: " << ttable.getSize() << std::endl;

    return result;
}

std::tuple<Move, int> Searcher::iterativeDeepening(Board& board, std::chrono::milliseconds timeMs) {
    ttable.NewSearch();

    std::tuple<Move, int> result;
    result = {Move(0), -INF};

    std::thread timer([this, &result, &board] {
        for (int depth = 1; depth < 256; depth++) {
            auto newResult = search(board, depth, 0, -INF, INF);
            if (!this->stopSearch) {
                result = newResult;
            } else {
                break;
            }
        }
    });

    std::this_thread::sleep_for(timeMs);

    this->stopSearch = true;
    timer.join();
    this->stopSearch = false;

    // not handling (unlikely) possibility of stopping search before even depth 1 finishes

    pv.clear();
    pv.push_back(std::get<0>(result));

    if (pv[0].isNull()) return result;

    board.makeMove(pv[0]);
    int count = 1;

    std::vector<uint64_t> pvHashes;

    while (this->ttable.contains(board.hash()) && this->ttable.get(board.hash()).flag == TTEntry::EXACT) {
        std::cout << "found pv move\n";
        auto entry = this->ttable.get(board.hash());
        if (entry.move.isNull() || std::find(pvHashes.begin(), pvHashes.end(), board.hash()) != pvHashes.end()) {
            break;
        }
        pv.push_back(entry.move);
        pvHashes.push_back(board.hash());
        count++;
        board.makeMove(entry.move);
    }

    for (int i = 0; i < count; i++) {
        board.undoMove();
    }

    for (const auto& m : pv) {
        std::cout << m << ' ';
    }
    std::cout << std::endl;

    std::cout << "size: " << ttable.getSize() << std::endl;

    return result;
}

std::tuple<Move, int> Searcher::search(Board& board, const int depth, const int ply, int alpha, int beta) {
    Move bestMove = Move(0);
    int  bestEval = -INF;

    if (this->stopSearch) {
        return {bestMove, bestEval};
    }

    MoveList<ALL> moveList(board);

    moveList.sort(board, pv, ttable);

    // no moves means we are either in checkmate or a stalemate
    if (moveList.size() == 0) {
        if (board.inCheck()) {
            bestEval = -WIN + ply;

            return {bestMove, bestEval};
        }

        return {bestMove, 0};
    }

    // depth limit reached, return evaluation
    if (depth <= 0) {
        bestEval = quiesce(board, alpha, beta);

        return {bestMove, bestEval};
    }

    if (board.getRepeats(board.hash()) == 3) {
        return {bestMove, DRAW};
    }

    // check transposition table for already computed position
    int            originalAlpha = alpha;
    const uint64_t curHash       = board.hash();
    assert(curHash == Zobrist::zhash(board));
    if (ttable.contains(curHash)) {
        TTData entry = ttable.get(curHash);
        
        if (entry.depth >= depth) {
            switch (entry.flag) {
            case TTEntry::EXACT :
                return {entry.move, entry.eval};
            case TTEntry::UPPER :
                beta = std::min(beta, entry.eval);
                break;
            case TTEntry::LOWER :
                alpha = std::max(alpha, entry.eval);
                break;
            }

            if (alpha >= beta) {
                return {entry.move, entry.eval};
            }
        }
    }

    bool first = true;
    for (const auto& move : moveList) {
        int curEval;
        board.makeMove(move);
        uint64_t moveHash = board.hash();

        // ensure we don't grab a stale value from the ttable
        if (board.getRepeats(board.hash()) == 2) {
            curEval = DRAW;
        } else {

            // Principal variation search
            if (first) {
                curEval = -std::get<1>(Searcher::search(board, depth - 1, ply + 1, -beta, -alpha));
                first   = false;
            } else {
                curEval = -std::get<1>(Searcher::search(board, depth - 1, ply + 1, -alpha - 1, -alpha));

                if (alpha < curEval && curEval < beta) {
                    curEval = -std::get<1>(Searcher::search(board, depth - 1, ply + 1, -beta, -alpha));
                }
            }
        }

        board.undoMove();

        if constexpr (DEBUG) {
            if (ply == 0) {
                std::cout << move << " " << curEval << " hash: " << moveHash << '\n';
            }
        }

        if (curEval > bestEval) {
            bestMove = move;
            bestEval = curEval;
        }

        alpha = std::max(alpha, curEval);
        if (alpha >= beta) {
            break;
        }
    }

    // update transposition table with new values
    TTEntry entry;
    entry.eval = bestEval;
    entry.move = bestMove;

    // set flag
    if (bestEval <= originalAlpha) {
        entry.flag = TTEntry::UPPER;
    } else if (bestEval >= beta) {
        entry.flag = TTEntry::LOWER;
    } else {
        entry.flag = TTEntry::EXACT;
    }

    // set depth
    entry.depth = depth;

    ttable.save(curHash, entry);

    return {bestMove, bestEval};
}

int Searcher::quiesce(Board& board, int alpha, int beta) {
    const int perspective = board.blackToMove() ? -1 : 1;

    // from searcher's perspective, negative is bad and positive is good
    int standPat = evaluation(board) * perspective;

    if (standPat >= beta) {
        return beta;
    }

    if (standPat > alpha) {
        alpha = standPat;
    }

    MoveList<CAPTURES> moveList(board);

    for (auto& move : moveList) {
        board.makeMove(move);
        int eval = -quiesce(board, -beta, -alpha);
        board.undoMove();

        if (eval >= beta) {
            return beta;
        }

        if (eval > alpha) {
            alpha = eval;
        }
    }

    return alpha;
}
