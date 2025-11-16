#pragma once

#include "Game.h"
#include "Grid.h"

#define WHITE 0
#define BLACK 1

constexpr int pieceSize = 80;

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    // Generating bitboards and moves
    Bitboard generateKnightMoveBitboard(int square);
    Bitboard generateKingMoveBitboard(int square);
    void addPawnBitboardMovesToList(std::vector<BitMove>& moves, const Bitboard bitboard, const int shift);
    void generatePawnMoves(std::vector<BitMove>& moves, Bitboard pawnBoard, Bitboard enemyPieces, Bitboard emptySquares, char color);
    void generateKnightMoves(std::vector<BitMove>& moves, Bitboard knightBoard, uint64_t emptySquares);
    void generateKingMoves(std::vector<BitMove>& moves, Bitboard kingBoard, uint64_t emptySquares);
    std::vector<BitMove> generateMoves(char color);

    void clearBoardHighlights();

    Grid* _grid;
    Bitboard _knightBitboards[64];
    Bitboard _kingBitboards[64];

    std::vector<BitMove> _moves;
};