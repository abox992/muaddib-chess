// #include <chrono>
// #include <cstdint>
// #include <bit>
// #include <tuple>

// #include "movegen.h"
// #include "board.h"
// #include "precompute_masks.h"
// #include "move.h"
// #include "helpers.h"
// #include "check_pin_masks.h"
// #include "bit_manip.h"

// using namespace std;

// // move list to add moves to, color to gen moves for (0 for white 1 for black), returns movecount
// template<int moveType>
// int generateMoves(Board& board, struct Move moveList[], int color) {

//     uint64_t moveTypeMask; // we generate all moves, then filter by this mask
//     if constexpr (moveType == ALL_MOVES_FLAG) {
//         moveTypeMask = ~uint64_t(0);
//     }
//     else if constexpr (moveType == CAPTURES_FLAG) {
//         moveTypeMask = board.state.allPieces[!board.state.blackToMove];
//     }
//     else if constexpr (moveType == QUIET_FLAG) {
//         moveTypeMask = board.state.empty;
//     }

//     int moveCount = 0;
//     int enemyColor = color == 0 ? 1 : 0;

//     uint64_t currentSquareMask;

//     // check mask
//     tuple<uint64_t, uint64_t, uint64_t> checkAndPinMasks = generateCheckAndPinMask(board, color);
//     uint64_t checkMask = get<0>(checkAndPinMasks);
//     uint64_t pinHV     = get<1>(checkAndPinMasks);
//     uint64_t pinDiag   = get<2>(checkAndPinMasks);

//     /*

//         Iterate board from top left to bottom right (63 -> 0)

//         63 62 61 60 59 58 57 56
//         57 56   .....
//         07 06 05 04 03 02 01 00

//     */
//     for (int rank = 7; rank >= 0; rank--) {
//         for (int file = 0; file < 8; file++) {
//             int currentSquare = (8 * rank) + (7 - file);
//             currentSquareMask = maskForPos(currentSquare);

//             if ((board.state.empty & currentSquareMask) != 0) { // skip if empty
//                 continue;
//             }

//             // find the corresponding piece type
//             int piece = 0;
//             for (; piece < 12; piece += 2) {
//                 uint64_t currentBB = board.getPieceSet(piece + color);

//                 if ((currentBB & currentSquareMask) != 0) { // found piece at square
//                     break;
//                 }
//             }

//             /*  
//             *  9  8  7
//             *  1  0 -1
//             * -7 -8 -9
//             */
//             switch (piece) {
//                 case Piece::PAWNS: { // pawn 

//                     // chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

//                     uint64_t initialMoveMask = pawnMoveMasks[color][currentSquare];
//                     uint64_t initialAttackMask = pawnAttackMasks[color][currentSquare];

//                     uint64_t pLegalMoves = initialMoveMask & board.state.empty;
//                     uint64_t pLegalAttacks = initialAttackMask & board.state.allPieces[enemyColor];

//                     // en passant
//                     if (board.state.enpassantPos > 0) {
//                         pLegalAttacks |= (initialAttackMask & maskForPos(board.state.enpassantPos));
//                     }

//                     // adjust for checks
//                     pLegalMoves &= checkMask;
//                     pLegalAttacks &= checkMask;

//                     if (board.state.enpassantPos > 0) {
//                         int offset = color == 0 ? -8 : 8;
//                         board.state.pieces[enemyColor] &= ~(uint64_t(1) << (board.state.enpassantPos + offset));

//                         if (generateCheckMask(board, color) == 0) {
//                             pLegalAttacks |= (initialAttackMask & maskForPos(board.state.enpassantPos));
//                         }

//                         board.state.pieces[enemyColor] |= (uint64_t(1) << (board.state.enpassantPos + offset));

//                     }

//                     pLegalMoves &= moveTypeMask;

//                     for (int index = std::countr_zero(pLegalMoves); pLegalMoves; popRSB(pLegalMoves), index = std::countr_zero(pLegalMoves)) {

//                         struct Move Temp;
//                         Temp.from = currentSquare;
//                         Temp.to = index;
//                         Temp.color = color;
//                         Temp.piece = piece;

//                         int currentY = currentSquare / 8;
//                         int currentX = currentSquare % 8;
//                         int newY = index / 8;
//                         int newX = index % 8;

//                         // make sure were not jumping over a piece on pawn double move
//                         int maxDif = max(abs(currentX - newX), abs(currentY - newY));
//                         if (maxDif > 1) {
//                             if ((pawnMoveMasks[color][currentSquare] & ~board.state.empty) != 0) { // check if square is empty
//                                 continue;
//                             }
//                         }

//                         // promotion
//                         if ((index / 8) == 0 || (index / 8) == 7) {
//                             for (int i = 0; i < 4; i++) {
//                                 struct Move Promo;
//                                 Promo = Temp;
//                                 Promo.promotion = maskForPos(i);

//                                 // check pin
//                                 Board pin = board;
//                                 pin.makeMove(Temp);
//                                 uint64_t checkAfterMove = attacksToKing(pin, color);
//                                 if (checkAfterMove != 0) {
//                                     continue;
//                                 }

//                                 moveList[moveCount++] = Promo;
//                             }

//                             continue;
//                         }

//                         // check pin
//                         Board pin = board;
//                         pin.makeMove(Temp);
//                         uint64_t checkAfterMove = attacksToKing(pin, color);
//                         if (checkAfterMove != 0) {
//                             continue;
//                         }

//                         moveList[moveCount++] = Temp;
                            
//                     }

//                     if constexpr (moveType == QUIET_FLAG) {
//                         pLegalAttacks &= moveTypeMask;
//                     }

//                     Bitloop(pLegalAttacks) {
//                         const int index = squareOf(pLegalAttacks);

//                         struct Move Temp;
//                         Temp.from = currentSquare;
//                         Temp.to = index;
//                         Temp.color = color;
//                         Temp.piece = piece;

//                         if (board.state.enpassantPos > 0 && index == board.state.enpassantPos) {
//                             Temp.enpessant = true;
//                         }

//                         // promotion
//                         if ((index / 8) == 0 || (index / 8) == 7) {
//                             for (int i = 0; i < 4; i++) {
//                                 struct Move Promo;
//                                 Promo = Temp;
//                                 Promo.promotion = maskForPos(i);

//                                 // check pin
//                                 Board pin = board;
//                                 pin.makeMove(Temp);
//                                 uint64_t checkAfterMove = attacksToKing(pin, color);
//                                 if (checkAfterMove != 0) {
//                                     continue;
//                                 }

//                                 moveList[moveCount++] = Promo;
//                             }

//                             continue;
//                         }

//                         // check pin
//                         Board pin = board;
//                         pin.makeMove(Temp);
//                         uint64_t checkAfterMove = attacksToKing(pin, color);
//                         if (checkAfterMove != 0) {
//                             continue;
//                         }

//                         moveList[moveCount++] = Temp;
                            
//                     }

//                     break;
//                 } case Piece::KNIGHTS: { // knight 

//                     uint64_t pLegalMoves = knightMasks[currentSquare] & (board.state.empty | board.state.allPieces[enemyColor]);

//                     // adjust for checks
//                     pLegalMoves &= checkMask;
//                     pLegalMoves &= moveTypeMask;

//                     Bitloop(pLegalMoves) {
//                         const int index = squareOf(pLegalMoves);

//                         struct Move Temp;
//                         Temp.from = currentSquare;
//                         Temp.to = index;
//                         Temp.color = color;
//                         Temp.piece = piece;

//                         // check pin
//                         Board pin = board;
//                         pin.makeMove(Temp);
//                         uint64_t checkAfterMove = attacksToKing(pin, color);
//                         if (checkAfterMove != 0) {
//                             continue;
//                         }

//                         moveList[moveCount++] = Temp;
                            
//                     }
                        
//                     break;
//                 }
//                 case Piece::BISHOPS: { // bishop

//                     uint64_t blockers = (~board.state.empty) & bishopMasks[currentSquare];
//                     uint64_t compressedBlockers = extract_bits(blockers, bishopMasks[currentSquare]);

//                     uint64_t pLegalMoves = bishopLegalMoves[currentSquare][compressedBlockers];
//                     pLegalMoves &= ~board.state.allPieces[color];

//                     // adjust for checks
//                     pLegalMoves &= checkMask;

//                     if ((currentSquareMask & pinHV) != 0) { // can not move if pinned vert
//                         continue;
//                     }

//                     pLegalMoves &= moveTypeMask;

//                     Bitloop(pLegalMoves) {
//                         const int index = squareOf(pLegalMoves);

//                         struct Move Temp;
//                         Temp.from = currentSquare;
//                         Temp.to = index;
//                         Temp.color = color;
//                         Temp.piece = piece;

//                         // check pin
//                         Board pin = board;
//                         pin.makeMove(Temp);
//                         uint64_t checkAfterMove = attacksToKing(pin, color);
//                         if (checkAfterMove != 0) {
//                             continue;
//                         }

//                         moveList[moveCount++] = Temp;
                            
//                     }

//                     break;
//                 } case Piece::ROOKS: { // rook
//                     uint64_t blockers = (~board.state.empty) & rookMasks[currentSquare];
//                     uint64_t compressedBlockers = extract_bits(blockers, rookMasks[currentSquare]);

//                     uint64_t pLegalMoves = rookLegalMoves[currentSquare][compressedBlockers];
//                     pLegalMoves &= ~board.state.allPieces[color];

//                     // adjust for checks
//                     pLegalMoves &= checkMask;

//                     if ((currentSquareMask & pinDiag) != 0) { // can not move if pinned diag
//                         continue;
//                     }

//                     pLegalMoves &= moveTypeMask;

//                     Bitloop(pLegalMoves) {
//                         const int index = squareOf(pLegalMoves);

//                         struct Move Temp;
//                         Temp.from = currentSquare;
//                         Temp.to = index;
//                         Temp.color = color;
//                         Temp.piece = piece;

//                         // check pin
//                         Board pin = board;
//                         pin.makeMove(Temp);
//                         uint64_t checkAfterMove = attacksToKing(pin, color);
//                         if (checkAfterMove != 0) {
//                             continue;
//                         }

//                         moveList[moveCount++] = Temp;
                            
//                     }

//                     break;
//                 } case Piece::QUEENS: { // queen

//                     uint64_t blockers = (~board.state.empty) & rookMasks[currentSquare];
//                     uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[currentSquare]);

//                     blockers = (~board.state.empty) & bishopMasks[currentSquare];
//                     uint64_t bishopCompressedBlockers = extract_bits(blockers, bishopMasks[currentSquare]);

//                     uint64_t pLegalMoves = rookLegalMoves[currentSquare][rookCompressedBlockers] | bishopLegalMoves[currentSquare][bishopCompressedBlockers];
//                     pLegalMoves &= ~board.state.allPieces[color];

//                     // adjust for checks
//                     pLegalMoves &= checkMask;
//                     pLegalMoves &= moveTypeMask;

//                     Bitloop(pLegalMoves) {
//                         const int index = squareOf(pLegalMoves);

//                         struct Move Temp;
//                         Temp.from = currentSquare;
//                         Temp.to = index;
//                         Temp.color = color;
//                         Temp.piece = piece;

//                         // check pin
//                         Board pin = board;
//                         pin.makeMove(Temp);
//                         uint64_t checkAfterMove = attacksToKing(pin, color);
//                         if (checkAfterMove == 0) {
//                             moveList[moveCount++] = Temp;
//                         }
                            
//                     }

//                     break;
//                 } case Piece::KINGS: { // king 

//                     uint64_t pLegalMoves = kingMasks[currentSquare] & (board.state.empty | board.state.allPieces[enemyColor]);

//                     int kingPos = currentSquare;

//                     // castles
//                     if (~checkMask == 0) { // can only castle if not in check
//                         if (board.state.canCastle[color]) { // king side
//                             if ((~board.state.empty & castleMasks[color]) == 0) {
//                                 if (attacksOnSquare(board, color, currentSquare - 1) == 0) { // cannot pass through check
//                                     pLegalMoves |= castleSquares[color];
//                                 }
//                             }
//                         }

//                         if (board.state.canCastle[color + 2]) { // queen side
//                             if ((~board.state.empty & castleMasks[color + 2]) == 0) {
//                                 if (attacksOnSquare(board, color, currentSquare + 1) == 0) { // cannot pass through check
//                                     pLegalMoves |= castleSquares[color + 2];
//                                 }
//                             }
//                         }
//                     }

//                     pLegalMoves &= moveTypeMask;

//                     Bitloop(pLegalMoves) {
//                         const int index = squareOf(pLegalMoves);

//                         // filter out moves that leave the king in check
//                         if (attacksOnSquareIgnoreKing(board, color, index) != 0) {
//                             continue;
//                         }

//                         struct Move Temp;
//                         Temp.from = currentSquare;
//                         Temp.to = index;
//                         Temp.color = color;
//                         Temp.piece = piece;

//                         int currentY = kingPos / 8;
//                         int currentX = kingPos % 8;
//                         int newY = index / 8;
//                         int newX = index % 8;

//                         int maxDif = max(abs(currentX - newX), abs(currentY - newY));

//                         if (maxDif > 1) {
//                             if ((maskForPos(index) & castleSquares[color]) != 0) {
//                                 Temp.castle = maskForPos(color);
//                             } else if ((maskForPos(index) & castleSquares[color + 2]) != 0) {
//                                 Temp.castle = maskForPos(color + 2);
//                             }
//                         }

//                         moveList[moveCount++] = Temp;
                            
//                     }
                        
//                     break;
//                 }
//             }
//         }

//     }

//     return moveCount;

// }

// template int generateMoves<ALL_MOVES_FLAG>(Board& board, struct Move moveList[], int color);
// template int generateMoves<CAPTURES_FLAG>(Board& board, struct Move moveList[], int color);
// template int generateMoves<QUIET_FLAG>(Board& board, struct Move moveList[], int color);