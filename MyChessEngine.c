#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>


typedef struct {
    unsigned long long int* moves;
    int length;
    int capacity;
} MoveList;

typedef struct {
    unsigned long long int pieceBB[14];
    unsigned long long int emptyBB;
    unsigned long long int occupiedBB;
    bool castlingRights[4];
    unsigned long long int epSquare;
    int boardBySquare[64];
    int halfMoveClock;
    int fullMoveNumber;
    int playerToMove;
    MoveList history;
} Board;

typedef struct {
    Board* board;
    char* pieceSymbols;
} Parameters;

unsigned long long int notAFile = 0x7F7F7F7F7F7F7F7FULL;
unsigned long long int notHFile = 0xFEFEFEFEFEFEFEFEULL;

void initMoveList(MoveList *ml, int capacity) {
    ml->moves = malloc(capacity * sizeof(unsigned long long int));
    ml->length = 0;
    ml->capacity = capacity;
}

void destroyMoveList(MoveList* ml) {
    free(ml->moves);
}

void addMove(MoveList *ml, unsigned long long int move) {
    if (ml->length == ml->capacity) {
        ml->capacity += 10;
        unsigned long long int *temp = (unsigned long long int*)realloc(ml->moves, ml->capacity * sizeof(unsigned long long int));
        if (temp == NULL) {
            printf("problem while trying to grow a move list\n");
            free(ml->moves);
            exit(0);
        }
        ml->moves = temp;
    }
    ml->moves[ml->length] = move;
    ml->length++;
}

void removeLastMove(MoveList* ml) {
    ml->length--;
}

// Sets up the chess board to the starting position
void initBoardState(Board* board, char* pieceSymbols) {
    board->pieceBB[0] = 0x000000000000FFFFL; // White
    board->pieceBB[1] = 0x000000000000FF00L; // Pawns
    board->pieceBB[2] = 0x0000000000000042L; // Knights
    board->pieceBB[3] = 0x0000000000000024L; // Bishops
    board->pieceBB[4] = 0x0000000000000081L; // Rooks
    board->pieceBB[5] = 0x0000000000000010L; // Queens
    board->pieceBB[6] = 0x0000000000000008L; // Kings
    board->pieceBB[7] = 0xFFFF000000000000L; // Black
    board->pieceBB[8] = 0x00FF000000000000L; // Pawns
    board->pieceBB[9] = 0x4200000000000000L; // Knights
    board->pieceBB[10] = 0x2400000000000000L; // Bishops
    board->pieceBB[11] = 0x8100000000000000L; // Rooks
    board->pieceBB[12] = 0x1000000000000000L; // Queens
    board->pieceBB[13] = 0x0800000000000000L; // Kings
    board->emptyBB = 0x0000ffffffff0000ULL;
    board->occupiedBB = 0xffff00000000ffffULL;
    board->castlingRights[0] = true; board->castlingRights[1] = true; // K  Q
    board->castlingRights[2] = true; board->castlingRights[3] = true; // k  q
    board->epSquare = 0;

    // This looks rotated, but that is so that the index gotten by bitscanforward on a bitboard to line up correctly,
    // since 0 there is the bottom right, and zero here is top left
    int temp[64] = {
        4, 2, 3, 6, 5, 3, 2, 4,
        1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1,
        4, 2, 3, 6, 5, 3, 2, 4
    };
    memcpy_s(board->boardBySquare, sizeof(board->boardBySquare), temp, 64 * sizeof(int));
    board->halfMoveClock = 0;
    board->fullMoveNumber = 1;
    board->playerToMove = 0;
    initMoveList(&(board->history), 30);

    strcpy_s(pieceSymbols, 15, "_PNBRQK_pnbrqk");
}

// Assumes a properly formed FEN string, sets up board according to the string
void readFenStringToBoard(char *fenString, Board *board) {
    int i = 0, squareIndex;
    unsigned long long int square = 0x8000000000000000ULL;
    for (i = 0; i < 14; i++) board->pieceBB[i] = 0;
    board->emptyBB = 0xffffffffffffffffULL;
    board->occupiedBB = 0x0000000000000000ULL;
    int temp[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    };
    memcpy_s(board->boardBySquare, sizeof(board->boardBySquare), temp, 64 * sizeof(int));
    board->castlingRights[0] = false; board->castlingRights[1] = false; // K  Q
    board->castlingRights[2] = false; board->castlingRights[3] = false; // k  q
    board->epSquare = 0;

    destroyMoveList(&(board->history));
    initMoveList(&(board->history), 30);

    i = 0;
    while (fenString[i] != ' ') {
        if (isalpha(fenString[i])) {
            switch (fenString[i]) {
            case 'P':
                board->emptyBB ^= square;
                board->occupiedBB |= square;
                board->pieceBB[0] |= square;
                board->pieceBB[1] |= square;
                BitScanForward64(&squareIndex, square);
                board->boardBySquare[squareIndex] = 1;
                break;
            case 'N':
                board->emptyBB ^= square;
                board->occupiedBB |= square;
                board->pieceBB[0] |= square;
                board->pieceBB[2] |= square;
                BitScanForward64(&squareIndex, square);
                board->boardBySquare[squareIndex] = 2;
                break;
            case 'B':
                board->emptyBB ^= square;
                board->occupiedBB |= square;
                board->pieceBB[0] |= square;
                board->pieceBB[3] |= square;
                BitScanForward64(&squareIndex, square);
                board->boardBySquare[squareIndex] = 3;
                break;
            case 'R':
                board->emptyBB ^= square;
                board->occupiedBB |= square;
                board->pieceBB[0] |= square;
                board->pieceBB[4] |= square;
                BitScanForward64(&squareIndex, square);
                board->boardBySquare[squareIndex] = 4;
                break;
            case 'Q':
                board->emptyBB ^= square;
                board->occupiedBB |= square;
                board->pieceBB[0] |= square;
                board->pieceBB[5] |= square;
                BitScanForward64(&squareIndex, square);
                board->boardBySquare[squareIndex] = 5;
                break;
            case 'K':
                board->emptyBB ^= square;
                board->occupiedBB |= square;
                board->pieceBB[0] |= square;
                board->pieceBB[6] |= square;
                BitScanForward64(&squareIndex, square);
                board->boardBySquare[squareIndex] = 6;
                break;
            case 'p':
                board->emptyBB ^= square;
                board->occupiedBB |= square;
                board->pieceBB[7] |= square;
                board->pieceBB[8] |= square;
                BitScanForward64(&squareIndex, square);
                board->boardBySquare[squareIndex] = 1;
                break;
            case 'n':
                board->emptyBB ^= square;
                board->occupiedBB |= square;
                board->pieceBB[7] |= square;
                board->pieceBB[9] |= square;
                BitScanForward64(&squareIndex, square);
                board->boardBySquare[squareIndex] = 2;
                break;
            case 'b':
                board->emptyBB ^= square;
                board->occupiedBB |= square;
                board->pieceBB[7] |= square;
                board->pieceBB[10] |= square;
                BitScanForward64(&squareIndex, square);
                board->boardBySquare[squareIndex] = 3;
                break;
            case 'r':
                board->emptyBB ^= square;
                board->occupiedBB |= square;
                board->pieceBB[7] |= square;
                board->pieceBB[11] |= square;
                BitScanForward64(&squareIndex, square);
                board->boardBySquare[squareIndex] = 4;
                break;
            case 'q':
                board->emptyBB ^= square;
                board->occupiedBB |= square;
                board->pieceBB[7] |= square;
                board->pieceBB[12] |= square;
                BitScanForward64(&squareIndex, square);
                board->boardBySquare[squareIndex] = 5;
                break;
            case 'k':
                board->emptyBB ^= square;
                board->occupiedBB |= square;
                board->pieceBB[7] |= square;
                board->pieceBB[13] |= square;
                BitScanForward64(&squareIndex, square);
                board->boardBySquare[squareIndex] = 6;
                break;
            }
            square = square >> 1;
        }
        else if (isdigit(fenString[i])) {
            square = square >> (fenString[i] - '0');
        } // Don't need to do anything with '/'s, square automatically wraps back
          //when it gets shifted off the right edge of the board.
        i++;
    }

    board->playerToMove = fenString[++i] == 'b'; i++;

    if (fenString[++i] == '-') {
        i++;
    }
    else {
        while (fenString[i] != ' ') {
            switch (fenString[i]) {
            case 'K':
                board->castlingRights[0] = true;
                break;
            case 'Q':
                board->castlingRights[1] = true;
                break;
            case 'k':
                board->castlingRights[2] = true;
                break;
            case 'q':
                board->castlingRights[3] = true;
                break;
            }
            i++;
        }
    }

    if (fenString[++i] == '-') {
        i++;
    }
    else {
        board->epSquare = 1ULL << ('h' - 'a' - fenString[i++] + 1);
        board->epSquare = board->epSquare << (8 * ('8' - '1' - fenString[i++]));
    }
    board->halfMoveClock = fenString[++i] - '0';
    if (fenString[++i] != ' ') {
        board->halfMoveClock = board->halfMoveClock * 10 + fenString[i++] - '0';
    }

    board->fullMoveNumber = fenString[++i] - '0';
    while (isdigit(fenString[++i])) {
        board->fullMoveNumber = board->fullMoveNumber * 10 + fenString[i++] - '0';
    }
}

void strreverse(char* begin, char* end) {
    char temp;
    while (end > begin) {
        temp = *end, * end-- = *begin, * begin++ = temp;
    }
}

void intToString(int value, char* str, int base) {
    static char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char* wstr = str;
    int sign;

    if (base < 2 || base>35) { *wstr = '\0'; return; }

    // Take care of sign
    if ((sign = value) < 0) value = -value;

    // Conversion. Number is reversed.
    do *wstr++ = num[value % base]; while (value /= base);

    if (sign < 0) *wstr++ = '-';

    *wstr = '\0';

    // Reverse string
    strreverse(str, wstr - 1);
}

char* boardToFEN(Board *board, char *pieceSymbols) {
    char* fen = (char*)calloc(92, sizeof(char)); // upper bound on fen string length
    int a = 0, emptyCount = 0;

    // piece placements
    for (int i = 56, k = 0; i >= 0; i -= 8, k += 8) {
        for (int j = 7, m = 0; j >= 0; j--, m++) {
            if ((board->occupiedBB & (1ULL << (i + j))) && emptyCount > 0) {
                fen[(a++)] = '0' + emptyCount;
                emptyCount = 0;
            }

            for (int p = 1; p <= 6; p++) {
                if ((1ULL << (i + j)) & board->pieceBB[p]) { // white piece
                    fen[(a++)] = pieceSymbols[p];
                    break;
                }
                else if ((1ULL << (i + j)) & board->pieceBB[p + 7]) { // black piece
                    fen[(a++)] = pieceSymbols[p + 7];
                    break;
                }
                else if (p == 6) { // empty square. find how many empty squares there are in a row.
                    emptyCount++;
                }
            }

            if (emptyCount > 0 && j == 0) {
                fen[(a++)] = '0' + emptyCount;
                emptyCount = 0;
            }

            if (j == 0 && i != 0) fen[(a++)] = '/';
        }
    }

    // playerToMove
    fen[(a++)] = ' ';
    if (board->playerToMove) {
        fen[(a++)] = 'b';
    }
    else fen[(a++)] = 'w';

    // Castling rights
    fen[(a++)] = ' ';
    if (board->castlingRights[0] || board->castlingRights[1] || board->castlingRights[2] || board->castlingRights[3]) {
        if (board->castlingRights[0]) fen[(a++)] = 'K';
        if (board->castlingRights[1]) fen[(a++)] = 'Q';
        if (board->castlingRights[2]) fen[(a++)] = 'k';
        if (board->castlingRights[3]) fen[(a++)] = 'q';
    }
    else {
        fen[(a++)] = '-';
    }

    // en passant square
    fen[(a++)] = ' ';
    if (board->epSquare) {
        int trailingZeros;
        BitScanForward64(&trailingZeros, board->epSquare);
        fen[(a++)] = 'h' - (trailingZeros % 8);
        fen[(a++)] = '1' + (trailingZeros / 8);
    }
    else fen[(a++)] = '-';

    // half move clock
    fen[(a++)] = ' ';
    if (board->halfMoveClock <= 9) { // only ever going to be 2 digits long at most
        fen[(a++)] = '0' + board->halfMoveClock;
    }
    else {
        fen[(a++)] = '0' + board->halfMoveClock / 10;
        fen[(a++)] = '0' + board->halfMoveClock % 10;
    }

    // full move number
    fen[(a++)] = ' ';
    char buffer[6];
    intToString(board->fullMoveNumber, buffer, 10);
    buffer[5] = 0;
    strcpy_s(&(fen[a]), 5, buffer);
    return fen;
}

void printBoard(int showFEN, int showBoard, Board *board, char *pieceSymbols) {
    char* boardString = boardToFEN(board, pieceSymbols);
    if (showFEN) printf("FEN: %s\n", boardString);
    if (!showBoard) return;

    int i = 0;

    printf("To move: %s\n-------------------------------------------------\n", (board->playerToMove)? "black" : "white");
    for (int j = 0; j < 8; j++) {
        while (boardString[i] != '/' && boardString[i] != ' ') {
            if (isalpha(boardString[i])) {
                printf("|  %c  ", boardString[i]);
                i++;
            }
            else if (boardString[i] == '0') {
                i++;
            }
            else {
                printf("|     ");
                boardString[i]--;
            }
        }
        printf("|\n-------------------------------------------------\n");
        i++;
    }

    free(boardString);
}

void printSquareBasedBoard(Board* board) {
    for (int i = 7; i >= 0; i--) {
        for (int j = 7; j >= 0; j--) {
            printf("%d\t", board->boardBySquare[8 * i + j]);
        }
        printf("\n");
    }
    printf("\n");
}

/*
    Move types:

    capture?	Promotion?	Flag1?	Flag2?	Type of move
    0			0			0		0		quiet
    0			0			0		1		double pawn push
    0			0			1		0		king side castle
    0			0			1		1		queen side castle
    0			1			0		0		promotion to knight
    0			1			0		1		promotion to bishop
    0			1			1		0		promotion to rook
    0			1			1		1		promotion to queen
    1			0			0		0		normal capture
    1			0			0		1		UN-USED
    1			0			1		0		en-passant capture
    1			0			1		1		UN-USED
    1			1			0		0		capture promotion to knight
    1			1			0		1		capture promotion to bishop
    1			1			1		0		capture promotion to rook
    1			1			1		1		capture promotion to queen
*/
unsigned long long int formMove(int from, int to,
    int piece, int cPiece,
    bool isPromotion,
    bool f1, bool f2,
    bool K, bool Q, bool k, bool q,
    int epSquare, int halfMoveClock) {

    unsigned long long int move = ((from & 0b111111ULL) << 31) |
        ((to & 0b111111ULL) << 25) |
        ((piece & 0b111ULL) << 22) |
        ((cPiece & 0b111ULL) << 19) |
        ((isPromotion & 0b1ULL) << 18) |
        ((f1 & 0b1ULL) << 17) |
        ((f2 & 0b1ULL) << 16) |
        ((K & 0b1ULL) << 15) |
        ((Q & 0b1ULL) << 14) |
        ((k & 0b1ULL) << 13) |
        ((q & 0b1ULL) << 12) |
        ((epSquare & 0b111111ULL) << 6) |
        (halfMoveClock & 0b111111ULL);

    return move;
}

int getFrom(unsigned long long int move) {
    return (int)((move >> 31) & 0b111111ULL);
}
int getTo(unsigned long long int move) {
    return (int)((move >> 25) & 0b111111ULL);
}
int getPiece(unsigned long long int move) {
    return (int)((move >> 22) & 0b111ULL);
}
int getCPiece(unsigned long long int move) {
    return (int)((move >> 19) & 0b111ULL);
}
bool getIsPromotion(unsigned long long int move) {
    return (int)((move >> 18) & 0b1ULL);
}
bool getF1(unsigned long long int move) {
    return (int)((move >> 17) & 0b1ULL);
}
bool getF2(unsigned long long int move) {
    return (int)((move >> 16) & 0b1ULL);
}
bool getK(unsigned long long int move) {
    return (int)((move >> 15) & 0b1ULL);
}
bool getQ(unsigned long long int move) {
    return (int)((move >> 14) & 0b1ULL);
}
bool getk(unsigned long long int move) {
    return (int)((move >> 13) & 0b1ULL);
}
bool getq(unsigned long long int move) {
    return (int)((move >> 12) & 0b1ULL);
}
int getEpSquare(unsigned long long int move) {
    return (int)((move >> 6) & 0b111111ULL);
}
int getHalfMoveClock(unsigned long long int move) {
    return (int)(move & 0b111111ULL);
}

// Takes move inf the form fftt (e.g. e2e4) and the board it is played on, turns it into a move
unsigned long long int textToMove(char *moveText, Board *board) {
    int from = 8 * (moveText[1] - '1') + ('h' - moveText[0]);
    int to = 8 * (moveText[3] - '1') + ('h' - moveText[2]);
    unsigned long long int fromSquare = 1ULL << from;
    unsigned long long int toSquare = 1ULL << to;

    int piece = board->boardBySquare[from];
    
    bool isEpCapture = false;
    unsigned long long int cPiece = board->pieceBB[(1 - board->playerToMove) * 7] & toSquare;
    if (cPiece) {
        cPiece = board->boardBySquare[to];
    }
    else if (piece == 1 && to == board->epSquare) { // Look for ep capture
        isEpCapture = true;
        cPiece = 1;
    }

    bool isDoublePush = (piece == 1 && (from - to == 16 || to - from == 16));

    bool isCastle = (piece == 6 && (from - to == 2 || to - from == 2));
    bool castleSide = (from - to) < 0;
    
    bool isPromotion = (piece == 1) && ((board->playerToMove) ? (to < 8) : (to > 55));

    bool f1 = false, f2 = false;
    if (isPromotion) {
        char buffer[2] = { '\0' };
        printf("Which promotion?\nQueen = q, Rook = r, Bishop = b, Knight = n, otherwise defaults to queen\n> ");
        gets_s(buffer, sizeof(buffer));
        switch (buffer[0]) {
            default:
            case 'q':
                f1 = true;
                f2 = true;
                break;
            case 'r':
                f1 = true;
                f2 = false;
                break;
            case 'b':
                f1 = false;
                f2 = true;
                break;
            case 'n':
                f1 = false;
                f2 = false;
        }
    }
    else if (isEpCapture) {
        f1 = true;
        f2 = false;
    }
    else if (isCastle) {
        f1 = true;
        f2 = castleSide;
    }
    else if (isDoublePush) {
        f1 = false;
        f2 = true;
    }

    int temp;
    BitScanForward64(&temp, board->epSquare);
    return formMove(from, to, piece, cPiece, isPromotion, f1, f2, 
        board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], 
        board->castlingRights[3], temp, board->halfMoveClock);
}

void moveToText(char *moveText, unsigned long long int move) {
    char rank[8] = {'h', 'g', 'f', 'e', 'd', 'c', 'b', 'a'};
    char file[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    int from = getFrom(move);
    int to = getTo(move);
    moveText[0] = rank[from % 8];
    moveText[1] = file[from / 8];
    moveText[2] = rank[to % 8];
    moveText[3] = file[to / 8];
}

void makeMove(Board *board, unsigned long long int move) {
    int fromIndex = getFrom(move);
    int toIndex = getTo(move);
    unsigned long long int from = 1ULL << fromIndex;
    unsigned long long int to = 1ULL << toIndex;
    int piece = getPiece(move);
    int cPiece = getCPiece(move);
    int color = board->playerToMove;
    int color7 = color * 7;

    // Update castling rights
    if (to == 0x0000000000000001ULL || from == 0x0000000000000001ULL) board->castlingRights[0] = false;
    if (to == 0x0000000000000080ULL || from == 0x0000000000000080ULL) board->castlingRights[1] = false;
    if (to == 0x0100000000000000ULL || from == 0x0100000000000000ULL) board->castlingRights[2] = false;
    if (to == 0x8000000000000000ULL || from == 0x8000000000000000ULL) board->castlingRights[3] = false;
    if (piece == 6) {
        board->castlingRights[(color << 1)] = false;
        board->castlingRights[(color << 1) + 1] = false;
    }

    if (!cPiece && !getIsPromotion(move) && getF1(move)) { // Castling is special
        if (getF2(move)) { // Queenside
            if (color == 0) {
                board->pieceBB[6] ^= 0x0000000000000028L;
                board->pieceBB[0] ^= 0x0000000000000028L;
                board->pieceBB[4] ^= 0x0000000000000090L;
                board->pieceBB[0] ^= 0x0000000000000090L;
                board->emptyBB ^= 0x00000000000000B8L;
                board->occupiedBB ^= 0x00000000000000B8L;

                board->boardBySquare[7] = 0;
                board->boardBySquare[5] = 6;
                board->boardBySquare[4] = 4;
                board->boardBySquare[3] = 0;
            }
            else {
                board->pieceBB[13] ^= 0x2800000000000000L;
                board->pieceBB[7] ^= 0x2800000000000000L;
                board->pieceBB[11] ^= 0x9000000000000000L;
                board->pieceBB[7] ^= 0x9000000000000000L;
                board->emptyBB ^= 0xB800000000000000L;
                board->occupiedBB ^= 0xB800000000000000L;

                board->boardBySquare[63] = 0;
                board->boardBySquare[61] = 6;
                board->boardBySquare[60] = 4;
                board->boardBySquare[59] = 0;
            }
        }
        else { // Kingside
            if (color == 0) {
                board->pieceBB[6] ^= 0x000000000000000AL;
                board->pieceBB[0] ^= 0x000000000000000AL;
                board->pieceBB[4] ^= 0x0000000000000005L;
                board->pieceBB[0] ^= 0x0000000000000005L;
                board->emptyBB ^= 0x000000000000000FL;
                board->occupiedBB ^= 0x000000000000000FL;

                board->boardBySquare[0] = 0;
                board->boardBySquare[1] = 6;
                board->boardBySquare[2] = 4;
                board->boardBySquare[3] = 0;
            }
            else {
                board->pieceBB[13] ^= 0x0A00000000000000L;
                board->pieceBB[7] ^= 0x0A00000000000000L;
                board->pieceBB[11] ^= 0x0500000000000000L;
                board->pieceBB[7] ^= 0x0500000000000000L;
                board->emptyBB ^= 0x0F00000000000000L;
                board->occupiedBB ^= 0x0F00000000000000L;

                board->boardBySquare[56] = 0;
                board->boardBySquare[57] = 6;
                board->boardBySquare[58] = 4;
                board->boardBySquare[59] = 0;
            }
        }
    }
    else {
        // Remove piece at from
        board->pieceBB[color7 + piece] &= ~from;
        board->pieceBB[color7] &= ~from;
        board->occupiedBB &= ~from;
        board->emptyBB |= from;

        board->boardBySquare[fromIndex] = 0;


        // Remove catured piece at to / ep special case
        if (cPiece == 1 && getF1(move) && !getIsPromotion(move)) {
            unsigned long long int temp = (color) ? (to << 8) : (to >> 8);
            board->pieceBB[7 - color7 + cPiece] &= ~temp;
            board->pieceBB[7 - color7] &= ~temp;
            board->occupiedBB &= ~temp;
            board->emptyBB |= temp;

            board->boardBySquare[(color) ? toIndex + 8 : toIndex - 8] = 0;
        }
        else if (cPiece) {
            board->pieceBB[7 - color7 + cPiece] &= ~to;
            board->pieceBB[7 - color7] &= ~to;
            board->occupiedBB &= ~to;
            board->emptyBB |= to;

            board->boardBySquare[toIndex] = 0;
        }

        // Place piece from from at to / promotion special case
        if (getIsPromotion(move)) {
            if (getF1(move)) {
                if (getF2(move)) {
                    piece = 5;
                }
                else {
                    piece = 4;
                }
            }
            else {
                if (getF2(move)) {
                    piece = 3;
                }
                else {
                    piece = 2;
                }
            }
        }
        board->pieceBB[color7 + piece] |= to;
        board->pieceBB[color7] |= to;
        board->occupiedBB |= to;
        board->emptyBB &= ~to;

        board->boardBySquare[toIndex] = piece;
    }

    // Update player to move, ep, and clocks
    board->fullMoveNumber += color;
    board->playerToMove = !board->playerToMove;
    board->epSquare = (!getIsPromotion(move) && !getF1(move) && getF2(move)) ? (board->epSquare = (color) ? (to << 8) : (to >> 8)) : 0;
    if (cPiece || piece == 1 || 
        board->castlingRights[0] != getK(move) || 
        board->castlingRights[1] != getQ(move) || 
        board->castlingRights[2] != getk(move) || 
        board->castlingRights[3] != getq(move))
    {
        board->halfMoveClock = 0;
    }
    else {
        board->halfMoveClock++;
    }

    // Add to history of moves. Record the game to be able to undo, and just keep track of how the game went.
    addMove(&(board->history), move);
}

void unmakeMove(Board *board, unsigned long long int move) {
    int fromIndex = getFrom(move);
    int toIndex = getTo(move);
    unsigned long long int from = 1ULL << fromIndex;
    unsigned long long int to = 1ULL << toIndex;
    int piece = getPiece(move);
    int cPiece = getCPiece(move);
    board->playerToMove = !board->playerToMove;
    board->fullMoveNumber -= board->playerToMove;
    int color = board->playerToMove; // color from perspective of the player who made the move
    int color7 = color * 7;

    // Recover history information from the move to restore otherwise irreversible changes
    board->epSquare = (getEpSquare(move)) ? (1ULL << getEpSquare(move)) : 0;
    board->halfMoveClock = getHalfMoveClock(move);
    board->castlingRights[0] = getK(move);
    board->castlingRights[1] = getQ(move);
    board->castlingRights[2] = getk(move);
    board->castlingRights[3] = getq(move);

    // Normally reversible parts of moves
    if (!cPiece && !getIsPromotion(move) && getF1(move)) { // Castling is special
        if (getF2(move)) { // Queenside
            if (color == 0) {
                board->pieceBB[6] ^= 0x0000000000000028L;
                board->pieceBB[0] ^= 0x0000000000000028L;
                board->pieceBB[4] ^= 0x0000000000000090L;
                board->pieceBB[0] ^= 0x0000000000000090L;
                board->emptyBB ^= 0x00000000000000B8L;
                board->occupiedBB ^= 0x00000000000000B8L;

                board->boardBySquare[7] = 4;
                board->boardBySquare[5] = 0;
                board->boardBySquare[4] = 0;
                board->boardBySquare[3] = 6;
            }
            else {
                board->pieceBB[13] ^= 0x2800000000000000L;
                board->pieceBB[7] ^= 0x2800000000000000L;
                board->pieceBB[11] ^= 0x9000000000000000L;
                board->pieceBB[7] ^= 0x9000000000000000L;
                board->emptyBB ^= 0xB800000000000000L;
                board->occupiedBB ^= 0xB800000000000000L;

                board->boardBySquare[63] = 4;
                board->boardBySquare[61] = 0;
                board->boardBySquare[60] = 0;
                board->boardBySquare[59] = 6;
            }
        }
        else { // Kingside
            if (color == 0) {
                board->pieceBB[6] ^= 0x000000000000000AL;
                board->pieceBB[0] ^= 0x000000000000000AL;
                board->pieceBB[4] ^= 0x0000000000000005L;
                board->pieceBB[0] ^= 0x0000000000000005L;
                board->emptyBB ^= 0x000000000000000FL;
                board->occupiedBB ^= 0x000000000000000FL;

                board->boardBySquare[0] = 4;
                board->boardBySquare[1] = 0;
                board->boardBySquare[2] = 0;
                board->boardBySquare[3] = 6;
            }
            else {
                board->pieceBB[13] ^= 0x0A00000000000000L;
                board->pieceBB[7] ^= 0x0A00000000000000L;
                board->pieceBB[11] ^= 0x0500000000000000L;
                board->pieceBB[7] ^= 0x0500000000000000L;
                board->emptyBB ^= 0x0F00000000000000L;
                board->occupiedBB ^= 0x0F00000000000000L;

                board->boardBySquare[56] = 4;
                board->boardBySquare[57] = 0;
                board->boardBySquare[58] = 0;
                board->boardBySquare[59] = 6;
            }
        }
    }
    else {
        // Remove peice at to
        board->pieceBB[color7 + piece] &= ~to;
        board->pieceBB[color7] &= ~to;
        board->occupiedBB &= ~to;
        board->emptyBB |= to;

        board->boardBySquare[toIndex] = 0;

        // replace captured piece at to / exception for ep capture
        if (cPiece == 1 && getF1(move) && !getIsPromotion(move)) {
            unsigned long long int temp = (color) ? (to << 8) : (to >> 8);
            board->pieceBB[7 - color7 + cPiece] |= temp;
            board->pieceBB[7 - color7] |= temp;
            board->occupiedBB |= temp;
            board->emptyBB &= ~temp;

            board->boardBySquare[(color) ? (toIndex + 8) : (toIndex - 8)] = cPiece;
        }
        else if (cPiece) {
            board->pieceBB[7 - color7 + cPiece] |= to;
            board->pieceBB[7 - color7] |= to;
            board->occupiedBB |= to;
            board->emptyBB &= ~to;

            board->boardBySquare[toIndex] = cPiece;
        }

        // replace piece at from / exeption for promotion
        piece = (getIsPromotion(move)) ? 1 : piece;
        board->pieceBB[color7 + piece] |= from;
        board->pieceBB[color7] |= from;
        board->occupiedBB |= from;
        board->emptyBB &= ~from;

        board->boardBySquare[fromIndex] = piece;
    }

    // remove the move being unmade from the history record
    removeLastMove(&(board->history));
}

void unmakeLastMove(Board *board) {
    unmakeMove(board, board->history.moves[board->history.length - 1]);
}

void removeOrPlacePiece(Board *board, unsigned long long int square, int piece, int color) {
    int color7 = color * 7;
    board->pieceBB[color7 + piece] ^= square;
    board->pieceBB[color7] ^= square;
    board->occupiedBB ^= square;
    board->emptyBB ^= square;
}

// Generate rays from squares to blockers / edge of board in specific directions, including blocker but not origin square
// can generate for set of like pieces at the same time (rooks and queens, bishops and queens)
unsigned long long int ul(unsigned long long int square, unsigned long long int empty) {
    unsigned long long int upLeft = 0;
    do {
        square = (square << 9) & notHFile;
        upLeft |= square;
    } while (square &= empty);
    return upLeft;
}
unsigned long long int u(unsigned long long int square, unsigned long long int empty) {
    unsigned long long int up = 0;
    do {
        square = square << 8;
        up |= square;
    } while (square &= empty);
    return up;
}
unsigned long long int ur(unsigned long long int square, unsigned long long int empty) {
    unsigned long long int upRight = 0;
    do {
        square = (square << 7) & notAFile;
        upRight |= square;
    } while (square &= empty);
    return upRight;
}
unsigned long long int l(unsigned long long int square, unsigned long long int empty) {
    unsigned long long int left = 0;
    do {
        square = (square << 1) & notHFile;
        left |= square;
    } while (square &= empty);
    return left;
}
unsigned long long int r(unsigned long long int square, unsigned long long int empty) {
    unsigned long long int right = 0;
    do {
        square = (square >> 1) & notAFile;
        right |= square;
    } while (square &= empty);
    return right;
}
unsigned long long int dl(unsigned long long int square, unsigned long long int empty) {
    unsigned long long int downLeft = 0;
    do {
        square = (square >> 7) & notHFile;
        downLeft |= square;
    } while (square &= empty);
    return downLeft;
}
unsigned long long int d(unsigned long long int square, unsigned long long int empty) {
    unsigned long long int down = 0;
    do {
        square = square >> 8;
        down |= square;
    } while (square &= empty);
    return down;
}
unsigned long long int dr(unsigned long long int square, unsigned long long int empty) {
    unsigned long long int downRight = 0;
    do {
        square = (square >> 9) & notAFile;
        downRight |= square;
    } while (square &= empty);
    return downRight;
}

unsigned long long int squaresSeen(unsigned long long int empty, unsigned long long int square, unsigned long long int piece, int color) {
    unsigned long long int seen = 0;

    switch (piece) {
    case 1:
        // Only the pawn attack squares. Pawns are handled specially for pushes
        if (color) { // black
            seen |= (square >> 7) & notHFile;
            seen |= (square >> 9) & notAFile;
        }
        else { // white
            seen |= (square << 9) & notHFile;
            seen |= (square << 7) & notAFile;
        }
        break;
    case 2:
        seen |= (square << 17) & notHFile;
        seen |= (square << 15) & notAFile;
        seen |= ((square & notAFile) << 10) & notHFile;
        seen |= ((square & notHFile) << 6) & notAFile;
        seen |= ((square & notAFile) >> 6) & notHFile;
        seen |= ((square & notHFile) >> 10) & notAFile;
        seen |= (square >> 15) & notHFile;
        seen |= (square >> 17) & notAFile;
        break;
    case 3:
        seen |= ul(square, empty);
        seen |= ur(square, empty);
        seen |= dl(square, empty);
        seen |= dr(square, empty);
        break;
    case 4:
        seen |= u(square, empty);
        seen |= r(square, empty);
        seen |= d(square, empty);
        seen |= l(square, empty);
        break;
    case 5:
        seen |= ul(square, empty);
        seen |= ur(square, empty);
        seen |= dl(square, empty);
        seen |= dr(square, empty);
        seen |= u(square, empty);
        seen |= r(square, empty);
        seen |= d(square, empty);
        seen |= l(square, empty);
        break;
    }
    return seen;
}

// Pupulates the given empty bitboards with the correct masks
void makePushAndCaptureMask(Board *board, unsigned long long int *pushCapMask) {
    int opp = 7 * !board->playerToMove;
    int king = board->playerToMove * 7 + 6;
    unsigned long long int attackingKing;

    attackingKing  = squaresSeen(board->emptyBB, board->pieceBB[king], 1, board->playerToMove) & board->pieceBB[opp + 1];
    attackingKing |= squaresSeen(board->emptyBB, board->pieceBB[king], 2, board->playerToMove) & board->pieceBB[opp + 2];
    attackingKing |= squaresSeen(board->emptyBB, board->pieceBB[king], 3, board->playerToMove) & (board->pieceBB[opp + 3] | board->pieceBB[opp + 5]);
    attackingKing |= squaresSeen(board->emptyBB, board->pieceBB[king], 4, board->playerToMove) & (board->pieceBB[opp + 4] | board->pieceBB[opp + 5]);

    int count = __popcnt64(attackingKing);
    if (!count) {
        *pushCapMask = 0xFFFFFFFFFFFFFFFFULL;
        return;
    }
    if (count == 1) {
        *pushCapMask = attackingKing;
        unsigned long int tzcKing, tzcAttacker, temp;
        unsigned int rk, fk, ra, fa;
        bool reverse = false;
        BitScanForward64(&tzcKing, board->pieceBB[king]);
        BitScanForward64(&tzcAttacker, attackingKing);
        if (tzcKing > tzcAttacker) reverse = true;
        rk = tzcKing >> 3;
        fk = tzcKing & 7;
        ra = tzcAttacker >> 3;
        fa = tzcAttacker & 7;
        if (rk + fk == ra + fa) {
            *pushCapMask |= ur((reverse) ? attackingKing : board->pieceBB[king], board->emptyBB);
        }
        else if (rk - fk == ra - fa) {
            *pushCapMask |= ul((reverse) ? attackingKing : board->pieceBB[king], board->emptyBB);
        }
        else if (rk == ra) {
            *pushCapMask |= l((reverse) ? attackingKing : board->pieceBB[king], board->emptyBB);
        }
        else if (fk == fa) {
            *pushCapMask |= u((reverse) ? attackingKing : board->pieceBB[king], board->emptyBB);
        }
    }
}

bool inCheck(Board *board, int color) {
    unsigned long long int opponentAttacks = 0;
    int king = color * 7 + 6, opp = (1 - color) * 7;
    unsigned long long int bAndQ = board->pieceBB[opp + 3] | board->pieceBB[opp + 5];
    unsigned long long int rAndQ = board->pieceBB[opp + 4] | board->pieceBB[opp + 5];
    unsigned long long int emptyKingRemoved = board->emptyBB ^ board->pieceBB[king];

    unsigned long long int unsafeSquares;

    unsafeSquares = squaresSeen(emptyKingRemoved, board->pieceBB[opp + 1], 1, opp);
    unsafeSquares |= squaresSeen(emptyKingRemoved, board->pieceBB[opp + 2], 2, opp);
    unsafeSquares |= squaresSeen(emptyKingRemoved, bAndQ, 3, opp);
    unsafeSquares |= squaresSeen(emptyKingRemoved, rAndQ, 4, opp);

    return (unsafeSquares & board->pieceBB[king]) != 0;
}

// Legal moves only are added to move list
void generateMoves(MoveList *ml, Board *board) {
    unsigned long long int pushCapMask = 0;
    makePushAndCaptureMask(board, &pushCapMask);
    int epSquareIndex = 0;
    if (board->epSquare) BitScanForward64(&epSquareIndex, board->epSquare);
    // Generate king moves
    int self = board->playerToMove * 7;
    int opp = 7 - self;
    int king = self + 6;
    unsigned long long int bAndQ = board->pieceBB[opp + 3] | board->pieceBB[opp + 5];
    unsigned long long int rAndQ = board->pieceBB[opp + 4] | board->pieceBB[opp + 5];
    unsigned long long int unsafeSquares;
    // Find the squares king can't move to
    unsafeSquares  = squaresSeen(board->emptyBB ^ board->pieceBB[king], board->pieceBB[opp + 1], 1, opp);
    unsafeSquares |= squaresSeen(board->emptyBB ^ board->pieceBB[king], board->pieceBB[opp + 2], 2, opp);
    unsafeSquares |= squaresSeen(board->emptyBB ^ board->pieceBB[king], bAndQ, 3, opp);
    unsafeSquares |= squaresSeen(board->emptyBB ^ board->pieceBB[king], rAndQ, 4, opp);
    // Pseudo-legal non-castling king moves
    unsigned long long int kingDestinations = (board->pieceBB[king] << 1) & notHFile;
    kingDestinations |= (kingDestinations >> 2) & notAFile;
    kingDestinations |= kingDestinations >> 8;
    kingDestinations |= (kingDestinations | board->pieceBB[king]) << 8;
    kingDestinations |= board->pieceBB[king] >> 8;
    // Get rid of the ones to unsafe squares and those occupied by ones own pieces
    kingDestinations = ~board->pieceBB[self] & ~unsafeSquares & kingDestinations;
    // Enter non-castling legal king moves into move list
    int squareIndex, targetSquareIndex, cPiece, kingSquareIndex;
    BitScanForward64(&kingSquareIndex, board->pieceBB[king]);
    if (kingDestinations) do {
        BitScanForward64(&squareIndex, kingDestinations);
        cPiece = board->boardBySquare[squareIndex];
        
        addMove(ml, formMove(kingSquareIndex, squareIndex, 6, cPiece, false, false, false,
            board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
            epSquareIndex, board->halfMoveClock));
    } while (kingDestinations &= kingDestinations - 1);
    // Generate legal castling moves
    if (board->playerToMove) { // Black castling
        if (board->castlingRights[2] && !(unsafeSquares & 0x0E00000000000000ULL) && !(board->occupiedBB & 0x0600000000000000ULL)) {
            addMove(ml, formMove(59, 57, 6, 0, false, true, false,
                board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                epSquareIndex, board->halfMoveClock));
        }
        if (board->castlingRights[3] && !(unsafeSquares & 0x3800000000000000ULL) && !(board->occupiedBB & 0x7000000000000000ULL)) {
            addMove(ml, formMove(59, 61, 6, 0, false, true, true,
                board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                epSquareIndex, board->halfMoveClock));
        }
    }
    else { // white castling
        if (board->castlingRights[0] && !(unsafeSquares & 0x000000000000000EULL) && !(board->occupiedBB & 0x0000000000000006ULL)) {
            addMove(ml, formMove(3, 1, 6, 0, false, true, false,
                board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                epSquareIndex, board->halfMoveClock));
        }
        if (board->castlingRights[1] && !(unsafeSquares & 0x0000000000000038ULL) && !(board->occupiedBB & 0x0000000000000070ULL)) {
            addMove(ml, formMove(3, 5, 6, 0, false, true, true,
                board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                epSquareIndex, board->halfMoveClock));
        }
    }
    // Set bitboard of pinned pieces for player to move
    // Shoot ray from slider pieces and from king in opposite difections, intersection of these rays gives pinned pieces and possible discovered checks.
    // It also gives a line from the king to a checker if the king is in check, which is okay since any of those squares are
    // empty and intersecting with piece set gets rid of those.
    // Intersecting pinned with one's pieces will give which of them are pinned pieces.
    unsigned long long int pinned = 0, free, tempP, tempF;
    pinned |= ur(board->pieceBB[king], board->emptyBB) & dl(bAndQ, board->emptyBB);
    pinned |= ul(board->pieceBB[king], board->emptyBB) & dr(bAndQ, board->emptyBB);
    pinned |= dl(board->pieceBB[king], board->emptyBB) & ur(bAndQ, board->emptyBB);
    pinned |= dr(board->pieceBB[king], board->emptyBB) & ul(bAndQ, board->emptyBB);
    pinned |= u(board->pieceBB[king], board->emptyBB) & d(rAndQ, board->emptyBB);
    pinned |= d(board->pieceBB[king], board->emptyBB) & u(rAndQ, board->emptyBB);
    pinned |= r(board->pieceBB[king], board->emptyBB) & l(rAndQ, board->emptyBB);
    pinned |= l(board->pieceBB[king], board->emptyBB) & r(rAndQ, board->emptyBB);
    free = ~pinned;
    // By only do extra legality checks on pieces which are pinned, time is saved vs checking
    // for pin on every piece individually and applying a mask to where it can move

    // Generate Pawn Moves
    tempF = board->pieceBB[self + 1] & free;
    tempP = board->pieceBB[self + 1] & pinned;
    unsigned long long int square;
    if (tempF) do {
        BitScanForward64(&squareIndex, tempF);
        square = 1ULL << squareIndex;
        unsigned long long int move;
        // Generate ep captures
        // I don't need to apply push or capture mask, since it will look for check after making the move anyway
        if (board->playerToMove && board->epSquare && ((((square >> 7) & notHFile) | ((square >> 9) & notAFile)) & board->epSquare)) { // Black ep
            // Generate ep move
            move = formMove(squareIndex, epSquareIndex, 1, 1, false, true, false,
                board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                epSquareIndex, board->halfMoveClock);
            // legality check
            makeMove(board, move);
            // Look for check
            bool isLegal = !inCheck(board, !board->playerToMove);
            unmakeMove(board, move);
            // If doesn't put king in check add the move to the list
            if (isLegal) addMove(ml, move);
        }
        else if (board->epSquare && ((((square << 9) & notHFile) | ((square << 7) & notAFile)) & board->epSquare)) { // White ep            
            // Generate ep move
            // I don't need to apply push or capture mask, since it will look for check after making the move anyway
            move = formMove(squareIndex, epSquareIndex, 1, 1, false, true, false,
                board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                epSquareIndex, board->halfMoveClock);
            // legality check
            makeMove(board, move);
            // Look for check
            bool isLegal = !inCheck(board, !board->playerToMove);
            unmakeMove(board, move);
            // If doesn't put king in check add the move to the list
            if (isLegal) addMove(ml, move);
        }
        // non-ep moves for the pawn
        unsigned long long int captures = ((board->playerToMove) ? (
              ((square >> 7) & notHFile) | ((square >> 9) & notAFile)) 
            : (((square << 9) & notHFile) | ((square << 7) & notAFile))
            ) & board->pieceBB[opp];
        unsigned long long int push = ((board->playerToMove) ? square >> 8 : square << 8) & board->emptyBB;
        unsigned long long int doublePush = ((board->playerToMove) ? (push & 0x0000FF0000000000ULL) >> 8 : (push & 0x0000000000FF0000ULL) << 8) & board->emptyBB;
        doublePush &= pushCapMask;
        unsigned long long int targets = (push | captures) & pushCapMask;
        if (doublePush) {
            BitScanForward64(&targetSquareIndex, doublePush);
            addMove(ml, formMove(squareIndex, targetSquareIndex, 1, board->boardBySquare[targetSquareIndex], false, false, true,
                board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                epSquareIndex, board->halfMoveClock));
        }
        if (targets) do {
            BitScanForward64(&targetSquareIndex, targets);
            bool isPromotion =
                (board->playerToMove && (targetSquareIndex < 8))
                || ((!board->playerToMove) && (targetSquareIndex > 55));
            if (isPromotion) {
                addMove(ml, formMove(squareIndex, targetSquareIndex, 1, board->boardBySquare[targetSquareIndex], true, false, false,
                    board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                    epSquareIndex, board->halfMoveClock));
                addMove(ml, formMove(squareIndex, targetSquareIndex, 1, board->boardBySquare[targetSquareIndex], true, false, true,
                    board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                    epSquareIndex, board->halfMoveClock));
                addMove(ml, formMove(squareIndex, targetSquareIndex, 1, board->boardBySquare[targetSquareIndex], true, true, false,
                    board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                    epSquareIndex, board->halfMoveClock));
                addMove(ml, formMove(squareIndex, targetSquareIndex, 1, board->boardBySquare[targetSquareIndex], true, true, true,
                    board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                    epSquareIndex, board->halfMoveClock));
            }
            else {
                addMove(ml, formMove(squareIndex, targetSquareIndex, 1, board->boardBySquare[targetSquareIndex], false, false, false,
                    board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                    epSquareIndex, board->halfMoveClock));
            }
        } while (targets &= targets - 1);
    } while (tempF &= tempF - 1);

    // pinned pawn moves. Same but extra check for legality after generating before adding to the list for all of them, not just ep
    if (tempP) do {
        BitScanForward64(&squareIndex, tempP);
        square = 1ULL << squareIndex;
        unsigned long long int move;
        // Generate ep captures
        // I don't need to apply push or capture mask, since it will look for check after making the move anyway
        if (board->playerToMove && board->epSquare && ((((square >> 7) & notHFile) | ((square >> 9) & notAFile)) & board->epSquare)) { // Black ep
            // Generate ep move
            move = formMove(squareIndex, epSquareIndex, 1, 1, false, true, false,
                board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                epSquareIndex, board->halfMoveClock);
            // legality check
            makeMove(board, move);
            // Look for check
            bool isLegal = !inCheck(board, !board->playerToMove);
            unmakeMove(board, move);
            // If doesn't put king in check add the move to the list
            if (isLegal) addMove(ml, move);
        }
        else if (board->epSquare && ((((square << 9) & notHFile) | ((square << 7) & notAFile)) & board->epSquare)) { // White ep
            // Generate ep move
            // I don't need to apply push or capture mask, since it will look for check after making the move anyway
            move = formMove(squareIndex, epSquareIndex, 1, 1, false, true, false,
                board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                epSquareIndex, board->halfMoveClock);
            // legality check
            makeMove(board, move);
            // Look for check
            bool isLegal = !inCheck(board, !board->playerToMove);
            unmakeMove(board, move);
            // If doesn't put king in check add the move to the list
            if (isLegal) addMove(ml, move);
        }
        // non-ep moves for the pawn
        unsigned long long int captures = ((board->playerToMove) ? (
            ((square >> 7) & notHFile) | ((square >> 9) & notAFile))
            : (((square << 9) & notHFile) | ((square << 7) & notAFile))
            ) & board->pieceBB[opp];
        unsigned long long int push = ((board->playerToMove) ? square >> 8 : square << 8) & board->emptyBB;
        unsigned long long int doublePush = ((board->playerToMove) ? (push & 0x0000FF0000000000ULL) >> 8 : (push & 0x0000000000FF0000ULL) << 8) & board->emptyBB;
        unsigned long long int targets = (push | captures) & pushCapMask;
        if (doublePush) {
            BitScanForward64(&targetSquareIndex, doublePush);
            move = formMove(squareIndex, targetSquareIndex, 1, board->boardBySquare[targetSquareIndex], false, false, true,
                board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                epSquareIndex, board->halfMoveClock);
            // legality check
            makeMove(board, move);
            // Look for check
            bool isLegal = !inCheck(board, !board->playerToMove);
            unmakeMove(board, move);
            // If doesn't put king in check add the move to the list
            if (isLegal) addMove(ml, move);
        }
        if (targets) do {
            BitScanForward64(&targetSquareIndex, targets);
            bool isPromotion =
                (board->playerToMove && (targetSquareIndex < 8))
                || ((!board->playerToMove) && (targetSquareIndex > 55));
            if (isPromotion) {
                move = formMove(squareIndex, targetSquareIndex, 1, board->boardBySquare[targetSquareIndex], true, false, false,
                    board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                    epSquareIndex, board->halfMoveClock);
                // legality check
                makeMove(board, move);
                // Look for check
                bool isLegal = !inCheck(board, !board->playerToMove);
                unmakeMove(board, move);
                // If doesn't put king in check add the move to the list
                if (isLegal) {
                    addMove(ml, move);
                    addMove(ml, formMove(squareIndex, targetSquareIndex, 1, board->boardBySquare[targetSquareIndex], true, false, true,
                        board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                        epSquareIndex, board->halfMoveClock));
                    addMove(ml, formMove(squareIndex, targetSquareIndex, 1, board->boardBySquare[targetSquareIndex], true, true, false,
                        board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                        epSquareIndex, board->halfMoveClock));
                    addMove(ml, formMove(squareIndex, targetSquareIndex, 1, board->boardBySquare[targetSquareIndex], true, true, true,
                        board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                        epSquareIndex, board->halfMoveClock));
                }
            }
            else {
                move = formMove(squareIndex, targetSquareIndex, 1, board->boardBySquare[targetSquareIndex], false, false, false,
                    board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                    epSquareIndex, board->halfMoveClock);
                // legality check
                makeMove(board, move);
                // Look for check
                bool isLegal = !inCheck(board, !board->playerToMove);
                unmakeMove(board, move);
                // If doesn't put king in check add the move to the list
                if (isLegal) addMove(ml, move);
            }
        } while (targets &= targets - 1);
    } while (tempP &= tempP - 1);

    // Generate other pieces's moves
    for (int piece = 2; piece <= 5; piece++) {
        tempF = board->pieceBB[self + piece] & free;
        tempP = board->pieceBB[self + piece] & pinned;

        // Free pieces
        if (tempF) do {
            BitScanForward64(&squareIndex, tempF);
            square = 1ULL << squareIndex;
            unsigned long long int targets = squaresSeen(board->emptyBB, square, piece, board->playerToMove);
            targets &= pushCapMask;
            targets &= ~board->pieceBB[self];
            if (targets) do {
                BitScanForward64(&targetSquareIndex, targets);
                addMove(ml, formMove(squareIndex, targetSquareIndex, piece, board->boardBySquare[targetSquareIndex], false, false, false,
                    board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                    epSquareIndex, board->halfMoveClock));
            } while (targets &= targets - 1);
        } while (tempF &= tempF - 1);

        // Pinned pieces
        if (tempP) do {
            BitScanForward64(&squareIndex, tempP);
            square = 1ULL << squareIndex;
            unsigned long long int targets = squaresSeen(board->emptyBB, square, piece, board->playerToMove);
            targets &= ~board->pieceBB[self];
            targets &= pushCapMask;

            unsigned long long int pinnedMask = 0;
            // remove piece from board temporarily, find push/capture mask without the pinned piece, that is now the pin mask for that piece
            // as long as the piece stays within the masked tiles there is no need to look for check before adding the move to the list.
            removeOrPlacePiece(board, square, piece, board->playerToMove);
            makePushAndCaptureMask(board, &pinnedMask);
            removeOrPlacePiece(board, square, piece, board->playerToMove);
            targets &= pinnedMask;

            if (targets) do {
                BitScanForward64(&targetSquareIndex, targets);
                BitScanForward64(&targetSquareIndex, targets);
                addMove(ml, formMove(squareIndex, targetSquareIndex, piece, board->boardBySquare[targetSquareIndex], false, false, false,
                    board->castlingRights[0], board->castlingRights[1], board->castlingRights[2], board->castlingRights[3],
                    epSquareIndex, board->halfMoveClock));
            } while (targets &= targets - 1);
        } while (tempP &= tempP - 1);
    }
}

void showAvailableMoves(Board *board) {
    MoveList moves;
    initMoveList(&moves, 30);
    char moveText[5] = {'\0'};

    generateMoves(&moves, board);
    for (int i = 0; i < moves.length; i++) {
        moveToText(moveText, moves.moves[i]);
        printf("%d. %s\n", i + 1, moveText);
    }
    destroyMoveList(&moves);
}

unsigned long long int perft(Board *board, int depth) {
    // Finishes depth of 7 in about 7 minutes from starting position, with results that match stockfish.

    if (depth == 0) return 1;
    unsigned long long int count = 0;
    MoveList legalMoves;
    initMoveList(&legalMoves, 40);
    generateMoves(&legalMoves, board);
    if (depth == 1) {
        destroyMoveList(&legalMoves);
        return legalMoves.length;
    }

    for (int i = 0; i < legalMoves.length; i++) {
        makeMove(board, legalMoves.moves[i]);
        count += perft(board, depth - 1);
        unmakeMove(board, legalMoves.moves[i]);
    }
    destroyMoveList(&legalMoves);
    return count;
}

void divide(Board *board, int depth) {
    if (depth == 0) return;
    MoveList legalMoves;
    initMoveList(&legalMoves, 40);
    generateMoves(&legalMoves, board);
    unsigned long long int movCount = 0, posCount = 0, subCount;

    for (int i = 0; i < legalMoves.length; i++) {
        char moveText[5] = { '\0' };
        moveToText(moveText, legalMoves.moves[i]);
        printf("%s - ", moveText);
        makeMove(board, legalMoves.moves[i]);
        subCount = perft(board, depth - 1);
        posCount += subCount;
        unmakeMove(board, legalMoves.moves[i]);
        printf("%llu\n", subCount);
    }
    destroyMoveList(&legalMoves);
    printf("moves: %d\npositions: %llu", legalMoves.length, posCount);
}

parseInt(char *string, int *integer) {
    *integer = 0;
    while (*string != '\0') {
        *integer *= 10;
        *integer += (*string) - '0';
        string++;
    }
}

DWORD WINAPI ioThread(LPVOID lpParameter) {
    Board *board = ((Parameters*)lpParameter)->board;
    char* pieceSymbols = ((Parameters*)lpParameter)->pieceSymbols;

    char buffer[100] = {'\0'};
    printf("Welcome to MyChessEngine\nThe board is currently set up at the starting position\nFor a list of commands type \"help\"\nTo exit type q or quit\n");
    printf("Please keep in mind that his will break if given bad or incorrect input,\nthere is no verification that user provided information is reasonable,\n");
    printf("the move generation also assumes all of the board state information is correct,\nif this is not the case then there may be unexpected side effects\n");
    printf("which made debugging a pain.");
    while (1) {
        printf("\n> ");
        gets_s(buffer, sizeof(buffer));

        if (!strcmp(buffer, "q") || !strcmp(buffer, "quit")) return;
        else if (!strcmp(buffer, "help")) {
            printf("q - quits the engine\n");
            printf("show - shows the board and FEN string\n");
            printf("showboard - shows just the board\n");
            printf("showfen - shows just the FEN string\n");
            printf("setfen <FEN> - sets the board tho the FEN string\n");
        }
        else if (!strcmp(buffer, "show")) printBoard(1, 1, board, pieceSymbols);
        else if (!strcmp(buffer, "showboard")) printBoard(0, 1, board, pieceSymbols);
        else if (!strcmp(buffer, "showfen")) printBoard(1, 0, board, pieceSymbols);
        else if (!memcmp(buffer, "setfen", 6)) readFenStringToBoard(buffer + 7, board);
        else if (!memcmp(buffer, "new", 3)) readFenStringToBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", board);
        else if (!strcmp(buffer, "legalmoves")) showAvailableMoves(board);
        else if (!memcmp(buffer, "move", 4)) makeMove(board, textToMove(buffer + 5, board));
        else if (!memcmp(buffer, "undo", 4)) unmakeLastMove(board);
        else if (!memcmp(buffer, "perft", 5)) {
            int depth;
            parseInt(buffer + 6, &depth);
            printf("%llu\n", perft(board, depth));
        }
        else if (!memcmp(buffer, "divide", 6)) {
            int depth;
            parseInt(buffer + 7, &depth);
            divide(board, depth);
        }
        else if (!strcmp(buffer, "showsbb")) printSquareBasedBoard(board);
    }
    return 0;
}

int main() {
    Board* mainBoard;
    mainBoard = (Board*)malloc(sizeof(Board));
    if (mainBoard == NULL) {
        printf("couldn't initialize the chess board.");
        return 1;
    }
    char pieceSymbols[15];
    initBoardState(mainBoard, pieceSymbols);

    // Create a new thread
    Parameters paramsIO;
    paramsIO.board = mainBoard;
    paramsIO.pieceSymbols = pieceSymbols;
    HANDLE hThread = CreateThread(
        NULL,    // Thread attributes
        0,       // Stack size (0 = use default)
        ioThread, // Thread start address
        &paramsIO,    // Parameter to pass to the thread
        0,       // Creation flags
        NULL);   // Thread id
    if (hThread == NULL)
    {
        // Thread creation failed.
        // More details can be retrieved by calling GetLastError()
        printf("couldn't create IO thread");
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    destroyMoveList(&(mainBoard->history));
    free(mainBoard);
    _CrtDumpMemoryLeaks();
}