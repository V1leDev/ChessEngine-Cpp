#include <iostream>
#include <array>
#include <list>
#include <map>
using namespace std;

// NOTE: Castling
// NOTE: Add pawn promotion

// Helper class to store a x and y value
class Vector {
	public: int x;
	public: int y;
	public: Vector(int x, int y) {
		this->x = x;
		this->y = y;
	}
	public: bool equals(Vector v) const { return (this->x == v.x && this->y == v.y); }
};

class LogEntry {
	public: string board[8][8];
	public: bool isWhiteToMove;
	public: map<char, Vector> kingPositions;
	public: map<char, array<bool, 2>> castleRights;
	public: LogEntry(string board[8][8], bool isWhiteToMove, map<char, Vector> kingPositions, map<char, array<bool, 2>> castleRights) {
        copy(&board[0][0], &board[0][0] + 64, &this->board[0][0]);
        this->isWhiteToMove = isWhiteToMove;
        this->kingPositions = std::move(kingPositions);
		this->castleRights = std::move(castleRights);
    }
};

class GameState {
	// This variable stores the current board, the pieces are expressed as a combination of two letters,
	// The first letter defines the color; the second the piece type. Empty squares are defined as "  "
	public: string board[8][8] = {
		{"bR", "bN", "bB", "bQ", "bK", "bB", "bN", "bR"},
		{"bP", "bP", "bP", "bP", "bP", "bP", "bP", "bP"},
		{"  ", "  ", "  ", "  ", "  ", "  ", "  ", "  "},
		{"  ", "  ", "  ", "  ", "  ", "  ", "  ", "  "},
		{"  ", "  ", "  ", "  ", "  ", "  ", "  ", "  "},
		{"  ", "  ", "  ", "  ", "  ", "  ", "  ", "  "},
		{"wP", "wP", "wP", "wP", "wP", "wP", "wP", "wP"},
		{"wR", "wN", "wB", "wQ", "wK", "wB", "wN", "wR"}
	};
	private: bool isWhiteToMove = true;
	private: list<array<Vector, 2>> validMoves;
	private: map<char, Vector> kingPositions = {
		{'w', Vector(7, 4)},
		{'b', Vector(0, 4)}
	};
	private: map<char, array<bool, 2>> castleRights = {
		{'w', array<bool, 2>{true, true}},
		{'b', array<bool, 2>{true, true}}
	};
	private: list<LogEntry> log;
	
	// Constructor: calls the getValidMoves function so the white player can play a move
	public: GameState() { this->getValidMoves(); }
	
	// This function allows an external program to access the possible moves for a single piece
	public: list<Vector> getMovesForPiece(int r, int c) {
		// Get all the possible moves (same as in the normal process for validating the moves)
        char turn = this->isWhiteToMove ? 'w' : 'b';
        list<array<Vector, 2>> moves;
        if (this->board[r][c].at(0) == turn) {
            char piece = this->board[r][c].at(1);
            switch (piece) {
                case 'P':
                    this->addPawnMove(r, c, moves);
                    break;
                case 'R':
                    this->addRookMove(r, c, moves);
                    break;
                case 'N':
                    this->addKnightMove(r, c, moves);
                    break;
                case 'B':
                    this->addBishopMove(r, c, moves);
                    break;
                case 'Q':
                    this->addQueenMove(r, c, moves);
                    break;
                default:
                    this->addKingMove(r, c, moves);
            }
            this->removeIllegalMoves(moves);
        }
		// Remove the source position since it is not needed
		list<Vector> returnVal;
		for (array<Vector, 2> m : moves) {
			returnVal.push_back(m[1]);
		}
		return returnVal;
	}
	
	// This function is used for an external program which uses this engine to pass in a move a player makes
	public: void playMove(int startRow, int startCol, int endRow, int endCol) {
		// is the move allowed
		Vector move[2] = {{startRow, startCol}, {endRow, endCol}};
		for (array<Vector,2> arr : this->validMoves) {
			if (arr[0].equals(move[0]) && arr[1].equals(move[1])) {
				// Update King position before executing the move
				if (this->board[startRow][startCol].at(1) == 'K') {
					this->kingPositions.at(this->board[startRow][startCol].at(0)) = move[1];
				} else if (this->board[startRow][startCol].at(1) == 'P') {
                    if (startCol != endCol && this->board[endRow][endCol].at(1) == ' ') {
                        // Captured en passant -> clear the square behind the current one
                        this->board[startRow][endCol] = "  ";
                    }
				}
				this->move(startRow, startCol, endRow, endCol);
				// log the board
				LogEntry entry(this->board, this->isWhiteToMove,this->kingPositions, this->castleRights);
				this->log.push_back(entry);
				this->isWhiteToMove = !this->isWhiteToMove; // change turn
				this->getValidMoves(); // get moves for the next player
				break;
			}
		}
	}
	
	// Helper function to only execute the move without logging and checking for validity
	private: void move(int startRow, int startCol, int endRow, int endCol) {
		// play the move
		this->board[endRow][endCol] = this->board[startRow][startCol];
		this->board[startRow][startCol] = "  ";
	}
	
	// This function is used to undo the last recent move played
	public: void undoMove() {
		// get last move
		LogEntry entry = this->log.back();
		this->log.pop_back();
		// restore the GameState
		copy(&this->board[0][0], &this->board[0][0] + 64, &entry.board[0][0]);
		this->isWhiteToMove = entry.isWhiteToMove;
		this->kingPositions = entry.kingPositions;
		this->castleRights = entry.castleRights;
		this->getValidMoves(); // get moves for the current player
	}
	
	// This function fills the "validMoves" variable with all the possible moves that the current player can play
	private: void getValidMoves() {
		this->validMoves.clear();
		this->getAllMoves(this->validMoves); // get all moves
		this->removeIllegalMoves(this->validMoves); // remove the invalid moves
	}
	
	// Helper function to fill the "validMoves" variable with all moves (also illegal moves) that the user has available
	private: void getAllMoves(list<array<Vector, 2>>& list) {
		char turn = this->isWhiteToMove ? 'w' : 'b';
		// Loop through all fields and check if there is piece that the current player owns and get its moves
		for (int r = 0; r < 8; r++) {
			for (int c = 0; c < 8; c++) {
				if ((this->board[r][c]).at(0) == turn) {
					char piece = (this->board[r][c]).at(1);
					switch (piece) {
						case 'P':
							this->addPawnMove(r, c, list);
						break;
						case 'R':
							this->addRookMove(r, c, list);
						break;
						case 'N':
							this->addKnightMove(r, c, list);
						break;
						case 'B':
							this->addBishopMove(r, c, list);
						break;
						case 'Q':
							this->addQueenMove(r, c, list);
						break;
						default:
							this->addKingMove(r, c, list);
					}
				}
			}
		}
	}
	
	// Helper function to remove all illegal moves from the list
	private: void removeIllegalMoves(list<array<Vector, 2>>& list) {
		// loop through the list (we have to check i < list.size() since we use an unsigned int which will never be < 0, which means that else this would be an infinite loop)
        for (unsigned int i = list.size() - 1; i >= 0 && i < list.size(); i--) {
            // Get the array and store the piece that is in the destination square so we can undo it later
            array<Vector, 2> arr = *next(list.begin(), i);
            string occupiedSquare = this->board[arr[1].x][arr[1].y];
            this->move(arr[0].x, arr[0].y, arr[1].x, arr[1].y); // make the move
            if (this->isCheck()) {
                // The King is in check and therefore that piece is not allowed to be moved
                list.erase(next(list.begin(), i));// Remove the move from the list (remove by iterator)
            }
            this->undo(arr, occupiedSquare); // undo move
        }
	}
	
	// This function undoes the move which was made specifically in removeIllegalMoves method
	private: void undo(array<Vector, 2> arr, string occupiedSquare) {
		this->board[arr[0].x][arr[0].y] = this->board[arr[1].x][arr[1].y];
		this->board[arr[1].x][arr[1].y] = std::move(occupiedSquare);
	}
	
	// This function checks if the king of the current active player is under attack
	private: bool isCheck() {
		return this->squareUnderAttack(this->isWhiteToMove ? this->kingPositions.at('w') : this->kingPositions.at('b'));
	}
	
	// This function checks if the square r, c is under attack
	private: bool squareUnderAttack(Vector pos) {
		this->isWhiteToMove = !this->isWhiteToMove; // change turn
		// get all the moves that the enemy player could make the next move
		list<array<Vector, 2>> enemyMoves;
		this->getAllMoves(enemyMoves);
		this->isWhiteToMove = !this->isWhiteToMove; // change the turn back
		// loop through moves and check if the endPos equals pos.x pos.y
		for (array<Vector, 2> arr : enemyMoves) {
			if (arr[1].x == pos.x && arr[1].y == pos.y) {
				return true;
			}
		}
		return false;
	}
	
	// Function to add all the possible moves for pawns
	private: void addPawnMove(int r, int c, list<array<Vector, 2>>& list) {
		int offset = this->isWhiteToMove ? -1 : 1;
		// Normal pawn advances
		if (this->board[r + offset][c] == "  ") { // Single pawn advance (Check field in front of the pawn)
			list.push_back(array<Vector, 2>{*new Vector(r, c), *new Vector(r + offset, c)});
			if ((this->isWhiteToMove && r == 6 || !this->isWhiteToMove && r == 1) && this->board[r + (2 * offset)][c] == "  ") { // Double pawn advance (Check the field 2 in front of the pawn)
				list.push_back(array<Vector, 2>{*new Vector(r, c), *new Vector(r + (2 * offset), c)});
			}
		}
		// Captures
		char enemy = this->isWhiteToMove ? 'b' : 'w';
		if (c <= 7 && (this->board[r + offset][c + 1]).at(0) == enemy) { // Capture to the right
			list.push_back(array<Vector, 2>{*new Vector(r, c), *new Vector(r + offset, c + 1)});
		}
		if (c >= 0 && (this->board[r + offset][c - 1]).at(0) == enemy) { // Capture to the left
			list.push_back(array<Vector, 2>{*new Vector(r, c), *new Vector(r + offset, c - 1)});
		}
		// En passant (Only possible if white is in row 3 or black is on row 4 (log.size should not be emtpy. The only
		// way that log is empty, is when creating custom positions, which should be possible)
        if (this->log.size() > 1 && (this->isWhiteToMove && r == 3 || !this->isWhiteToMove && r == 4)) {
            auto lastBoard = (*next(this->log.end(), -2)).board;  // Get the last board
            // Check if the last move was a 2 step pawn advance either left or right of the current column
            if (c - 1 >= 0 && lastBoard[r + (2 * offset)][c - 1].at(1) == 'P' && this->board[r + offset][c - 1] == "  " && this->board[r + (2 * offset)][c - 1] == "  ") {
                list.push_back(array<Vector, 2>{*new Vector(r, c), *new Vector(r + offset, c - 1)});
            } else if (c + 1 <= 7 && lastBoard[r + (2 * offset)][c + 1].at(1) == 'P' && this->board[r + offset][c + 1] == "  " && this->board[r + (2 * offset)][c + 1] == "  ") {
                list.push_back(array<Vector, 2>{*new Vector(r, c), *new Vector(r + offset, c + 1)});
            }

        }
	}
	
	// Function to add all the possible moves for rooks
	private: void addRookMove(int r, int c, list<array<Vector, 2>>& list) {
		int rows[4] = {-1, 1, 0, 0};
		int cols[4] = {0, 0, 1, -1};
		this->bishopRookHelper(rows, cols, r, c, list);
	}
	
	// Function to add all the possible moves for bishops
	private: void addBishopMove(int r, int c, list<array<Vector, 2>>& list) {
		int rows[4] = {-1, 1, -1, 1};
		int cols[4] = {-1, 1, 1, -1};
		this->bishopRookHelper(rows, cols, r, c, list);
	}
	
	// Helper function for rooks and bishops
	private: void bishopRookHelper(const int rows[], const int cols[], int r, int c, list<array<Vector, 2>>& list) {
		char turn = this->isWhiteToMove ? 'w' : 'b';
		char enemy = this->isWhiteToMove ? 'b' : 'w';
		for (int i = 0; i < 4; i++) {
			for (int row = r + rows[i], col = c + cols[i]; row < 8 && row >= 0 && col < 8 && col >= 0; row += rows[i], col += cols[i]) {
				if ((this->board[row][col]).at(0) == turn) break; // Own Piece, so no need to keep on checking
				list.push_back(array<Vector, 2>{*new Vector(r, c), *new Vector(row, col)});
				if ((this->board[row][col]).at(0) == enemy) break; // Enemy Piece, so allow to capture, but dont go further
			}
		}
	}
	
	// Function to add all the possible moves for knights
	private: void addKnightMove(int r, int c, list<array<Vector, 2>>& list) {
		char turn = this->isWhiteToMove ? 'w' : 'b';
		int rows[8] = {-2, -2, 2, 2, -1, 1, -1, 1};
		int cols[8] = {-1, 1, -1, 1, -2, -2, 2, 2};
		for (int i = 0; i < 8; i++) {
			int row = r + rows[i];
			int col = c + cols[i];
			// Inside the board and not attacking an ally
			if (row >= 0 && row < 8 && col >= 0 && col < 8 && (this->board[row][col]).at(0) != turn) {
				list.push_back(array<Vector, 2>{*new Vector(r, c), *new Vector(row, col)});
			}
		}
	}
	
	// Function to add all the possible moves for the queen
	private: void addQueenMove(int r, int c, list<array<Vector, 2>>& list) {
		this->addRookMove(r, c, list);
		this->addBishopMove(r, c, list);
	}
	
	// Function to add all the possible moves for the king
	private: void addKingMove(int r, int c, list<array<Vector, 2>>& list) {
		char turn = this->isWhiteToMove ? 'w' : 'b';
		int rows[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
		int cols[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
		for (int i = 0; i < 8; i++) {
			int row = r + rows[i];
			int col = c + cols[i];
			if (row >= 0 && row < 8 && col >= 0 && col < 8 && (this->board[row][col]).at(0) != turn) {
				list.push_back(array<Vector, 2>{*new Vector(r, c), *new Vector(row, col)});
			}
		}
	}	
	
};




















