#include "search.h"
#include "board.h"
#include "evaluate.h"
#include "move_list.h"
#include "see.h"
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

constexpr int MATE_BOUND = WIN / 2;  // 5000

int evalToTT(int eval, int ply) {
    if (eval > MATE_BOUND) {
        return eval + ply;  // we mate
    }
    if (eval < -MATE_BOUND) {
        return eval - ply;  // we get mated
    }
    return eval;
}

int evalFromTT(int eval, int ply) {
    if (eval > MATE_BOUND) {
        return eval - ply;
    }
    if (eval < -MATE_BOUND) {
        return eval + ply;
    }
    return eval;
}

std::tuple<Move, int> Searcher::getBestMove(Board& board, int depth) {
    ttable.NewSearch();
    killers = {};
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

std::tuple<Move, int> Searcher::searchDepth(Board& board, int depth, bool collectRootMoves) {
    killers = {};
    collectRoot = collectRootMoves;
    std::tuple<Move, int> result = {Move(0), -INF};
    for (int i = 1; i <= depth; i++) {
        rootMoves.clear();
        result = search(board, i, 0, -INF, INF);
        if (this->stopSearch) {
            break;
        }
    }
    collectRoot = false;
    return result;
}

std::tuple<Move, int> Searcher::iterativeDeepening(Board& board, std::chrono::milliseconds timeMs) {
    ttable.NewSearch();
    killers = {};

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

    if (pv[0].isNull()) {
        return result;
    }

    board.makeMove(pv[0]);
    int count = 1;

    std::vector<uint64_t> pvHashes;

    while (this->ttable.contains(board.hash()) /*&& this->ttable.get(board.hash()).flag == TTEntry::EXACT*/) {
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
        return {Move(0), 0};  // value unused
    }

    if (board.getRepeats(board.hash()) == 3) {
        return {bestMove, DRAW};
    }

    // check transposition table for already computed position
    int            originalAlpha = alpha;
    const uint64_t curHash       = board.hash();
    Move           bestMoveTT;
    assert(curHash == Zobrist::zhash(board));
    if (!(collectRoot && ply == 0) && ttable.contains(curHash)) {
        TTData entry = ttable.get(curHash);
        bestMoveTT   = entry.move;

        int eval = evalFromTT(entry.eval, ply);

        if (entry.depth >= depth) {
            switch (entry.flag) {
            case TTEntry::EXACT:
                return {entry.move, eval};
            case TTEntry::UPPER:
                beta = std::min(beta, eval);
                break;
            case TTEntry::LOWER:
                alpha = std::max(alpha, eval);
                break;
            }

            if (alpha >= beta) {
                return {entry.move, eval};
            }
        }
    }

    // depth limit reached, return evaluation
    if (depth <= 0) {
        bestEval = quiesce(board, alpha, beta, ply);

        return {bestMove, bestEval};
    }

    Move killer1{};
    Move killer2{};
    if (ply < kMaxSearchPly) {
        killer1 = killers[ply][0];
        killer2 = killers[ply][1];
    }

    MoveList<ALL> moveList(board);
    moveList.sort(board, bestMoveTT, killer1, killer2);

    // no moves means we are either in checkmate or a stalemate
    if (moveList.size() == 0) {
        if (board.inCheck()) {
            bestEval = -WIN + ply;

            return {bestMove, bestEval};
        }

        return {bestMove, 0};
    }

    bool first = true;
    for (const auto& move : moveList) {
        int curEval;
        board.makeMove(move);
        uint64_t moveHash = board.hash();

        // ensure we don't grab a stale value from the ttable
        if (board.getRepeats(board.hash()) == 2) {
            curEval = DRAW;
        } else if (collectRoot && ply == 0) {
            curEval = -std::get<1>(Searcher::search(board, depth - 1, ply + 1, -INF, INF));
        } else if (first) {
            curEval = -std::get<1>(Searcher::search(board, depth - 1, ply + 1, -beta, -alpha));
            first   = false;
        } else {
            curEval = -std::get<1>(Searcher::search(board, depth - 1, ply + 1, -alpha - 1, -alpha));

            if (alpha < curEval && curEval < beta) {
                curEval = -std::get<1>(Searcher::search(board, depth - 1, ply + 1, -beta, -alpha));
            }
        }

        board.undoMove();

        if (this->stopSearch) {
            break;  // do not update bestEval / alpha with this child
        }

        if constexpr (DEBUG) {
            if (ply == 0) {
                std::cout << move << " " << curEval << " hash: " << moveHash << '\n';
            }
        }

        if (collectRoot && ply == 0) {
            rootMoves.push_back({move, curEval});
        }

        if (curEval > bestEval) {
            bestMove = move;
            bestEval = curEval;
        }

        alpha = std::max(alpha, curEval);
        if (!(collectRoot && ply == 0) && alpha >= beta) {
            if (ply < kMaxSearchPly && !See::isTactical(board, move)) {
                if (killers[ply][0] != move) {
                    killers[ply][1] = killers[ply][0];
                    killers[ply][0] = move;
                }
            }
            break;
        }
    }

    if (this->stopSearch) {
        return {bestMove, bestEval};
    }

    // update transposition table with new values
    TTEntry entry;
    entry.eval = evalToTT(bestEval, ply);
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

int Searcher::quiesce(Board& board, int alpha, int beta, int ply) {
    const bool inCheck = board.inCheck();

    // No stand-pat while in check: the position is illegal to evaluate, and quiet
    // evasions are required. Otherwise the side to move may decline every capture.
    int best = -INF;
    if (!inCheck) {
        const int perspective = board.blackToMove() ? -1 : 1;
        best                  = evaluation(board) * perspective;
        if (best >= beta) {
            return best;
        }
        if (best > alpha) {
            alpha = best;
        }
    }

    Move killer1{};
    Move killer2{};
    if (ply < kMaxSearchPly) {
        killer1 = killers[ply][0];
        killer2 = killers[ply][1];
    }

    auto searchMoves = [&](auto& moveList) -> int {
        if (inCheck && moveList.size() == 0) {
            return -WIN + ply;
        }

        moveList.sort(board, Move(0), killer1, killer2);

        for (std::size_t i = 0; i < moveList.size(); ++i) {
            // Losing captures stay in the list so checks can try them, but a quiet
            // node can stop once scores drop below zero.
            if (!inCheck && moveList.scoreOf(i) < 0) {
                break;
            }

            const Move move = moveList.get(i);
            if (move.moveType() == MoveType::PROMOTION && move.promotionPiece() != PromoPiece::QUEEN) {
                continue;
            }

            board.makeMove(move);
            const int eval = -quiesce(board, -beta, -alpha, ply + 1);
            board.undoMove();

            if (eval > best) {
                best = eval;
            }
            if (best >= beta) {
                return best;
            }
            if (best > alpha) {
                alpha = best;
            }
        }

        return best;
    };

    if (inCheck) {
        MoveList<ALL> moveList(board);
        return searchMoves(moveList);
    }

    MoveList<CAPTURES> moveList(board);
    return searchMoves(moveList);
}
