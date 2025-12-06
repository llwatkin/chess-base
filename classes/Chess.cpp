#include "Chess.h"
#include "Logger.h"
#include "EvaluationData.h"
#include <limits>
#include <cmath>
#include <chrono>

Logger &logger = Logger::GetInstance();

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        int playerNumber = bit->getOwner()->playerNumber();
        notation = playerNumber == WHITE ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();

    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    if (gameHasAI()) setAIPlayer(AI_PLAYER);

    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    //FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    //FENtoBoard("r1bk3r/p2pBpNp/n4n2/1p1NP2P/6P1/3P4/P1P1K3/q5b1");
    
    _gameState.init(stateString().c_str(), WHITE);
    _moves = _gameState.generateAllMoves(false);

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) 
{
    // Current board position
    int x = 0;
    int y = 7;

    for (size_t i = 0; i < fen.length(); i++)
    {
        int playerNumber = 0;
        ChessPiece pieceType = NoPiece;

        char c = fen[i];
        int ascii = (int)c;
        
        if (c == ' ') break; // Ignore the last bit of the string
        
        if (c == '/') 
        {
            y--;
            x = 0;
        }
        else if (ascii >= 49 && ascii <= 56) // 1 to 8
        {
            int spaces = c -'0';
            x += spaces;
        }
        else if (ascii >= 65 && ascii <= 90) // A to Z
        {
            playerNumber = 0;
            switch (c)
            {
                case 'R': pieceType = Rook; break;
                case 'N': pieceType = Knight; break;
                case 'B': pieceType = Bishop; break;
                case 'Q': pieceType = Queen; break;
                case 'K': pieceType = King; break;
                case 'P': pieceType = Pawn; break;
            }
        }
        else if (ascii >= 97 && ascii <= 122) // a to z
        {
            playerNumber = 1;
            switch (c)
            {
                case 'r': pieceType = Rook; break;
                case 'n': pieceType = Knight; break;
                case 'b': pieceType = Bishop; break;
                case 'q': pieceType = Queen; break;
                case 'k': pieceType = King; break;
                case 'p': pieceType = Pawn; break;
            }
        }

        if (pieceType != NoPiece) 
        {
            Bit *newPiece = PieceForPlayer(playerNumber, pieceType);
            ChessSquare *square = _grid->getSquare(x, y);
            newPiece->setPosition(square->getPosition());
            square->setBit(newPiece);
            newPiece->setOwner(getPlayerAt(playerNumber));
            newPiece->setGameTag(pieceType);

            x++;
        }
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    // Can't place pieces in chess
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    if (bit.isFriendly(getCurrentPlayer())) 
    {
        bool isPossibleMove = false;
        ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
        if (srcSquare)
        {
            int srcIndex = srcSquare->getSquareIndex();
            //logger.Info("Source index = " + std::to_string(srcIndex));
            for (auto const & move : _moves)
            {
                //logger.Info("Move from = " + std::to_string(move.from) + " Move to = " + std::to_string(move.to));
                if (move.from == srcIndex)
                {
                    //logger.Warn("Move from is equal to source index!");
                    isPossibleMove = true;
                    auto destination = _grid->getSquareByIndex(move.to);
                    destination->setHighlighted(true);
                }
            }
        }
        else
        {
            logger.Error("No source square found");
        }
        if (!isPossibleMove) logger.Warn("There are no possible moves for this piece");
        return isPossibleMove;
    }
    else
    {
        logger.Warn("This piece is not yours");
    }
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = static_cast<ChessSquare*>(&dst);
    if (!srcSquare || !dstSquare) return false;

    int srcIndex = srcSquare->getSquareIndex();
    int dstIndex = dstSquare->getSquareIndex();

    for (auto const & move : _moves) if (move.from == srcIndex && move.to == dstIndex) return true;
    return false;
}

void Chess::stopGame()
{
    _grid->forEachSquare(
        [](ChessSquare* square, int x, int y) 
        {
            square->destroyBit();
        }
    );
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

int Chess::evaluateGameState(const GameState& gameState)
{
    int evaluation = 0;
    for (int square = 0; square < 64; square++)
    {
        const unsigned char piece = (gameState.state[square]);
        evaluation += evaluateScores[piece]; 
        evaluation += pieceSquareTables[piece][square];
    }
    if (gameState.color == AI_PLAYER) evaluation *= -1; // ???
    return evaluation;
}

int Chess::negamax(GameState& gameState, int depth, int alpha, int beta)
{
    _moveCount++;
    if (depth == 0) return evaluateGameState(gameState);

    auto newMoves = gameState.generateAllMoves(true);
    if (newMoves.size() == 0) return evaluateGameState(gameState);

    int bestEvaluation = -1000;
    for (const auto & move : newMoves)
    {
        gameState.pushMove(move);
        bestEvaluation = std::max(bestEvaluation, -negamax(gameState, depth - 1, -beta, -alpha));
        gameState.popState();
        alpha = std::max(alpha, bestEvaluation);
        if (alpha >= beta) break;
    }
    return bestEvaluation;
}

void Chess::updateAI()
{
    int bestEvaluation = -1000;
    BitMove bestMove;
    _moveCount = 0;
    const auto searchStartTime = std::chrono::steady_clock::now();

    // Find the best move with negamax
    for (const auto & move : _moves)
    {
        _gameState.pushMove(move);
        int moveEvaluation = -negamax(_gameState, 3, -1000, 1000);
        _gameState.popState();

        // If this current move is better than the best move, update!
        if (moveEvaluation > bestEvaluation)
        {
            bestMove = move;
            bestEvaluation = moveEvaluation;
        }
    }

    // Finally, make the best move
    if (bestEvaluation != -1000)
    {
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - searchStartTime).count();
        const double boardsPerSecond = seconds > 0.0 ? static_cast<double>(_moveCount) / seconds : 0.0;
        logger.Info("Checked " + std::to_string(_moveCount) + " moves at a speed of " + std::to_string(boardsPerSecond) + " boards/second");
        int srcSquare = bestMove.from;
        int dstSquare = bestMove.to;
        BitHolder& src = getHolderAt(srcSquare&7, srcSquare/8);
        BitHolder& dst = getHolderAt(dstSquare&7, dstSquare/8);
        Bit* bit = src.bit();
        dst.dropBitAtPoint(bit, ImVec2(0, 0));
        src.setBit(nullptr);
        bitMovedFromTo(*bit, src, dst);
    }
}


Player* Chess::checkForWinner()
{
    // Generate moves for next player
    int nextPlayer = getCurrentPlayer()->playerNumber();
    int prevPlayer = nextPlayer == 0 ? 1 : 0;
    _gameState.init(stateString().c_str(), nextPlayer);
    _moves = _gameState.generateAllMoves(false);

    // If the next player cannot move out of check, the previous player wins!
    if (_moves.size() == 0)
    {
        logger.Event("Player " + std::to_string(nextPlayer) + " cannot make any legal moves. Player " + std::to_string(prevPlayer) + " wins!");
        return getPlayerAt(prevPlayer);
    }
    return nullptr;
}

bool Chess::checkForDraw()
{
    // TODO: Check whether there is sufficient material for a checkmate
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;
}

void Chess::setStateString(const std::string &s)
{
    // TODO?
}

void Chess::clearBoardHighlights()
{
    _grid->forEachSquare(
        [](ChessSquare* square, int x, int y) 
        {
            square->setHighlighted(false);
        }
    );
}
