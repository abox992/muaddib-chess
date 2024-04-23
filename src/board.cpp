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

// void BoardState::operator=(const BoardState& other) {
//     std::memcpy(this->pieces, other.pieces, sizeof(uint64_t) * (end(this->pieces)-begin(this->pieces)));
//     std::memcpy(this->pieces, other.allPieces, sizeof(uint64_t) * (end(this->allPieces)-begin(this->allPieces)));
//     std::memcpy(this->pieces, other.canCastle, sizeof(bool) * (end(this->canCastle)-begin(this->canCastle)));
//     this->enpassantPos = other.enpassantPos;
//     this->empty = other.empty;
//     this->blackToMove = other.blackToMove;
//     this->halfMoves = other.halfMoves;
//     this->fullMoves = other.fullMoves;
// }

Board::Board() {
    setStartPos();
}

Board::Board(const Board& copy) { // deep copy contructor
    std::memcpy(this->state.pieces, copy.state.pieces, sizeof(uint64_t) * (end(this->state.pieces)-begin(this->state.pieces)));
    std::memcpy(this->state.allPieces, copy.state.allPieces, sizeof(uint64_t) * (end(this->state.allPieces)-begin(this->state.allPieces)));
    std::memcpy(this->state.canCastle, copy.state.canCastle, sizeof(bool) * (end(this->state.canCastle)-begin(this->state.canCastle)));
    this->state.enpassantPos = copy.state.enpassantPos;
    this->state.empty = copy.state.empty;
    this->state.blackToMove = copy.state.blackToMove;
    this->state.halfMoves = copy.state.halfMoves;
    this->state.fullMoves = copy.state.fullMoves;
}

void Board::setPieceSet(int i, uint64_t num) {
    this->state.pieces[i] = num;
}

void Board::setStartPos() {
    this->state.pieces[ColorPiece::WPAWNS]   = 0x000000000000FF00;
    this->state.pieces[ColorPiece::BPAWNS]   = 0x00FF000000000000;

    this->state.pieces[ColorPiece::WKNIGHTS] = 0x0000000000000042;
    this->state.pieces[ColorPiece::BKNIGHTS] = 0x4200000000000000;

    this->state.pieces[ColorPiece::WBISHOPS] = 0x0000000000000024;
    this->state.pieces[ColorPiece::BBISHOPS] = 0x2400000000000000;

    this->state.pieces[ColorPiece::WROOKS]   = 0x0000000000000081;
    this->state.pieces[ColorPiece::BROOKS]   = 0x8100000000000000;

    this->state.pieces[ColorPiece::WQUEENS]  = 0x0000000000000010;
    this->state.pieces[ColorPiece::BQUEENS]  = 0x1000000000000000;

    this->state.pieces[ColorPiece::WKING]    = 0x000000000000008;
    this->state.pieces[ColorPiece::BKING]    = 0x800000000000000;

    updateAllPieces();

    this->state.enpassantPos = 0; // not possible for enpessant sqaure to be 0, so this fine

    for (int i = 0; i < 4; i++) {
        this->state.canCastle[i] = true;
    }

    this->state.blackToMove = false;

    this->state.halfMoves = 0;
    this->state.fullMoves = 1;

    highestRepeat = 1;
    seenPositions.push_back(zhash(*this));
}

void Board::updateAllPieces() {
    this->state.allPieces[0] = (this->state.pieces[0] | 
                    this->state.pieces[2] | 
                    this->state.pieces[4] | 
                    this->state.pieces[6] | 
                    this->state.pieces[8] | 
                    this->state.pieces[10]);

    this->state.allPieces[1] = (this->state.pieces[1] | 
                    this->state.pieces[3] | 
                    this->state.pieces[5] | 
                    this->state.pieces[7] | 
                    this->state.pieces[9] | 
                    this->state.pieces[11]);

    this->state.empty = ~(this->state.allPieces[0] | this->state.allPieces[1]);
}

void Board::makeMove(const Move& move) {

    // save state
    BoardState prevState;
    std::memcpy(prevState.pieces, this->state.pieces, sizeof(uint64_t) * (end(this->state.pieces)-begin(this->state.pieces)));
    std::memcpy(prevState.allPieces, this->state.allPieces, sizeof(uint64_t) * (end(this->state.allPieces)-begin(this->state.allPieces)));
    std::memcpy(prevState.canCastle, this->state.canCastle, sizeof(bool) * (end(this->state.canCastle)-begin(this->state.canCastle)));
    prevState.enpassantPos = this->state.enpassantPos;
    prevState.empty = this->state.empty;
    prevState.blackToMove = this->state.blackToMove;
    prevState.halfMoves = this->state.halfMoves;
    prevState.fullMoves = this->state.fullMoves;
    stateHistory.push_back(prevState);

    // make the move (update bitboards) - normal moves
    uint64_t fromMask = uint64_t(1) << move.from();
    uint64_t toMask = uint64_t(1) << move.to();

    const Color color = this->state.allPieces[Color::WHITE] & fromMask ? Color::WHITE : Color::BLACK;
    const Color enemyColor = static_cast<Color>(!color);

    Piece piece;
    for (int i = 0; i < 12; i += 2) {
        if (this->state.pieces[i + color] & fromMask) {
            piece = static_cast<Piece>(i);
        }
    }

    this->state.halfMoves++;

    // if pawn move, reset halfmoves
    if (piece == Piece::PAWNS) {
        this->state.halfMoves = 0;
    }

    // black move, increment full move clock
    if (color) {
        this->state.fullMoves++;
    }

    // update my pieces
    this->state.pieces[piece + color] &= ~fromMask; // remove old position
    this->state.pieces[piece + color] |= toMask; // add new position

    // update opponent pieces
    for (int i = enemyColor; i < 12; i+=2) {
        if ((this->state.pieces[i] & toMask) != 0) { // found enemy piece taken
            this->state.pieces[i] &= ~toMask;
            this->state.halfMoves = 0; // capture, reset halfMoves

            // why is this here? changed to const, doesnt make sense that im editting the Move
            //move.capturedPiece = i; // save captured piece
            break;
        }
    }

    // update castle bitboards
    if (move.moveType() == MoveType::CASTLE) {
        //uint64_t tempMoveCastle = move.castle;
        //int pos = squareOf(tempMoveCastle);

        uint64_t castleSide = toMask & this->state.pieces[Piece::ROOKS + color]; // bit mask for rook taken
        assert(castleSide != 0);
        int pos = std::countr_zero(castleSide);
        if (pos == 56) {
            pos = 1;
        } else if (pos == 7) {
            pos = 2;
        } else if (pos == 63) {
            pos = 3;
        }

        this->state.pieces[Piece::KINGS + color] = castleSquares[pos]; // update king pos
        this->state.pieces[Piece::ROOKS + color] |= castleRookSquares[pos]; // add new rook pos
        this->state.pieces[Piece::ROOKS + color] &= ~originalRookSquares[pos]; // remove old rook pos

        this->state.canCastle[color] = false;
        this->state.canCastle[color + 2] = false;
    }

    if (piece == Piece::KINGS) { // king move, can no longer castle
        this->state.canCastle[color] = false;
        this->state.canCastle[color + 2] = false;
    }

    if (piece == Piece::ROOKS) { // rook move, can no longer castle on that side
        if ((originalRookSquares[color] & this->state.pieces[Piece::ROOKS + color]) == 0) { // if king side rook not on original square
            this->state.canCastle[color] = false;
        }

        if ((originalRookSquares[color + 2] & this->state.pieces[Piece::ROOKS + color]) == 0) { // if queen side...
            this->state.canCastle[color + 2] = false;
        }
    }

    // if we captured the enemies rook, they can no longer castle
    if ((toMask & originalRookSquares[enemyColor]) != 0) {
        this->state.canCastle[enemyColor] = false;
    }
    if ((toMask & originalRookSquares[enemyColor + 2]) != 0) {
        this->state.canCastle[enemyColor + 2] = false;
    }

    // update enpassant bitboards
    if (move.moveType() == MoveType::EN_PASSANT) { // enpassant move
        // pawn was already moved above, just have to get rid of the piece it took
        if (color == Color::BLACK) { // black
            this->state.pieces[enemyColor] &= ~maskForPos(this->state.enpassantPos + 8);
        } else { // white
            this->state.pieces[enemyColor] &= ~maskForPos(this->state.enpassantPos - 8);
        }
    }
    this->state.enpassantPos = 0;

    if (piece == 0 && (abs(move.to() - move.from()) > 9)) { // pawn push move, need to set enpassant pos
        if (color) { // black
            this->state.enpassantPos = move.to() + 8;
        } else { // white
            this->state.enpassantPos = move.to() - 8;
        }
    }

    // promotion
    if (move.moveType() == MoveType::PROMOTION) {
        //uint64_t tempMovePromo = move.promotion;
        int index = move.promotionPiece();
        this->state.pieces[promoPieces[index] + color] |= toMask; // add new piece
        this->state.pieces[color] &= ~toMask; // remove old pawn

    }

    // update black to move
    this->state.blackToMove = enemyColor;

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
    BoardState prevState = stateHistory.back();

    // load state
    std::memcpy(this->state.pieces, prevState.pieces, sizeof(uint64_t) * (end(this->state.pieces)-begin(this->state.pieces)));
    std::memcpy(this->state.allPieces, prevState.allPieces, sizeof(uint64_t) * (end(this->state.allPieces)-begin(this->state.allPieces)));
    std::memcpy(this->state.canCastle, prevState.canCastle, sizeof(bool) * (end(this->state.canCastle)-begin(this->state.canCastle)));
    this->state.enpassantPos = prevState.enpassantPos;
    this->state.empty = prevState.empty;
    this->state.blackToMove = prevState.blackToMove;
    this->state.halfMoves = prevState.halfMoves;
    this->state.fullMoves = prevState.fullMoves;

    stateHistory.pop_back();
    seenPositions.pop_back();
    highestRepeat--; // this is dubious i dont trust this

}

bool Board::inCheck() const {
    // int color = this->state.blackToMove ? 1 : 0;
    // int enemyColor = color == 0 ? 1 : 0;

    // int kingPos = squareOf(this->state.pieces[Piece::KINGS + color]);//getPiecePos(10 + color);

    // uint64_t opPawns, opKnights, opRQ, opBQ;
    // opPawns = this->state.pieces[0 + enemyColor];
    // opKnights = this->state.pieces[2 + enemyColor];
    // opRQ = opBQ = this->state.pieces[8 + enemyColor];
    // opRQ |= this->state.pieces[6 + enemyColor];
    // opBQ |= this->state.pieces[4 + enemyColor];

    // uint64_t blockers = (~this->state.empty) & rookMasks[kingPos];
    // uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[kingPos]);

    // blockers = (~this->state.empty) & bishopMasks[kingPos];
    // uint64_t bishopCompressedBlockers = extract_bits(blockers, bishopMasks[kingPos]);

    // uint64_t kingAttackers = (pawnAttackMasks[color][kingPos] & opPawns)
    //     | (knightMasks[kingPos] & opKnights)
    //     | (bishopLegalMoves[kingPos][bishopCompressedBlockers] & opBQ)
    //     | (rookLegalMoves[kingPos][rookCompressedBlockers] & opRQ)
    //     ;

    // return (kingAttackers != 0);

    return attacksToKing(*this, this->state.blackToMove);
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
                uint64_t currentBB = board.state.pieces[piece];

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