#include "board.h"
#include "move.h"
#include "helpers.h"
#include "precompute_masks.h"

#include <immintrin.h>
#include <cstring>

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

Board::Board(const Board& copy) { // deep copy contructor
    std::memcpy(pieces, copy.pieces, sizeof(uint64_t) * (end(pieces)-begin(pieces)));
    std::memcpy(allPieces, copy.allPieces, sizeof(uint64_t) * (end(allPieces)-begin(allPieces)));
    std::memcpy(canCastle, copy.canCastle, sizeof(bool) * (end(canCastle)-begin(canCastle)));
    enpassantPos = copy.enpassantPos;
    empty = copy.empty;
    blackToMove = copy.blackToMove;
}

uint64_t Board::getPieceSet(int i) {
    return pieces[i];
}

void Board::setPieceSet(int i, uint64_t num) {
    pieces[i] = num;
}

// only use this on king/queen
int Board::getPiecePos(int i) {
    uint64_t temp = pieces[i];
    return _tzcnt_u64(temp);
}

void Board::setStartPos() {
    pieces[wpawns]   = 0x000000000000FF00;
    pieces[bpawns]   = 0x00FF000000000000;

    pieces[wknights] = 0x0000000000000042;
    pieces[bknights] = 0x4200000000000000;

    pieces[wbishops] = 0x0000000000000024;
    pieces[bbishops] = 0x2400000000000000;

    pieces[wrooks]   = 0x0000000000000081;
    pieces[brooks]   = 0x8100000000000000;

    pieces[wqueens]  = 0x0000000000000010;
    pieces[bqueens]  = 0x1000000000000000;

    pieces[wking]    = 0x000000000000008;
    pieces[bking]    = 0x800000000000000;

    updateAllPieces();

    enpassantPos = 0; // not possible for enpessant sqaure to be 0, so this fine

    for (int i = 0; i < 4; i++) {
        canCastle[i] = true;
    }

    blackToMove = false;
}

void Board::updateAllPieces() {
    allPieces[0] = (pieces[0] | 
                    pieces[2] | 
                    pieces[4] | 
                    pieces[6] | 
                    pieces[8] | 
                    pieces[10]);

    allPieces[1] = (pieces[1] | 
                    pieces[3] | 
                    pieces[5] | 
                    pieces[7] | 
                    pieces[9] | 
                    pieces[11]);

    empty = ~(allPieces[0] | allPieces[1]);
}

void Board::makeMove(struct Move move) {
    // make the move (update bitboards) - normal moves
    uint64_t fromMask = uint64_t(1) << move.from;
    uint64_t toMask = uint64_t(1) << move.to;
    int enemyColor = move.color == 0 ? 1 : 0;

    // update my pieces
    pieces[move.piece + move.color] &= ~fromMask; // remove old position
    pieces[move.piece + move.color] |= toMask; // add new position

    // update opponent pieces
    for (int i = enemyColor; i < 12; i+=2) {
        if ((pieces[i] & toMask) != 0) { // found enemy piece taken
            pieces[i] &= ~toMask;
        }
    }

    // update castle bitboards
    if (move.castle != 0) {
        uint64_t tempMoveCastle = move.castle;
        int pos = _tzcnt_u64(tempMoveCastle); // note castle is 0 after this is called
        pieces[10 + move.color] = castleSquares[pos]; // update king pos
        pieces[6 + move.color] |= castleRookSquares[pos]; // add new rook pos
        pieces[6 + move.color] &= ~originalRookSquares[pos]; // remove old rook pos

        canCastle[move.color] = false;
        canCastle[move.color + 2] = false;
    }

    if (move.piece == 10) { // king move, can no longer castle
        canCastle[move.color] = false;
        canCastle[move.color + 2] = false;
    }

    if (move.piece == 6) { // rook move, can no longer castle on that side
        if ((originalRookSquares[move.color] & pieces[6 + move.color]) == 0) { // if king side rook not on original square
            canCastle[move.color] = false;
        }

        if ((originalRookSquares[move.color + 2] & pieces[6 + move.color]) == 0) { // if queen side...
            canCastle[move.color + 2] = false;
        }
    }

    // if we captured the enemies rook, they can no longer castle
    if ((toMask & originalRookSquares[enemyColor]) != 0) {
        canCastle[enemyColor] = false;
    }
    if ((toMask & originalRookSquares[enemyColor + 2]) != 0) {
        canCastle[enemyColor + 2] = false;
    }

    // update enpassant bitboards
    if (move.enpessant) { // enpassant move
        // pawn was already moved above, just have to get rid of the piece it took
        if (move.color) { // black
            pieces[enemyColor] &= ~MaskForPos(enpassantPos + 8);
        } else { // white
            pieces[enemyColor] &= ~MaskForPos(enpassantPos - 8);
        }
    }
    enpassantPos = 0;

    if (move.piece == 0 && (abs(move.to - move.from) > 9)) { // pawn push move, need to set enpassant pos
        if (move.color) { // black
            enpassantPos = move.to + 8;
        } else { // white
            enpassantPos = move.to - 8;
        }
    }

    // promotion
    if (move.promotion != 0) {
        uint64_t tempMovePromo = move.promotion;
        int index = _tzcnt_u64(tempMovePromo);
        pieces[promoPieces[index] + move.color] |= toMask; // add new piece
        pieces[move.color] &= ~toMask; // remove old pawn

    }

    // update black to move
    blackToMove = enemyColor;

    // update all pieces
    updateAllPieces();
}

std::ostream& operator << (std::ostream& o, Board& board) {

    // white upper, black lower
    char printPiece[] = {'P', 'p', 'N', 'n', 'B', 'b', 'R', 'r', 'Q', 'q', 'K', 'k'};
    //string printPiece[] = {"\u2659", "\u265F", "\u2658", "\u265E", "\u2657", "\u265D", "\u2656", "\u265C", "\u2655", "\u265B", "\u2654", "\u265A"};

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