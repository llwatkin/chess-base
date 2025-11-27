#pragma once

#include "Game.h"
#include "Bitboard.h"

#define WHITE 0
#define BLACK 1

constexpr int pieceSize = 80;

enum AllBitboards
{
    WHITE_PAWNS,
    WHITE_KNIGHTS,
    WHITE_BISHOPS,
    WHITE_ROOKS,
    WHITE_QUEENS,
    WHITE_KING,
    WHITE_ALL,
    BLACK_PAWNS,
    BLACK_KNIGHTS,
    BLACK_BISHOPS,
    BLACK_ROOKS,
    BLACK_QUEENS,
    BLACK_KING,
    BLACK_ALL,
    EMPTY_SQUARES
};

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;
    void clearBoardHighlights() override;
    void endTurn() override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    const int NUM_BITBOARDS = 14;

    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    // Generating bitboards and moves
    void addPawnBitboardMovesToList(std::vector<BitMove>& moves, const Bitboard bitboard, const int shift);
    void generatePawnMoves(std::vector<BitMove>& moves, Bitboard pawnBoard, Bitboard enemyPieces, Bitboard emptySquares, char color);
    void generateKnightMoves(std::vector<BitMove>& moves, Bitboard knightBoard, uint64_t movableSquares);
    void generateKingMoves(std::vector<BitMove>& moves, Bitboard kingBoard, uint64_t movableSquares);
    void generateRookMoves(std::vector<BitMove>& moves, Bitboard rookBoard, uint64_t occupiedSquares, uint64_t friendlySquares);
    void generateBishopMoves(std::vector<BitMove>& moves, Bitboard bishopBoard, uint64_t occupiedSquares, uint64_t friendlySquares);
    void generateQueenMoves(std::vector<BitMove>& moves, Bitboard queenBoard, uint64_t occupiedSquares, uint64_t friendlySquares);
    std::vector<BitMove> generateMoves(const std::string& gameState, char color);

    Grid* _grid;
    int _bitboardLookup[128];
    Bitboard _bitboards[14];
    Bitboard _knightBitboards[64];
    Bitboard _kingBitboards[64];

    std::vector<BitMove> _moves;
};