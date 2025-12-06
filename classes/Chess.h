#pragma once

#include "Game.h"
#include "GameState.h"

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
    void clearBoardHighlights() override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    void updateAI() override;
    bool gameHasAI() override { return true; }
    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    // AI methods
    int evaluateGameState(const GameState& gameState);
    int negamax(GameState& gameState, int depth, int alpha, int beta);

    Grid* _grid;

    std::vector<BitMove> _moves;
    GameState _gameState;
    int _moveCount;
};