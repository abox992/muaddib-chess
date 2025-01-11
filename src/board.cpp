#include "bit_manip.h"
#include "bitboard.h"
#include "board_state.h"
#include "helpers.h"
#include "move.h"
#include "types.h"
#include "zobrist.h"
#include <cstdint>
#include <cstring>
#include <memory>

Board::Board() {
    curState = nullptr;
    setStartPos();
}

Board::Board(const Board& source) {
    this->curState = std::make_unique<BoardState>(*source.curState);
    if (source.curState->prevState == nullptr) {
        this->curState->prevState = nullptr;
        return;
    }
    auto cur       = this->curState.get();
    auto sourceCur = source.curState->prevState.get();

    while (sourceCur) {
        cur->prevState = std::make_unique<BoardState>(*sourceCur);
        cur            = cur->prevState.get();
        sourceCur      = sourceCur->prevState.get();
    }
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
        case 0: {  // piece positions
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
        case 1: {  // piece to move

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
        case 2: {  // castling

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
        case 3: {  // enpassant
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
        case 4: {  // halfmove clock

            for (int i = 0; i < int(tokens[field].length()); i++) {
                char currentChar = tokens[field][i];

                this->curState->halfMoves = int(currentChar - '0');
            }

            break;
        }
        case 5: {  // full move number

            for (int i = 0; i < int(tokens[field].length()); i++) {
                char currentChar = tokens[field][i];

                this->curState->fullMoves = int(currentChar - '0');
            }

            break;
        }
        }
    }

    this->updateAllPieces();

    this->curState->hash = Zobrist::zhash(*this);
}

void Board::setPieceSet(int i, uint64_t num) { this->curState->pieces[i] = num; }

void Board::setStartPos() {
    this->curState            = std::make_unique<BoardState>();
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
    updatePieceOnSquare();

    // while 0 is a position on the board, it is not possible for enpessant sqaure to be 0, so this fine
    this->curState->enpassantPos = 0;

    for (int i = 0; i < 4; i++) {
        this->curState->canCastle[i] = true;
    }

    this->curState->blackToMove = false;

    this->curState->halfMoves = 0;
    this->curState->fullMoves = 1;

    this->curState->hash = Zobrist::zhash(*this);
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

void Board::updatePieceOnSquare() {
    for (int i = 0; i < 64; i++) {
        curState->pieceOnSquare[i] = NO_PIECE; 
    }

    for (int i = 0; i < 12; i++) {
        uint64_t bitboard = curState->pieces[i];

        while (bitboard) {
            const int currentSquare = tz_count(bitboard);
            pop_lsb(bitboard); 

            curState->pieceOnSquare[currentSquare] = static_cast<PieceType>((i >> 1) << 1);
        }
    }
}

// move is assumed to be legal, undefinded behavior with an illegal/null move
void Board::makeMove(const Move& move) {
    assert(!move.isNull());

    // create a copy of the current state (add to head of list)
    std::unique_ptr<BoardState> newState = std::make_unique<BoardState>(*this->curState);
    newState->prevState                  = std::move(this->curState);
    this->curState                       = std::move(newState);

    const int from = move.from();
    const int to   = move.to();

    // make the move (update bitboards) - normal moves
    uint64_t fromMask = maskForPos(from);
    uint64_t toMask   = maskForPos(to);

    const Color color      = this->blackToMove() ? BLACK : WHITE;
    const Color enemyColor = static_cast<Color>(!color);

    // find what piece we are moving
    PieceType piece = PAWNS;
    for (int i = 0; i < 12; i += 2) {
        if (this->curState->pieces[Bitboard::colorPiece(color, i)] & fromMask) {
            piece = static_cast<PieceType>(i);
            break;
        }
    }

    const int colorPiece = Bitboard::colorPiece(color, piece);

    // update my pieces
    this->curState->pieces[colorPiece] &= ~fromMask;  // remove old position
    this->curState->pieces[colorPiece] |= toMask;     // add new position

    this->curState->hash ^= Zobrist::randomTable[from][colorPiece];
    this->curState->hash ^= Zobrist::randomTable[to][colorPiece];

    // update opponent pieces (if its a capture)
    if (toMask & this->getAll(enemyColor)) {
        for (int i = enemyColor; i < 12; i += 2) {
            if (this->curState->pieces[i] & toMask) {  // found enemy piece taken
                // remove the piece
                this->curState->pieces[i] &= ~toMask;

                // capture, reset halfmoves
                this->curState->halfMoves = 0;

                this->curState->hash ^= Zobrist::randomTable[to][i];

                if (i == Bitboard::colorPiece(enemyColor, PAWNS)) {
                    // if a pawn is captured, check if its the last pawn on the board
                    if ((this->getBB(WHITE, PAWNS) | this->getBB(BLACK, PAWNS)) == 0) {
                        this->curState->hash ^= Zobrist::noPawns;
                    }
                } else if (i == Bitboard::colorPiece(enemyColor, ROOKS)) {
                    // if we captured the enemies rook, they can no longer castle
                    for (auto side : {0 /* king side */, 2 /* queen side */}) {
                        if ((toMask & Bitboard::originalRookSquares[enemyColor + side]) != 0) {
                            if (this->curState->canCastle[enemyColor + side]) {
                                this->curState->canCastle[enemyColor + side] = false;
                                this->curState->hash ^= Zobrist::castling[enemyColor + side];
                            }
                        }
                    }
                }

                break;
            }
        }
    }

    // half moves are incremented on every move
    this->curState->halfMoves++;

    switch (piece) {
    case PAWNS:
        // if pawn move, reset halfmoves
        this->curState->halfMoves = 0;

        // special pawn move handling
        if (move.moveType() == MoveType::EN_PASSANT) {  // enpassant capture
            // pawn was already moved above, just have to get rid of the piece it took
            int capturedPawnPos = this->curState->enpassantPos - Bitboard::pawnPush(color);
            this->curState->pieces[enemyColor] &= ~maskForPos(capturedPawnPos);

            this->curState->hash ^= Zobrist::randomTable[capturedPawnPos][enemyColor];
        } else if (move.moveType() == MoveType::PROMOTION) {
            PieceType promoPiece = static_cast<PieceType>(move.promotionPiece() * 2 + 2);

            // add the new piece
            this->curState->pieces[Bitboard::colorPiece(color, promoPiece)] |= toMask;
            // remove the pawn we added by default
            this->curState->pieces[color] &= ~toMask;

            this->curState->hash ^= Zobrist::randomTable[to][Bitboard::colorPiece(color, promoPiece)];
            this->curState->hash ^= Zobrist::randomTable[to][color];
        }

        // remove old enpassant file from hash
        if (this->curState->enpassantPos) {
            this->curState->hash ^= Zobrist::enpassantFile[Bitboard::fileOf(this->curState->enpassantPos)];
        }
        this->curState->enpassantPos = 0;

        // pawn double push, need to set enpassant pos
        if (abs(to - from) > 9) {
            this->curState->enpassantPos = to - Bitboard::pawnPush(color);

            // update hash with new enpassant file
            this->curState->hash ^= Zobrist::enpassantFile[Bitboard::fileOf(this->curState->enpassantPos)];
        }

        break;
    case ROOKS:
        // remove old enpassant file from hash
        if (this->curState->enpassantPos) {
            this->curState->hash ^= Zobrist::enpassantFile[Bitboard::fileOf(this->curState->enpassantPos)];
        }
        this->curState->enpassantPos = 0;

        // rook move, can no longer castle on that side
        if (this->curState->canCastle[color]) {
            // if king side rook not on original square
            if ((Bitboard::originalRookSquares[color] & this->curState->pieces[colorPiece]) == 0) {
                this->curState->canCastle[color] = false;
                this->curState->hash ^= Zobrist::castling[color];
            }
        }

        if (this->curState->canCastle[color + 2]) {
            if ((Bitboard::originalRookSquares[color + 2] & this->curState->pieces[colorPiece])
                == 0) {  // if queen side...
                this->curState->canCastle[color + 2] = false;
                this->curState->hash ^= Zobrist::castling[color + 2];
            }
        }
        break;
    case KINGS:
        // remove old enpassant file from hash
        if (this->curState->enpassantPos) {
            this->curState->hash ^= Zobrist::enpassantFile[Bitboard::fileOf(this->curState->enpassantPos)];
        }
        this->curState->enpassantPos = 0;

        // update castle bitboards
        if (move.moveType() == MoveType::CASTLE) {
            int pos = Bitboard::rookPosToIndex(to);

            // update king and rook pos
            this->curState->pieces[Bitboard::colorPiece(color, KINGS)] = Bitboard::castledKingSquares[pos];
            this->curState->pieces[Bitboard::colorPiece(color, ROOKS)] |= Bitboard::castledRookSquares[pos];
            this->curState->pieces[Bitboard::colorPiece(color, ROOKS)] &= ~Bitboard::originalRookSquares[pos];

            // undo from earlier
            this->curState->hash ^= Zobrist::randomTable[to][colorPiece];
            // add real pos
            this->curState->hash ^=
              Zobrist::randomTable[tz_count(Bitboard::castledKingSquares[pos])][Bitboard::colorPiece(color, KINGS)];
            // update rook
            this->curState->hash ^=
              Zobrist::randomTable[tz_count(Bitboard::castledRookSquares[pos])][Bitboard::colorPiece(color, ROOKS)];
            this->curState->hash ^=
              Zobrist::randomTable[tz_count(Bitboard::originalRookSquares[pos])][Bitboard::colorPiece(color, ROOKS)];
        }

        // king move, can no longer castle
        if (this->curState->canCastle[color]) {
            this->curState->canCastle[color] = false;
            this->curState->hash ^= Zobrist::castling[color];
        }
        if (this->curState->canCastle[color + 2]) {
            this->curState->canCastle[color + 2] = false;
            this->curState->hash ^= Zobrist::castling[color + 2];
        }
        break;
    default:
        // remove old enpassant file from hash
        if (this->curState->enpassantPos) {
            this->curState->hash ^= Zobrist::enpassantFile[Bitboard::fileOf(this->curState->enpassantPos)];
        }
        this->curState->enpassantPos = 0;
        break;
    }

    // we increment full moves after each black move
    if (color == BLACK) {
        this->curState->fullMoves++;
    }

    // update black to move
    this->curState->blackToMove = enemyColor;
    this->curState->hash ^= Zobrist::randomBlackToMove;

    // update all pieces bitboards
    updateAllPieces();

    updatePieceOnSquare();
}

void Board::undoMove() {

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
