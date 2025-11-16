#include "Chess.h"
#include "Logger.h"
#include <limits>
#include <cmath>

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
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    //FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    //FENtoBoard("r1bk3r/p2pBpNp/n4n2/1p1NP2P/6P1/3P4/P1P1K3/q5b1");

    startGame();

    // Pre-compute knight and king move bitboards
    for (int i = 0; i < 64; i++)
    {
        _knightBitboards[i] = generateKnightMoveBitboard(i);
        _kingBitboards[i] = generateKingMoveBitboard(i);
    }
    
    _moves = generateMoves(WHITE);
}

void Chess::FENtoBoard(const std::string& fen) {
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

//
// Generates a bitboard with all possible knight moves from a given square
//
Bitboard Chess::generateKnightMoveBitboard(int square)
{
    Bitboard bitboard = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    // All possible L-shaped moves from this position
    std::pair<int, int> knightOffsets[] = 
    {
        { 2, 1 }, { 2, -1 }, { -2, 1 }, { -2, -1 },
        { 1, 2 }, { 1, -2 }, { -1, 2 }, { -1, -2 }
    };

    // Shift bits in bitboard according to all possible L-shaped moves (limited by the edges of the board)
    constexpr uint64_t oneBit = 1;
    for (auto [dr, df] : knightOffsets) 
    {
        int r = rank + dr, f = file + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8) bitboard |= oneBit << (r * 8 + f);
    }

    return bitboard;
}

//
// Generated a bitboard with all possible king moves from a given square
//
Bitboard Chess::generateKingMoveBitboard(int square)
{
    Bitboard bitboard = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    // All possible 1-space moves from this position
    std::pair<int, int> kingOffsets[] = 
    {
        { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 },
        { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 }
    };

    // Shift bits in bitboard according to all possible 1-space moves (limited by the edges of the board)
    constexpr uint64_t oneBit = 1;
    for (auto [dr, df] : kingOffsets) 
    {
        int r = rank + dr, f = file + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8) bitboard |= oneBit << (r * 8 + f);
    }

    return bitboard;
}

void Chess::addPawnBitboardMovesToList(std::vector<BitMove>& moves, const Bitboard bitboard, const int shift)
{
    if (bitboard.getData() == 0) return;

    bitboard.forEachBit(
        [&](int toSquare)
        {
            int fromSquare = toSquare - shift;
            moves.emplace_back(fromSquare, toSquare, Pawn);
        }
    );
}

//
// Generates move objects for pawns from a bitboard, adding them to the moves list with addPawnBitboardMovesToList()
//
void Chess::generatePawnMoves(std::vector<BitMove>& moves, Bitboard pawnBoard, Bitboard enemyPieces, Bitboard emptySquares, char color)
{
    if (pawnBoard.getData() == 0) return;

    // Constants for ranks and files
    constexpr uint64_t NotAFile(0xFEFEFEFEFEFEFEFEULL); // A file mask (left edge)
    constexpr uint64_t NotHFile(0x7F7F7F7F7F7F7F7FULL); // H file mask (right edge)
    constexpr uint64_t Rank3(0x0000000000FF0000ULL); // Rank 3 mask (one space ahead starting rank for white)
    constexpr uint64_t Rank6(0x0000FF0000000000ULL); // Rank 6 mask (one space ahead starting rank for black)

    // Calculate single pawn moves forward
    Bitboard singleMoves = (color == WHITE) ? 
                           (pawnBoard.getData() << 8) & emptySquares.getData() : 
                           (pawnBoard.getData() >> 8) & emptySquares.getData();
    // Calculate double pawn moves from the starting rank
    Bitboard doubleMoves = (color == WHITE) ?
                           ((singleMoves.getData() & Rank3) << 8) & emptySquares.getData() :
                           ((singleMoves.getData() & Rank6) >> 8) & emptySquares.getData();
    // Calculate left and right pawn captures
    Bitboard capturesLeft = (color == WHITE) ?
                            ((pawnBoard.getData() & NotAFile) << 7) & enemyPieces.getData() :
                            ((pawnBoard.getData() & NotAFile) >> 9) & enemyPieces.getData();
    Bitboard capturesRight = (color == WHITE) ?
                             ((pawnBoard.getData() & NotHFile) << 9) & enemyPieces.getData() :
                             ((pawnBoard.getData() & NotHFile) >> 7) & enemyPieces.getData();

    int singleShift = (color == WHITE) ? 8 : -8;
    int doubleShift = (color == WHITE) ? 16 : -16;
    int captureLeftShift = (color == WHITE) ? 7 : -9;
    int captureRightShift = (color == WHITE) ? 9 : -7;

    // Add all calculated moves to the move list
    addPawnBitboardMovesToList(moves, singleMoves, singleShift);
    addPawnBitboardMovesToList(moves, doubleMoves, doubleShift);
    addPawnBitboardMovesToList(moves, capturesLeft, captureLeftShift);
    addPawnBitboardMovesToList(moves, capturesRight, captureRightShift);
}

//
// Generates move objects for knights from a bitboard
//
void Chess::generateKnightMoves(std::vector<BitMove>& moves, Bitboard knightBoard, uint64_t emptySquares)
{
    knightBoard.forEachBit(
        [&](int fromSquare) 
        {
            Bitboard moveBitboard = Bitboard(_knightBitboards[fromSquare].getData() & emptySquares);
            moveBitboard.forEachBit(
                [&](int toSquare) 
                { 
                    moves.emplace_back(fromSquare, toSquare, Knight); 
                }
            ); 
        }
    );
}

//
// Generates move objects for kings from a bitboard
//
void Chess::generateKingMoves(std::vector<BitMove>& moves, Bitboard kingBoard, uint64_t emptySquares)
{
    kingBoard.forEachBit(
        [&](int fromSquare) 
        {
            Bitboard moveBitboard = Bitboard(_kingBitboards[fromSquare].getData() & emptySquares);
            moveBitboard.forEachBit(
                [&](int toSquare) 
                { 
                    moves.emplace_back(fromSquare, toSquare, King); 
                }
            ); 
        }
    );
}

//
// Generates all possible moves for a given player
//
std::vector<BitMove> Chess::generateMoves(char color)
{
    std::vector<BitMove> moves;
    moves.reserve(32);
    std::string gameState = stateString();

    uint64_t myKnights = 0LL;
    uint64_t myPawns = 0LL;
    uint64_t myKing = 0LL;
    uint64_t myOtherPieces = 0LL;
    uint64_t enemyKnights = 0LL;
    uint64_t enemyPawns = 0LL;
    uint64_t enemyKing = 0LL;
    uint64_t enemyOtherPieces = 0LL;

    const char *myPieces = (color == WHITE) ? "PNBRQK" : "pnbrqk";
    const char *enemyPieces = (color == WHITE) ? "pnbrqk" : "PNBRQK";

    for (int i = 0; i < 64; i++)
    {
        if (gameState[i] == myPieces[0]) myPawns |= 1ULL << i;
        else if (gameState[i] == myPieces[1]) myKnights |= 1ULL << i;
        else if (gameState[i] == myPieces[2]) myOtherPieces |= 1ULL << i;
        else if (gameState[i] == myPieces[3]) myOtherPieces |= 1ULL << i;
        else if (gameState[i] == myPieces[4]) myOtherPieces |= 1ULL << i;
        else if (gameState[i] == myPieces[5]) myKing |= 1ULL << i;
        else if (gameState[i] == enemyPieces[0]) enemyPawns |= 1ULL << i;
        else if (gameState[i] == enemyPieces[1]) enemyKnights |= 1ULL << i;
        else if (gameState[i] == enemyPieces[2]) enemyOtherPieces |= 1ULL << i;
        else if (gameState[i] == enemyPieces[3]) enemyOtherPieces |= 1ULL << i;
        else if (gameState[i] == enemyPieces[4]) enemyOtherPieces |= 1ULL << i;
        else if (gameState[i] == enemyPieces[5]) enemyKing |= 1ULL << i;
    }

    uint64_t occupiedByMe = myKnights | myPawns | myKing | myOtherPieces;
    uint64_t occupiedByEnemy = enemyKnights | enemyPawns | enemyKing | enemyOtherPieces;
    generateKnightMoves(moves, myKnights, ~occupiedByMe);
    generateKingMoves(moves, myKing, ~occupiedByMe);
    generatePawnMoves(moves, myPawns, occupiedByEnemy, ~occupiedByMe, color);

    logger.Info("There are " + std::to_string(moves.size()) + " moves available for Player " + std::to_string(color));
    return moves;
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
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
            for (auto const & move : _moves)
            {
                if (move.from == srcIndex)
                {
                    isPossibleMove = true;
                    auto destination = _grid->getSquareByIndex(move.to);
                    destination->setHighlighted(true);
                }
            }
        }
        return isPossibleMove;
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
    _grid->forEachSquare(
        [](ChessSquare* square, int x, int y) 
        {
            square->setHighlighted(false);
        }
    );
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

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    char nextColor = getCurrentPlayer()->playerNumber() == 0 ? WHITE : BLACK;
    _moves = generateMoves(nextColor);
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
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}
