#include "bit_manip.h"
#include "bitboard.h"
#include "board_state.h"
#include "helpers.h"
#include "move.h"
#include "types.h"
#include "zobrist.h"
#include <cstring>

Board::Board() {
    curState = nullptr;
    setStartPos();
}

Board::Board(const std::string fen) {
    curState = nullptr;
    setStartPos();

    this->set(fen);
}

void Board::set(const std::string fen) {
    this->setStartPos();  // reset the board

    for (int i = 0; i < 12; i++) {  // zero all bitboards
        this->setPieceSet(i, 0);
    }

    std::vector<std::string> tokens = split(fen, ' ');
    for (int field = 0; field < int(tokens.size()); field++) {
        switch (field) {
        case 0 : {  // piece positions
            int currentPos = 63;
            for (int i = 0; i < int(tokens[field].length()); i++) {
                char currentChar = tokens[field][i];

                if (currentChar == '/') {
                    continue;
                }

                if (isdigit(currentChar)) {
                    currentPos -= int(currentChar - '0');
                    continue;
                }

                // update pieces
                char pieceChars[] = {'P', 'p', 'N', 'n', 'B', 'b', 'R', 'r', 'Q', 'q', 'K', 'k'};
                for (int j = 0; j < 12; j++) {
                    if (currentChar == pieceChars[j]) {
                        this->setPieceSet(j, this->curState->pieces[j] | (uint64_t(1) << currentPos));
                    }
                }

                currentPos--;
            }

            break;
        }
        case 1 : {  // piece to move

            for (int i = 0; i < int(tokens[field].length()); i++) {
                char currentChar = tokens[field][i];

                if (currentChar == 'w') {
                    this->curState->blackToMove = false;
                } else {
                    this->curState->blackToMove = true;
                }
            }

            break;
        }
        case 2 : {  // castling

            for (int i = 0; i < 4; i++) {
                this->curState->canCastle[i] = false;
            }

            for (int i = 0; i < int(tokens[field].length()); i++) {
                char currentChar = tokens[field][i];

                if (currentChar == '-') {
                    break;
                }

                if (currentChar == 'K') {
                    this->curState->canCastle[0] = true;
                } else if (currentChar == 'Q') {
                    this->curState->canCastle[2] = true;
                } else if (currentChar == 'k') {
                    this->curState->canCastle[1] = true;
                } else if (currentChar == 'q') {
                    this->curState->canCastle[3] = true;
                }
            }

            break;
        }
        case 3 : {  // enpassant
            int pos = 0;
            for (int i = 0; i < int(tokens[field].length()); i++) {
                char currentChar = tokens[field][i];

                if (currentChar == '-') {
                    this->curState->enpassantPos = 0;
                    break;
                }

                if (i == 0) {
                    pos += 'h' - currentChar;
                }

                if (i == 1) {
                    pos += 8 * ((currentChar - '0') - 1);
                    this->curState->enpassantPos = pos;
                }
            }

            break;
        }
        case 4 : {  // halfmove clock

            for (int i = 0; i < int(tokens[field].length()); i++) {
                char currentChar = tokens[field][i];

                this->curState->halfMoves = int(currentChar - '0');
            }

            break;
        }
        case 5 : {  // full move number

            for (int i = 0; i < int(tokens[field].length()); i++) {
                char currentChar = tokens[field][i];

                this->curState->fullMoves = int(currentChar - '0');
            }

            break;
        }
        }
    }

    this->updateAllPieces();

    /*this->curState->highestRepeat = 1;*/
    this->curState->hash = Zobrist::zhash(*this->curState);
}

void Board::setPieceSet(int i, uint64_t num) { this->curState->pieces[i] = num; }

void Board::setStartPos() {
    this->curState            = std::make_unique<BoardState>();  // new BoardState();
    this->curState->prevState = nullptr;

    this->curState->pieces[PAWNS]       = 0x000000000000FF00;
    this->curState->pieces[PAWNS + 1]   = 0x00FF000000000000;
    this->curState->pieces[KNIGHTS]     = 0x0000000000000042;
    this->curState->pieces[KNIGHTS + 1] = 0x4200000000000000;
    this->curState->pieces[BISHOPS]     = 0x0000000000000024;
    this->curState->pieces[BISHOPS + 1] = 0x2400000000000000;
    this->curState->pieces[ROOKS]       = 0x0000000000000081;
    this->curState->pieces[ROOKS + 1]   = 0x8100000000000000;
    this->curState->pieces[QUEENS]      = 0x0000000000000010;
    this->curState->pieces[QUEENS + 1]  = 0x1000000000000000;
    this->curState->pieces[KINGS]       = 0x000000000000008;
    this->curState->pieces[KINGS + 1]   = 0x800000000000000;

    updateAllPieces();

    this->curState->enpassantPos = 0;  // not possible for enpessant sqaure to be 0, so this fine

    for (int i = 0; i < 4; i++) {
        this->curState->canCastle[i] = true;
    }

    this->curState->blackToMove = false;

    this->curState->halfMoves = 0;
    this->curState->fullMoves = 1;

    /*this->curState->highestRepeat = 1;*/
    this->curState->hash = Zobrist::zhash(*this->curState);
}

void Board::updateAllPieces() {
    curState->allPieces[0] = 0;
    curState->allPieces[1] = 0;
    for (int i = 0; i < 12; i += 2) {
        curState->allPieces[0] |= curState->pieces[i];
        curState->allPieces[1] |= curState->pieces[i + 1];
    }

    this->curState->empty = ~(this->curState->allPieces[0] | this->curState->allPieces[1]);
}

void Board::makeMove(const Move& move) {

    // create a copy of the current state (add to head of list)
    std::unique_ptr<BoardState> newState = std::make_unique<BoardState>(*this->curState);
    newState->prevState                  = std::move(this->curState);
    this->curState                       = std::move(newState);

    const int from = move.from();
    const int to   = move.to();

    // make the move (update bitboards) - normal moves
    uint64_t fromMask = maskForPos(from);
    uint64_t toMask   = maskForPos(to);

    const Color color      = this->curState->allPieces[Color::WHITE] & fromMask ? Color::WHITE : Color::BLACK;
    const Color enemyColor = static_cast<Color>(!color);

    PieceType piece = PAWNS;
    for (int i = 0; i < 12; i += 2) {
        if (this->curState->pieces[i + static_cast<int>(color)] & fromMask) {
            piece = static_cast<PieceType>(i);
            break;
        }
    }

    int colorPiece = static_cast<int>(piece + static_cast<int>(color));

    // update hash
    this->curState->hash ^= Zobrist::randomBlackToMove;
    this->curState->hash ^= Zobrist::randomTable[from][colorPiece];

    if (move.moveType() == MoveType::NORMAL) {
        this->curState->hash ^= Zobrist::randomTable[to][colorPiece];
    }

    this->curState->halfMoves++;

    // if pawn move, reset halfmoves
    if (piece == PAWNS) {
        this->curState->halfMoves = 0;
    }

    // black move, increment full move clock
    if (color) {
        this->curState->fullMoves++;
    }

    // update my pieces
    this->curState->pieces[colorPiece] &= ~fromMask;  // remove old position
    this->curState->pieces[colorPiece] |= toMask;     // add new position

    // update opponent pieces
    for (int i = enemyColor; i < 12; i += 2) {
        if ((this->curState->pieces[i] & toMask) != 0) {  // found enemy piece taken
            this->curState->pieces[i] &= ~toMask;
            this->curState->halfMoves = 0;  // capture, reset halfMoves

            // update hash for normal capture
            this->curState->hash ^= Zobrist::randomTable[to][i];  // remove piece taken
            break;
        }
    }

    // update castle bitboards
    if (move.moveType() == MoveType::CASTLE) {
        int pos = to;
        if (pos == 56) {
            pos = 1;
        } else if (pos == 7) {
            pos = 2;
        } else if (pos == 63) {
            pos = 3;
        }

        this->curState->pieces[KINGS + static_cast<int>(color)] = Bitboard::castleSquares[pos];  // update king pos
        this->curState->pieces[ROOKS + static_cast<int>(color)] |=
          Bitboard::castleRookSquares[pos];  // add new rook pos
        this->curState->pieces[ROOKS + static_cast<int>(color)] &=
          ~Bitboard::originalRookSquares[pos];  // remove old rook pos

        this->curState->canCastle[color]     = false;
        this->curState->canCastle[color + 2] = false;

        // update hash
        this->curState->hash ^= Zobrist::randomTable[to][ROOKS + static_cast<int>(color)];  // remove old rook pos
        this->curState->hash ^= Zobrist::randomTable[tz_count(Bitboard::castleRookSquares[pos])]
                                                    [ROOKS + static_cast<int>(color)];  // add new rook pos
        this->curState->hash ^=
          Zobrist::randomTable[this->kingPos(color)][KINGS + static_cast<int>(color)];  // add new king pos
    }

    if (piece == KINGS) {  // king move, can no longer castle
        this->curState->canCastle[color]     = false;
        this->curState->canCastle[color + 2] = false;
    }

    if (piece == ROOKS) {  // rook move, can no longer castle on that side
        if ((Bitboard::originalRookSquares[color] & this->curState->pieces[colorPiece])
            == 0) {  // if king side rook not on original square
            this->curState->canCastle[color] = false;
        }

        if ((Bitboard::originalRookSquares[color + 2] & this->curState->pieces[colorPiece]) == 0) {  // if queen side...
            this->curState->canCastle[color + 2] = false;
        }
    }

    // if we captured the enemies rook, they can no longer castle
    if ((toMask & Bitboard::originalRookSquares[enemyColor]) != 0) {
        this->curState->canCastle[enemyColor] = false;
    }
    if ((toMask & Bitboard::originalRookSquares[enemyColor + 2]) != 0) {
        this->curState->canCastle[enemyColor + 2] = false;
    }

    // update enpassant bitboards
    if (move.moveType() == MoveType::EN_PASSANT) {  // enpassant move
        // pawn was already moved above, just have to get rid of the piece it took
        int capturedPawnPos = this->curState->enpassantPos - Bitboard::pawnPush(color);
        this->curState->pieces[enemyColor] &= ~maskForPos(capturedPawnPos);

        // update hash
        this->curState->hash ^= Zobrist::randomTable[to][colorPiece];  // add new pawn
        this->curState->hash ^= Zobrist::randomTable[capturedPawnPos][PAWNS + static_cast<int>(enemyColor)];
    }

    this->curState->enpassantPos = 0;

    if (piece == PAWNS && (abs(to - from) > 9)) {  // pawn push move, need to set enpassant pos
        if (color) {                               // black
            this->curState->enpassantPos = to + 8;
        } else {  // white
            this->curState->enpassantPos = to - 8;
        }
    }

    // promotion
    if (move.moveType() == MoveType::PROMOTION) {
        const int promoPieces[4] = {2, 4, 6, 8};
        int       index          = move.promotionPiece();
        this->curState->pieces[promoPieces[index] + static_cast<int>(color)] |= toMask;  // add new piece
        this->curState->pieces[color] &= ~toMask;                                        // remove old pawn

        // update hash
        this->curState->hash ^=
          Zobrist::randomTable[to][promoPieces[index] + static_cast<int>(color)];  // add new piece
    }

    // update black to move
    this->curState->blackToMove = enemyColor;

    // update all pieces
    updateAllPieces();
}

void Board::unmakeMove() {

    if (this->curState == nullptr || this->curState->prevState == nullptr) {
        return;
    }

    this->curState = std::move(this->curState->prevState);
}

int Board::getRepeats(uint64_t hash) const {
    int count = 0;
    for (BoardState* temp = this->curState.get(); temp != nullptr; temp = temp->prevState.get()) {
        if (temp->hash == hash) {
            count++;
        }
    }

    return count;
}

bool Board::inCheck() const {
    if (this->curState->blackToMove) {
        return Bitboard::attacksToKing<Color::BLACK, false>(*this);
    }

    return Bitboard::attacksToKing<Color::WHITE, false>(*this);
}

std::ostream& operator<<(std::ostream& o, Board& board) {

    // white upper, black lower
    char printPiece[] = {'P', 'p', 'N', 'n', 'B', 'b', 'R', 'r', 'Q', 'q', 'K', 'k'};
    // string printPiece[] = {"\u2659", "\u265F", "\u2658", "\u265E", "\u2657", "\u265D", "\u2656", "\u265C", "\u2655", "\u265B", "\u2654", "\u265A"};

    o << "    a   b   c   d   e   f   g   h  " << std::endl;
    o << "  +---+---+---+---+---+---+---+---+" << std::endl;

    for (int rank = 7; rank >= 0; rank--) {

        o << (rank + 1) << " ";

        for (int file = 0; file < 8; file++) {
            uint64_t mask = uint64_t(1) << ((7 - file) + (8 * rank));
            // cout << std::bitset<64>(mask) << endl;
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

        o << "| " << (rank + 1) << std::endl;
        o << "  +---+---+---+---+---+---+---+---+" << std::endl;
    }

    o << "    a   b   c   d   e   f   g   h  ";

    return o;
}
