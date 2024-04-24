#include "board.h"
#include "move.h"
#include "helpers.h"
#include "precompute_masks.h"
#include "constants.h"
#include "bit_manip.h"
#include "zobrist.h"
#include "check_pin_masks.h"

#include <cstring>

using namespace std;

BoardState::BoardState(const BoardState& copy) {
    std::memcpy(this->pieces, copy.pieces, sizeof(uint64_t) * (end(copy.pieces) - begin(copy.pieces)));
    std::memcpy(this->allPieces, copy.allPieces, sizeof(uint64_t) * (end(copy.allPieces) - begin(copy.allPieces)));
    std::memcpy(this->canCastle, copy.canCastle, sizeof(bool) * (end(copy.canCastle) - begin(copy.canCastle)));
    this->enpassantPos = copy.enpassantPos;
    this->empty = copy.empty;
    this->blackToMove = copy.blackToMove;
    this->halfMoves = copy.halfMoves;
    this->fullMoves = copy.fullMoves;

    this->prevState = copy.prevState;
}

Board::Board() {
    curState = nullptr;
    setStartPos();
}

Board::Board(const Board& copy) { // deep copy contructor
    // BoardState newCopy = *(copy.curState);
    // this->curState = &newCopy;
    this->curState = new BoardState(*copy.curState);

    this->seenPositions = copy.seenPositions;
}

void Board::setPieceSet(int i, uint64_t num) {
    this->curState->pieces[i] = num;
}

void Board::setStartPos() {
    //BoardState state;
    this->curState = new BoardState();

    this->curState->pieces[ColorPiece::WPAWNS]   = 0x000000000000FF00;
    this->curState->pieces[ColorPiece::BPAWNS]   = 0x00FF000000000000;

    this->curState->pieces[ColorPiece::WKNIGHTS] = 0x0000000000000042;
    this->curState->pieces[ColorPiece::BKNIGHTS] = 0x4200000000000000;

    this->curState->pieces[ColorPiece::WBISHOPS] = 0x0000000000000024;
    this->curState->pieces[ColorPiece::BBISHOPS] = 0x2400000000000000;

    this->curState->pieces[ColorPiece::WROOKS]   = 0x0000000000000081;
    this->curState->pieces[ColorPiece::BROOKS]   = 0x8100000000000000;

    this->curState->pieces[ColorPiece::WQUEENS]  = 0x0000000000000010;
    this->curState->pieces[ColorPiece::BQUEENS]  = 0x1000000000000000;

    this->curState->pieces[ColorPiece::WKING]    = 0x000000000000008;
    this->curState->pieces[ColorPiece::BKING]    = 0x800000000000000;

    updateAllPieces();

    this->curState->enpassantPos = 0; // not possible for enpessant sqaure to be 0, so this fine

    for (int i = 0; i < 4; i++) {
        this->curState->canCastle[i] = true;
    }

    this->curState->blackToMove = false;

    this->curState->halfMoves = 0;
    this->curState->fullMoves = 1;

    highestRepeat = 1;
    seenPositions.push_back(zhash(*this));
}

void Board::updateAllPieces() {
    this->curState->allPieces[0] = (this->curState->pieces[0] | 
                    this->curState->pieces[2] | 
                    this->curState->pieces[4] | 
                    this->curState->pieces[6] | 
                    this->curState->pieces[8] | 
                    this->curState->pieces[10]);

    this->curState->allPieces[1] = (this->curState->pieces[1] | 
                    this->curState->pieces[3] | 
                    this->curState->pieces[5] | 
                    this->curState->pieces[7] | 
                    this->curState->pieces[9] | 
                    this->curState->pieces[11]);

    this->curState->empty = ~(this->curState->allPieces[0] | this->curState->allPieces[1]);
}

void Board::makeMove(const Move& move) {

    // create a copy of the current state
    BoardState* oldState = this->curState;
    this->curState = new BoardState(*oldState);
    this->curState->prevState = oldState;

    // make the move (update bitboards) - normal moves
    uint64_t fromMask = uint64_t(1) << move.from();
    uint64_t toMask = uint64_t(1) << move.to();

    const Color color = this->curState->allPieces[Color::WHITE] & fromMask ? Color::WHITE : Color::BLACK;
    const Color enemyColor = static_cast<Color>(!color);

    Piece piece;
    for (int i = 0; i < 12; i += 2) {
        if (this->curState->pieces[i + color] & fromMask) {
            piece = static_cast<Piece>(i);
        }
    }

    this->curState->halfMoves++;

    // if pawn move, reset halfmoves
    if (piece == Piece::PAWNS) {
        this->curState->halfMoves = 0;
    }

    // black move, increment full move clock
    if (color) {
        this->curState->fullMoves++;
    }

    // update my pieces
    this->curState->pieces[piece + color] &= ~fromMask; // remove old position
    this->curState->pieces[piece + color] |= toMask; // add new position

    // update opponent pieces
    for (int i = enemyColor; i < 12; i+=2) {
        if ((this->curState->pieces[i] & toMask) != 0) { // found enemy piece taken
            this->curState->pieces[i] &= ~toMask;
            this->curState->halfMoves = 0; // capture, reset halfMoves

            // why is this here? changed to const, doesnt make sense that im editting the Move
            //move.capturedPiece = i; // save captured piece
            break;
        }
    }

    // update castle bitboards
    if (move.moveType() == MoveType::CASTLE) {
        //uint64_t tempMoveCastle = move.castle;
        //int pos = squareOf(tempMoveCastle);

        uint64_t castleSide = toMask & this->curState->pieces[Piece::ROOKS + color]; // bit mask for rook taken
        assert(castleSide != 0);
        int pos = std::countr_zero(castleSide);
        if (pos == 56) {
            pos = 1;
        } else if (pos == 7) {
            pos = 2;
        } else if (pos == 63) {
            pos = 3;
        }

        this->curState->pieces[Piece::KINGS + color] = castleSquares[pos]; // update king pos
        this->curState->pieces[Piece::ROOKS + color] |= castleRookSquares[pos]; // add new rook pos
        this->curState->pieces[Piece::ROOKS + color] &= ~originalRookSquares[pos]; // remove old rook pos

        this->curState->canCastle[color] = false;
        this->curState->canCastle[color + 2] = false;
    }

    if (piece == Piece::KINGS) { // king move, can no longer castle
        this->curState->canCastle[color] = false;
        this->curState->canCastle[color + 2] = false;
    }

    if (piece == Piece::ROOKS) { // rook move, can no longer castle on that side
        if ((originalRookSquares[color] & this->curState->pieces[Piece::ROOKS + color]) == 0) { // if king side rook not on original square
            this->curState->canCastle[color] = false;
        }

        if ((originalRookSquares[color + 2] & this->curState->pieces[Piece::ROOKS + color]) == 0) { // if queen side...
            this->curState->canCastle[color + 2] = false;
        }
    }

    // if we captured the enemies rook, they can no longer castle
    if ((toMask & originalRookSquares[enemyColor]) != 0) {
        this->curState->canCastle[enemyColor] = false;
    }
    if ((toMask & originalRookSquares[enemyColor + 2]) != 0) {
        this->curState->canCastle[enemyColor + 2] = false;
    }

    // update enpassant bitboards
    if (move.moveType() == MoveType::EN_PASSANT) { // enpassant move
        // pawn was already moved above, just have to get rid of the piece it took
        if (color == Color::BLACK) { // black
            this->curState->pieces[enemyColor] &= ~maskForPos(this->curState->enpassantPos + 8);
        } else { // white
            this->curState->pieces[enemyColor] &= ~maskForPos(this->curState->enpassantPos - 8);
        }
    }
    this->curState->enpassantPos = 0;

    if (piece == 0 && (abs(move.to() - move.from()) > 9)) { // pawn push move, need to set enpassant pos
        if (color) { // black
            this->curState->enpassantPos = move.to() + 8;
        } else { // white
            this->curState->enpassantPos = move.to() - 8;
        }
    }

    // promotion
    if (move.moveType() == MoveType::PROMOTION) {
        //uint64_t tempMovePromo = move.promotion;
        int index = move.promotionPiece();
        this->curState->pieces[promoPieces[index] + color] |= toMask; // add new piece
        this->curState->pieces[color] &= ~toMask; // remove old pawn

    }

    // update black to move
    this->curState->blackToMove = enemyColor;

    // update all pieces
    updateAllPieces();

    uint64_t currentHash = zhash(*this);
    seenPositions.push_back(currentHash);

    int count = 0;
    for (auto pos : seenPositions) {
        if (pos == currentHash) {
            count++;
        }
    }

    if (count > highestRepeat) {
        highestRepeat = count;
    }
}

void Board::unmakeMove() {

    // stateHistory.pop_back();
    BoardState* temp = this->curState;
    this->curState = this->curState->prevState;

    delete temp; // make sure we free the old state from memory

    seenPositions.pop_back();
    highestRepeat--; // this is dubious i dont trust this

}

bool Board::inCheck() const {
    return attacksToKing(*this, this->curState->blackToMove);
}

std::ostream& operator<<(std::ostream& o, Board& board) {

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
                uint64_t currentBB = board.curState->pieces[piece];

                if ((currentBB & mask) != 0) {
                    if (piece % 2 == 1) {
                        o << "| " << "\033[1;31m" << printPiece[piece] << "\033[0m" << " ";
                    } else {
                        o << "| " << printPiece[piece] << " ";
                    }
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