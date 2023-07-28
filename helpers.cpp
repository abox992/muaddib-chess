#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <ctype.h>
#include <bit>
#include "board.h"

using namespace std;

void printBitboard(uint64_t bitboard) {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int i = ((7 - file) + (8 * rank));
            uint64_t mask = uint64_t(1) << i;
            int bit = ((bitboard & mask) >> i);
            cout << bit << " ";
        }

        cout << endl;
    }
}

vector<string> split(const string &s, char delim) {
    vector<string> elems;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

Board generateBoardFromFen(string fen) {
    Board board = Board();

    for(int i = 0; i < 12; i++) { // zero all bitboards
        board.setPieceSet(i, 0);
    }

    vector<string> tokens = split(fen, ' ');
    for (int field = 0; field < int(tokens.size()); field++) {
        switch (field) {
            case 0: { // piece positions
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
                            board.setPieceSet(j, board.getPieceSet(j) | (uint64_t(1) << currentPos));
                        }
                    }

                    currentPos--;

                }

                break;
            } case 1: { // piece to move

                for (int i = 0; i < int(tokens[field].length()); i++) {
                    char currentChar = tokens[field][i];

                    if (currentChar == 'w') {
                        board.blackToMove = false;
                    } else {
                        board.blackToMove = true;
                    }
                }

                break;
            } case 2: { // castling

                for (int i = 0; i < 4; i++) {
                    board.castle[i] = false;
                }

                for (int i = 0; i < int(tokens[field].length()); i++) {
                    char currentChar = tokens[field][i];

                    if (currentChar == '-') {
                        break;
                    }

                    if (currentChar == 'K') {
                        board.castle[0] = true;
                    } else if (currentChar == 'Q') {
                        board.castle[2] = true;
                    } else if (currentChar == 'k') {
                        board.castle[1] = true;
                    } else if (currentChar == 'q') {
                        board.castle[3] = true;
                    }

                }

                break;
            } case 3: { // enpassant
                int pos = -1;
                for (int i = 0; i < int(tokens[field].length()); i++) {
                    char currentChar = tokens[field][i];

                    if (currentChar == '-') {
                        board.enpassantPos = -1;
                        break;
                    }

                    if (i == 0) {
                        pos += 'h' - currentChar;
                    }

                    if (i == 1) {
                        pos += 8 * ((currentChar - '0') - 1);
                        board.enpassantPos = pos + 1;
                    }

                }

                break;
            } case 4: { // halfmove clock

                for (int i = 0; i < int(tokens[field].length()); i++) {
                    //char currentChar = tokens[field][i];
                }

                break;
            } case 5: { // full move number

                for (int i = 0; i < int(tokens[field].length()); i++) {
                    //char currentChar = tokens[field][i];
                }

                break;
            }
            
        }
    }

board.updateAllPieces();
return board;

}