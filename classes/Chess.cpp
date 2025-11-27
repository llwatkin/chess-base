#include "Chess.h"
#include "Logger.h"
#include "MagicBitboards.h"
#include <limits>
#include <cmath>

Logger &logger = Logger::GetInstance();

Chess::Chess()
{
    _grid = new Grid(8, 8);
    initMagicBitboards();

    for (int i = 0; i < 128; i++) _bitboardLookup[i] = 0;
    _bitboardLookup['P'] = WHITE_PAWNS;
    _bitboardLookup['N'] = WHITE_KNIGHTS;
    _bitboardLookup['B'] = WHITE_BISHOPS;
    _bitboardLookup['R'] = WHITE_ROOKS;
    _bitboardLookup['Q'] = WHITE_QUEENS;
    _bitboardLookup['K'] = WHITE_KING;
    _bitboardLookup['p'] = BLACK_PAWNS;
    _bitboardLookup['n'] = BLACK_KNIGHTS;
    _bitboardLookup['b'] = BLACK_BISHOPS;
    _bitboardLookup['r'] = BLACK_ROOKS;
    _bitboardLookup['q'] = BLACK_QUEENS;
    _bitboardLookup['k'] = BLACK_KING;
    _bitboardLookup['0'] = EMPTY_SQUARES;
}

Chess::~Chess()
{
    delete _grid;
    cleanupMagicBitboards();
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
        _knightBitboards[i] = KnightAttacks[i];
        _kingBitboards[i] = KingAttacks[i];
    }
    
    _moves = generateMoves(stateString(), WHITE);
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
void Chess::generateKnightMoves(std::vector<BitMove>& moves, Bitboard knightBoard, uint64_t movableSquares)
{
    knightBoard.forEachBit(
        [&](int fromSquare) 
        {
            Bitboard moveBitboard = Bitboard(_knightBitboards[fromSquare].getData() & movableSquares);
            moveBitboard.forEachBit(
                [&](int toSquare) 
                { 
                    moves.emplace_back(fromSquare, toSquare, Knight); 
                }
            ); 
        }
    );
}

void Chess::generateKingMoves(std::vector<BitMove>& moves, Bitboard kingBoard, uint64_t movableSquares)
{
    kingBoard.forEachBit(
        [&](int fromSquare) 
        {
            Bitboard moveBitboard = Bitboard(_kingBitboards[fromSquare].getData() & movableSquares);
            moveBitboard.forEachBit(
                [&](int toSquare) 
                { 
                    moves.emplace_back(fromSquare, toSquare, King); 
                }
            ); 
        }
    );
}

void Chess::generateRookMoves(std::vector<BitMove>& moves, Bitboard rookBoard, uint64_t occupiedSquares, uint64_t friendlySquares)
{
    rookBoard.forEachBit(
        [&](int fromSquare)
        {
            Bitboard moveBitboard = Bitboard(getRookAttacks(fromSquare, occupiedSquares) & friendlySquares);
            moveBitboard.forEachBit(
                [&](int toSquare) 
                { 
                    moves.emplace_back(fromSquare, toSquare, Rook); 
                }
            ); 
        }
    );
}

void Chess::generateBishopMoves(std::vector<BitMove>& moves, Bitboard bishopBoard, uint64_t occupiedSquares, uint64_t friendlySquares)
{
    bishopBoard.forEachBit(
        [&](int fromSquare)
        {
            Bitboard moveBitboard = Bitboard(getBishopAttacks(fromSquare, occupiedSquares) & friendlySquares);
            moveBitboard.forEachBit(
                [&](int toSquare) 
                { 
                    moves.emplace_back(fromSquare, toSquare, Bishop); 
                }
            ); 
        }
    );
}

void Chess::generateQueenMoves(std::vector<BitMove>& moves, Bitboard queenBoard, uint64_t occupiedSquares, uint64_t friendlySquares)
{
    queenBoard.forEachBit(
        [&](int fromSquare)
        {
            Bitboard moveBitboard = Bitboard(getQueenAttacks(fromSquare, occupiedSquares) & friendlySquares);
            moveBitboard.forEachBit(
                [&](int toSquare) 
                { 
                    moves.emplace_back(fromSquare, toSquare, Queen); 
                }
            ); 
        }
    );
}

//
// Generates all possible moves for a given player
//
std::vector<BitMove> Chess::generateMoves(const std::string& gameState, char color)
{
    std::vector<BitMove> moves;
    moves.reserve(32);

    for (int i = 0; i < NUM_BITBOARDS; i++) _bitboards[i] = 0ULL;
    for (int i = 0; i < 64; i++)
    {
        int bitboardIndex = _bitboardLookup[gameState[i]];
        _bitboards[bitboardIndex] |= 1ULL << i;
    }
    
    _bitboards[WHITE_ALL] = _bitboards[WHITE_PAWNS].getData() | 
                            _bitboards[WHITE_KNIGHTS].getData() | 
                            _bitboards[WHITE_BISHOPS].getData() | 
                            _bitboards[WHITE_ROOKS].getData() |
                            _bitboards[WHITE_QUEENS].getData() |
                            _bitboards[WHITE_KING].getData();
    _bitboards[BLACK_ALL] = _bitboards[BLACK_PAWNS].getData() |
                            _bitboards[BLACK_KNIGHTS].getData() |
                            _bitboards[BLACK_BISHOPS].getData() |
                            _bitboards[BLACK_ROOKS].getData() |
                            _bitboards[BLACK_QUEENS].getData() |
                            _bitboards[BLACK_KING].getData();

    int myBitIndex = color == WHITE ? WHITE_PAWNS : BLACK_PAWNS;
    int enemyBitIndex = color == WHITE ? BLACK_PAWNS : WHITE_PAWNS;

    uint64_t myPawns = _bitboards[WHITE_PAWNS + myBitIndex].getData();
    uint64_t myKnights = _bitboards[WHITE_KNIGHTS + myBitIndex].getData();
    uint64_t myBishops = _bitboards[WHITE_BISHOPS + myBitIndex].getData();
    uint64_t myRooks = _bitboards[WHITE_ROOKS + myBitIndex].getData();
    uint64_t myQueens = _bitboards[WHITE_QUEENS + myBitIndex].getData();
    uint64_t myKing = _bitboards[WHITE_KING + myBitIndex].getData();
    
    uint64_t occupiedByEnemy = _bitboards[WHITE_ALL + enemyBitIndex].getData();
    uint64_t occupiedByMe = _bitboards[WHITE_ALL + myBitIndex].getData();

    generatePawnMoves(moves, myPawns, occupiedByEnemy, ~occupiedByMe & ~occupiedByEnemy, color);
    generateKnightMoves(moves, myKnights, ~occupiedByMe);
    generateBishopMoves(moves, myBishops, occupiedByMe | occupiedByEnemy, ~occupiedByMe);
    generateRookMoves(moves, myRooks, occupiedByMe | occupiedByEnemy, ~occupiedByMe);
    generateQueenMoves(moves, myQueens, occupiedByMe | occupiedByEnemy, ~occupiedByMe);
    generateKingMoves(moves, myKing, ~occupiedByMe);
    
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
    _moves = generateMoves(stateString(), nextColor);
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

void Chess::clearBoardHighlights()
{
    _grid->forEachSquare(
        [](ChessSquare* square, int x, int y) 
        {
            square->setHighlighted(false);
        }
    );
}
