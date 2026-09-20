#include "bit_manip.h"
#include "bitboard.h"
#include "board_state.h"
#include "helpers.h"
#include "move.h"
#include "types.h"
#include "zobrist.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

Board::Board() { setStartPos(); }

Board::Board(const Board& source) = default;

Board::Board(const std::string fen) { this->set(fen); }

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
                        this->setPieceSet(j, this->_pieces[j] | (uint64_t(1) << currentPos));
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
                    this->_blackToMove = false;
                } else {
                    this->_blackToMove = true;
                }
            }

            break;
        }
        case 2: {  // castling

            for (int i = 0; i < 4; i++) {
                this->_canCastle[i] = false;
            }

            for (int i = 0; i < int(tokens[field].length()); i++) {
                char currentChar = tokens[field][i];

                if (currentChar == '-') {
                    break;
                }

                if (currentChar == 'K') {
                    this->_canCastle[0] = true;
                } else if (currentChar == 'Q') {
                    this->_canCastle[2] = true;
                } else if (currentChar == 'k') {
                    this->_canCastle[1] = true;
                } else if (currentChar == 'q') {
                    this->_canCastle[3] = true;
                }
            }

            break;
        }
        case 3: {  // enpassant
            int pos = 0;
            for (int i = 0; i < int(tokens[field].length()); i++) {
                char currentChar = tokens[field][i];

                if (currentChar == '-') {
                    this->_enpassantPos = 0;
                    break;
                }

                if (i == 0) {
                    pos += 'h' - currentChar;
                }

                if (i == 1) {
                    pos += 8 * ((currentChar - '0') - 1);
                    this->_enpassantPos = pos;
                }
            }

            break;
        }
        case 4: {  // halfmove clock

            for (int i = 0; i < int(tokens[field].length()); i++) {
                char currentChar = tokens[field][i];

                this->_halfMoves = int(currentChar - '0');
            }

            break;
        }
        case 5: {  // full move number

            for (int i = 0; i < int(tokens[field].length()); i++) {
                char currentChar = tokens[field][i];

                this->_fullMoves = int(currentChar - '0');
            }

            break;
        }
        }
    }

    this->updateAllPieces();
    this->updatePieceOnSquare();

    this->_hash          = Zobrist::zhash(*this);
    this->_curPly        = 0;
    this->_keyHistory[0] = this->_hash;
}

std::string Board::toFen() const {
    static const char printPiece[] = {'P', 'p', 'N', 'n', 'B', 'b', 'R', 'r', 'Q', 'q', 'K', 'k'};
    std::string fen;

    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            const int sq = rank * 8 + (7 - file);
            const int piece = this->_pieceOnSquare[static_cast<size_t>(sq)];
            if (piece == NO_PIECE) {
                empty++;
                continue;
            }
            if (empty) {
                fen += static_cast<char>('0' + empty);
                empty = 0;
            }
            fen += printPiece[piece];
        }
        if (empty) {
            fen += static_cast<char>('0' + empty);
        }
        if (rank != 0) {
            fen += '/';
        }
    }

    fen += this->_blackToMove ? " b " : " w ";

    std::string castle;
    if (this->_canCastle[0]) castle += 'K';
    if (this->_canCastle[2]) castle += 'Q';
    if (this->_canCastle[1]) castle += 'k';
    if (this->_canCastle[3]) castle += 'q';
    fen += castle.empty() ? "-" : castle;

    fen += ' ';
    if (this->_enpassantPos) {
        const int sq = this->_enpassantPos;
        fen += static_cast<char>('h' - (sq & 7));
        fen += static_cast<char>('1' + (sq >> 3));
    } else {
        fen += '-';
    }

    fen += ' ';
    fen += std::to_string(this->_halfMoves);
    fen += ' ';
    fen += std::to_string(this->_fullMoves);
    return fen;
}

void Board::setPieceSet(int i, uint64_t num) { this->_pieces[i] = num; }

void Board::setStartPos() {
    this->_pieces[toInt(Piece::WHITE_PAWN)]   = 0x000000000000FF00;
    this->_pieces[toInt(Piece::BLACK_PAWN)]   = 0x00FF000000000000;
    this->_pieces[toInt(Piece::WHITE_KNIGHT)] = 0x0000000000000042;
    this->_pieces[toInt(Piece::BLACK_KNIGHT)] = 0x4200000000000000;
    this->_pieces[toInt(Piece::WHITE_BISHOP)] = 0x0000000000000024;
    this->_pieces[toInt(Piece::BLACK_BISHOP)] = 0x2400000000000000;
    this->_pieces[toInt(Piece::WHITE_ROOK)]   = 0x0000000000000081;
    this->_pieces[toInt(Piece::BLACK_ROOK)]   = 0x8100000000000000;
    this->_pieces[toInt(Piece::WHITE_QUEEN)]  = 0x0000000000000010;
    this->_pieces[toInt(Piece::BLACK_QUEEN)]  = 0x1000000000000000;
    this->_pieces[toInt(Piece::WHITE_KING)]   = 0x000000000000008;
    this->_pieces[toInt(Piece::BLACK_KING)]   = 0x800000000000000;

    updateAllPieces();
    updatePieceOnSquare();

    // while 0 is a position on the board, it is not possible for enpessant sqaure to be 0, so this fine
    this->_enpassantPos = 0;

    for (int i = 0; i < 4; i++) {
        this->_canCastle[i] = true;
    }

    this->_blackToMove = false;

    this->_halfMoves = 0;
    this->_fullMoves = 1;

    this->_hash          = Zobrist::zhash(*this);
    this->_curPly        = 0;
    this->_keyHistory[0] = this->_hash;
}

void Board::updateAllPieces() {
    _allPieces[0] = 0;
    _allPieces[1] = 0;
    for (int i = 0; i < 12; i += 2) {
        _allPieces[0] |= _pieces[i];
        _allPieces[1] |= _pieces[i + 1];
    }

    this->_empty = ~(this->_allPieces[0] | this->_allPieces[1]);
}

void Board::updatePieceOnSquare() {
    for (int i = 0; i < 64; i++) {
        _pieceOnSquare[i] = NO_PIECE;
    }

    for (int i = 0; i < 12; i++) {
        uint64_t bitboard = _pieces[i];

        while (bitboard) {
            const int currentSquare = tz_count(bitboard);
            pop_lsb(bitboard);

            _pieceOnSquare[currentSquare] = static_cast<Piece>(i);
        }
    }
}

// move is assumed to be legal, undefinded behavior with an illegal/null move
void Board::makeMove(const Move& move) {
    assert(!move.isNull());
    assert(_curPly + 1 < kMaxPly);

    const int       from          = move.from();
    const int       to            = move.to();
    const uint64_t  fromMask      = maskForPos(from);
    const uint64_t  toMask        = maskForPos(to);
    const Color     color         = this->blackToMove() ? BLACK : WHITE;
    const Color     enemyColor    = static_cast<Color>(!color);
    const PieceType pieceType     = static_cast<PieceType>(toInt(this->_pieceOnSquare[from]) - static_cast<int>(color));
    const Piece     piece         = Bitboard::colorPiece(color, pieceType);
    const Piece     capturedPiece = this->_pieceOnSquare[to];

    // Snapshot the position we are leaving
    Piece undoCaptured = NO_PIECE;
    if (move.moveType() == MoveType::EN_PASSANT) {
        undoCaptured = Bitboard::colorPiece(enemyColor, PieceType::PAWNS);
    } else if (move.moveType() != MoveType::CASTLE && capturedPiece != NO_PIECE
               && toInt(capturedPiece) % 2 == enemyColor) {
        undoCaptured = capturedPiece;
    }

    this->_undoStack[this->_curPly] = {
      this->_hash, move, this->_halfMoves, undoCaptured, this->_canCastle.raw(), this->_enpassantPos,
    };

    // update my pieces
    this->_pieces[toInt(piece)] &= ~fromMask;  // remove old position
    this->_pieces[toInt(piece)] |= toMask;     // add new position

    this->_allPieces[color] &= ~fromMask;
    this->_allPieces[color] |= toMask;

    this->_hash ^= Zobrist::randomTable[from][toInt(piece)];
    this->_hash ^= Zobrist::randomTable[to][toInt(piece)];

    this->_pieceOnSquare[from] = NO_PIECE;
    this->_pieceOnSquare[to]   = piece;

    // update opponent pieces (if its a capture)
    if (capturedPiece != NO_PIECE && toInt(capturedPiece) % 2 == enemyColor) {
        // remove the piece
        this->_pieces[toInt(capturedPiece)] &= ~toMask;

        this->_allPieces[enemyColor] &= ~toMask;

        // capture, reset halfmoves
        this->_halfMoves = 0;

        this->_hash ^= Zobrist::randomTable[to][toInt(capturedPiece)];

        if (capturedPiece == Bitboard::colorPiece(enemyColor, PieceType::PAWNS)) {
            // if a pawn is captured, check if its the last pawn on the board
            if ((this->getBB(WHITE, PieceType::PAWNS) | this->getBB(BLACK, PieceType::PAWNS)) == 0) {
                this->_hash ^= Zobrist::noPawns;
            }
        } else if (capturedPiece == Bitboard::colorPiece(enemyColor, PieceType::ROOKS)) [[unlikely]] {
            // if we captured the enemies rook, they can no longer castle
            for (auto side : {0 /* king side */, 2 /* queen side */}) {
                if ((toMask & Bitboard::originalRookSquares[enemyColor + side]) != 0) {
                    if (this->_canCastle[enemyColor + side]) {
                        this->_canCastle[enemyColor + side] = false;
                        this->_hash ^= Zobrist::castling[enemyColor + side];
                    }
                }
            }
        }
    }

    // half moves are incremented on every move
    this->_halfMoves++;
    // we increment full moves after each black move
    if (color == BLACK) {
        this->_fullMoves++;
    }

    // piece specific special cases
    switch (pieceType) {
    case PieceType::PAWNS:
        // if pawn move, reset halfmoves
        this->_halfMoves = 0;

        // special pawn move handling
        if (move.moveType() == MoveType::EN_PASSANT) [[unlikely]] {
            // pawn was already moved above, just have to get rid of the piece it took
            int capturedPawnPos = this->_enpassantPos - Bitboard::pawnPush(color);

            this->_pieces[enemyColor] &= ~maskForPos(capturedPawnPos);

            this->_allPieces[enemyColor] &= ~maskForPos(capturedPawnPos);

            this->_hash ^= Zobrist::randomTable[capturedPawnPos][enemyColor];

            this->_pieceOnSquare[capturedPawnPos] = NO_PIECE;
        } else if (move.moveType() == MoveType::PROMOTION) [[unlikely]] {
            PieceType promoPiece = static_cast<PieceType>(move.promotionPiece() * 2 + 2);

            // add the new piece
            this->_pieces[toInt(Bitboard::colorPiece(color, promoPiece))] |= toMask;
            // remove the pawn we added by default
            this->_pieces[color] &= ~toMask;

            this->_hash ^= Zobrist::randomTable[to][toInt(Bitboard::colorPiece(color, promoPiece))];
            this->_hash ^= Zobrist::randomTable[to][color];

            this->_pieceOnSquare[to] = Bitboard::colorPiece(color, promoPiece);

            if ((this->getBB(WHITE, PieceType::PAWNS) | this->getBB(BLACK, PieceType::PAWNS)) == 0) {
                this->_hash ^= Zobrist::noPawns;
            }
        }

        // remove old enpassant file from hash
        if (this->_enpassantPos) [[unlikely]] {
            this->_hash ^= Zobrist::enpassantFile[Bitboard::fileOf(this->_enpassantPos)];
        }
        this->_enpassantPos = 0;

        // pawn double push, need to set enpassant pos
        if (abs(to - from) > 9) {
            this->_enpassantPos = to - Bitboard::pawnPush(color);

            // update hash with new enpassant file
            this->_hash ^= Zobrist::enpassantFile[Bitboard::fileOf(this->_enpassantPos)];
        }

        break;
    case PieceType::ROOKS:
        // remove old enpassant file from hash
        if (this->_enpassantPos) [[unlikely]] {
            this->_hash ^= Zobrist::enpassantFile[Bitboard::fileOf(this->_enpassantPos)];
        }
        this->_enpassantPos = 0;

        // rook move, can no longer castle on that side
        if (this->_canCastle[color]) {
            // if king side rook not on original square
            if ((Bitboard::originalRookSquares[color] & this->_pieces[toInt(piece)]) == 0) {
                this->_canCastle[color] = false;
                this->_hash ^= Zobrist::castling[color];
            }
        }

        if (this->_canCastle[color + 2]) {
            if ((Bitboard::originalRookSquares[color + 2] & this->_pieces[toInt(piece)]) == 0) {  // if queen side...
                this->_canCastle[color + 2] = false;
                this->_hash ^= Zobrist::castling[color + 2];
            }
        }
        break;
    case PieceType::KINGS:
        // remove old enpassant file from hash
        if (this->_enpassantPos) [[unlikely]] {
            this->_hash ^= Zobrist::enpassantFile[Bitboard::fileOf(this->_enpassantPos)];
        }
        this->_enpassantPos = 0;

        // update castle bitboards
        if (move.moveType() == MoveType::CASTLE) [[unlikely]] {
            int         index     = Bitboard::rookPosToIndex(to);
            const Piece kingPiece = Bitboard::colorPiece(color, PieceType::KINGS);
            const Piece rookPiece = Bitboard::colorPiece(color, PieceType::ROOKS);

            // update king and rook pos
            this->_pieces[toInt(kingPiece)] = Bitboard::castledKingSquares[index];
            this->_pieces[toInt(rookPiece)] |= Bitboard::castledRookSquares[index];
            this->_pieces[toInt(rookPiece)] &= ~Bitboard::originalRookSquares[index];

            this->_allPieces[color] &= ~toMask;
            this->_allPieces[color] |= Bitboard::castledKingSquares[index];
            this->_allPieces[color] |= Bitboard::castledRookSquares[index];

            // undo from earlier
            this->_hash ^= Zobrist::randomTable[to][toInt(piece)];
            // add real pos
            this->_hash ^= Zobrist::randomTable[tz_count(Bitboard::castledKingSquares[index])][toInt(kingPiece)];
            // update rook
            this->_hash ^= Zobrist::randomTable[tz_count(Bitboard::castledRookSquares[index])][toInt(rookPiece)];
            this->_hash ^= Zobrist::randomTable[tz_count(Bitboard::originalRookSquares[index])][toInt(rookPiece)];

            this->_pieceOnSquare[to]                                            = NO_PIECE;
            this->_pieceOnSquare[tz_count(Bitboard::castledKingSquares[index])] = piece;
            this->_pieceOnSquare[tz_count(Bitboard::castledRookSquares[index])] = rookPiece;
        }

        // king move, can no longer castle
        if (this->_canCastle[color]) {
            this->_canCastle[color] = false;
            this->_hash ^= Zobrist::castling[color];
        }
        if (this->_canCastle[color + 2]) {
            this->_canCastle[color + 2] = false;
            this->_hash ^= Zobrist::castling[color + 2];
        }
        break;
    default:
        // remove old enpassant file from hash
        if (this->_enpassantPos) [[unlikely]] {
            this->_hash ^= Zobrist::enpassantFile[Bitboard::fileOf(this->_enpassantPos)];
        }
        this->_enpassantPos = 0;
        break;
    }

    // update black to move
    this->_blackToMove = enemyColor;
    this->_hash ^= Zobrist::randomBlackToMove;

    // update empty squares bitboard
    this->_empty = ~(this->_allPieces[0] | this->_allPieces[1]);

    this->_curPly++;
    this->_keyHistory[this->_curPly] = this->_hash;
}

void Board::undoMove() {
    assert(this->_curPly > 0);
    this->_curPly--;
    const Undo undo = this->_undoStack[this->_curPly];

    const Move     move     = undo.move;
    const int      from     = move.from();
    const int      to       = move.to();
    const uint64_t fromMask = maskForPos(from);
    const uint64_t toMask   = maskForPos(to);

    // Side to move is the opponent of the side that just moved.
    const Color color      = this->_blackToMove ? WHITE : BLACK;
    const Color enemyColor = static_cast<Color>(!color);

    if (move.moveType() == MoveType::CASTLE) {
        const int      index        = Bitboard::rookPosToIndex(to);
        const int      kingDest     = tz_count(Bitboard::castledKingSquares[index]);
        const int      rookDest     = tz_count(Bitboard::castledRookSquares[index]);
        const uint64_t kingDestMask = maskForPos(kingDest);
        const uint64_t rookDestMask = maskForPos(rookDest);
        const Piece    kingPiece    = Bitboard::colorPiece(color, PieceType::KINGS);
        const Piece    rookPiece    = Bitboard::colorPiece(color, PieceType::ROOKS);

        this->_pieces[toInt(kingPiece)] = fromMask;
        this->_pieces[toInt(rookPiece)] &= ~rookDestMask;
        this->_pieces[toInt(rookPiece)] |= toMask;

        this->_allPieces[color] &= ~kingDestMask;
        this->_allPieces[color] &= ~rookDestMask;
        this->_allPieces[color] |= fromMask | toMask;

        this->_pieceOnSquare[kingDest] = NO_PIECE;
        this->_pieceOnSquare[rookDest] = NO_PIECE;
        this->_pieceOnSquare[from]     = kingPiece;
        this->_pieceOnSquare[to]       = rookPiece;
    } else if (move.moveType() == MoveType::PROMOTION) {
        const PieceType promoPiece = static_cast<PieceType>(move.promotionPiece() * 2 + 2);
        const Piece     promoIndex = Bitboard::colorPiece(color, promoPiece);
        const Piece     pawnIndex  = Bitboard::colorPiece(color, PieceType::PAWNS);

        this->_pieces[toInt(promoIndex)] &= ~toMask;
        this->_pieces[toInt(pawnIndex)] |= fromMask;

        this->_allPieces[color] &= ~toMask;
        this->_allPieces[color] |= fromMask;

        this->_pieceOnSquare[to]   = NO_PIECE;
        this->_pieceOnSquare[from] = pawnIndex;

        if (undo.captured != NO_PIECE) {
            this->_pieces[toInt(undo.captured)] |= toMask;
            this->_allPieces[enemyColor] |= toMask;
            this->_pieceOnSquare[to] = undo.captured;
        }
    } else if (move.moveType() == MoveType::EN_PASSANT) {
        const Piece    pawnIndex       = Bitboard::colorPiece(color, PieceType::PAWNS);
        const int      capturedPawnPos = undo.ep - Bitboard::pawnPush(color);
        const uint64_t capturedMask    = maskForPos(capturedPawnPos);

        this->_pieces[toInt(pawnIndex)] &= ~toMask;
        this->_pieces[toInt(pawnIndex)] |= fromMask;
        this->_allPieces[color] &= ~toMask;
        this->_allPieces[color] |= fromMask;

        this->_pieces[toInt(undo.captured)] |= capturedMask;
        this->_allPieces[enemyColor] |= capturedMask;

        this->_pieceOnSquare[to]              = NO_PIECE;
        this->_pieceOnSquare[from]            = pawnIndex;
        this->_pieceOnSquare[capturedPawnPos] = undo.captured;
    } else [[likely]] {
        const Piece movedPiece = this->_pieceOnSquare[to];

        this->_pieces[toInt(movedPiece)] &= ~toMask;
        this->_pieces[toInt(movedPiece)] |= fromMask;
        this->_allPieces[color] &= ~toMask;
        this->_allPieces[color] |= fromMask;

        this->_pieceOnSquare[to]   = NO_PIECE;
        this->_pieceOnSquare[from] = movedPiece;

        if (undo.captured != NO_PIECE) {
            this->_pieces[toInt(undo.captured)] |= toMask;
            this->_allPieces[enemyColor] |= toMask;
            this->_pieceOnSquare[to] = undo.captured;
        }
    }

    if (color == BLACK) {
        this->_fullMoves--;
    }

    this->_hash      = undo.hash;
    this->_halfMoves = undo.halfMoves;
    this->_canCastle.setRaw(undo.castle);
    this->_enpassantPos = undo.ep;
    this->_blackToMove  = static_cast<uint8_t>(color);
    this->_empty        = ~(this->_allPieces[0] | this->_allPieces[1]);
}

int Board::getRepeats(uint64_t hash) const {
    int count = 0;
    for (int i = this->_curPly; i >= 0 && (this->_curPly - i) <= this->_halfMoves; i -= 2) {
        if (this->_keyHistory[i] == hash) {
            count++;
        }
    }

    return count;
}

bool Board::inCheck() const {
    if (this->_blackToMove) {
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
                uint64_t currentBB = board._pieces[piece];

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
