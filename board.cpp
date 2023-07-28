#include "board.h"
#include "move.h"
#include "helpers.h"

#include <immintrin.h>

using namespace std;

#define SquareOf(X) _tzcnt_u64(X)
#define Bitloop(X) for(;X; X = _blsr_u64(X))
// Bitloop(bishops) {
//      const Square sq = SquareOf(bishops);
//      ...
// }

Board::Board() {
    setStartPos();
}

uint64_t Board::getPieceSet(int i) {
    return pieces[i];
}

void Board::setPieceSet(int i, uint64_t num) {
    pieces[i] = num;
}

int Board::getPiecePos(int i) {
    uint64_t temp = pieces[i];
    return _tzcnt_u64(temp);
}

void Board::setStartPos() {
    pieces[wpawns]   = 0x000000000000FF00;
    pieces[bpawns]   = 0x00FF000000000000;

    pieces[wknights] = 0x0000000000000024;
    pieces[bknights] = 0x2400000000000000;

    pieces[wbishops] = 0x0000000000000042;
    pieces[bbishops] = 0x4200000000000000;

    pieces[wrooks]   = 0x0000000000000081;
    pieces[brooks]   = 0x8100000000000000;

    pieces[wqueens]  = 0x0000000000000010;
    pieces[bqueens]  = 0x1000000000000000;

    pieces[wking]    = 0x000000000000008;
    pieces[bking]    = 0x800000000000000;

    updateAllPieces();

    enpassantPos = -1;

    for (int i = 0; i < 4; i++) {
        castle[i] = true;
    }

    blackToMove = false;
}

void Board::updateAllPieces() {
    allPieces[0] = (pieces[0] | 
                    pieces[1] | 
                    pieces[2] | 
                    pieces[3] | 
                    pieces[4] | 
                    pieces[5]);

    allPieces[1] = (pieces[6] | 
                    pieces[7] | 
                    pieces[8] | 
                    pieces[9] | 
                    pieces[10] | 
                    pieces[11]);

    empty = ~(allPieces[0] | allPieces[1]);
}

void Board::makeMove(struct Move move) {
    // make the move (update bitboards) - normal moves
    uint64_t fromMask = uint64_t(1) << move.from;
    uint64_t toMask = uint64_t(1) << move.to;
    pieces[move.piece + move.color*6] &= ~fromMask;
    pieces[move.piece + move.color*6] |= toMask;
    int enemyColor = move.color == 0 ? 1 : 0;

    for (int i = move.color*6; i < move.color*6 + 6; i++) {
        if (pieces[i] & fromMask != 0) { // found piece moved, update my pieces
            pieces[i] &= fromMask; // add updated position
            pieces[i] &= ~toMask; // remove old position
        }
    }

    // check opponent pieces for a capture
    for (int i = enemyColor*6; i < enemyColor*6 + 6; i++) {
        if (pieces[i] & toMask != 0) { // found piece taken, update my pieces
            pieces[i] &= fromMask; // add updated position
            pieces[i] &= ~toMask; // remove old position
        }
    }

    // update castle bitboards

    // update enpassant bitboards

    // update black to move

    // update all pieces
    updateAllPieces();
}

std::ostream& operator << (std::ostream& o, Board& board) {

    // white upper, black lower
    char printPiece[] = {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'};

    o << "    a   b   c   d   e   f   g   h  " << endl;
    o << "  +---+---+---+---+---+---+---+---+" << endl;

    for (int rank = 7; rank >= 0; rank--) {

        o << (rank + 1) << " ";

        for (int file = 0; file < 8; file++) {
            uint64_t mask = uint64_t(1) << ((7 - file) + (8 * rank));
            //cout << std::bitset<64>(mask) << endl;
            bool foundPiece = false;

            for (int piece = 0; piece < 12; piece++) {
                uint64_t currentBB = board.getPieceSet(piece);

                if ((currentBB & mask) != 0) {
                    o << "| " << printPiece[piece] << " ";
                    foundPiece = true;
                    break;
                }

            }

            if (!foundPiece) {
                o << "|   ";
            }

        }

        o << "| " << (rank + 1) << endl;
        o << "  +---+---+---+---+---+---+---+---+" << endl;
    }

    o << "    a   b   c   d   e   f   g   h  ";

    return o;
}