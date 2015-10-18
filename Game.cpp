#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <climits>

#include "Game.h"

using std::cout;
using std::endl;
using std::vector;
using std::string;

Game::Game(Player * p1, Player * p2, int limit) {
	isOver = false;
	lastSkipped = false;

	player1 = p1;
	player2 = p2;
	timeLimit = limit;

	currentPlayer = player1;
	currentState = GameState(p1->GetId(), p2->GetId());
}

Game::Game(Player * p1, Player * p2, int limit, GameState state, int currentId) {
	isOver = false;
	lastSkipped = false;

	player1 = p1;
	player2 = p2;
	timeLimit = limit;

	// Repopulate current state with correct ids
	vector<vector<int>> newBoard = state.GetBoard();
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			if (newBoard[i][j] == 1) {
				newBoard[i][j] = player1->GetId();
			} else if (newBoard[i][j] == 2) {
				newBoard[i][j] = player2->GetId();
			}
		}
	}
	
	currentState = GameState(newBoard);

	if (currentId == 1) {
		currentPlayer = player1;
	} else if (currentId == 2) {
		currentPlayer = player2;
	} else {
		currentPlayer = player1; // Default to player 1 if invalid
	}
}

void Game::PrintBoard() {
	// Initialize color and symbol arrays
	char normal[] = { 0x1b, '[', '0', ';', '3', '9', 'm', 0 };
	char blue[] = { 0x1b, '[', '1', ';', '3', '4', 'm', 0 };
	char pink[] = { 0x1b, '[', '1', ';', '3', '5', 'm', 0 };
    vector<char *> colors;
    colors.push_back(&normal[0]);
    colors.push_back(&blue[0]);
    colors.push_back(&pink[0]);
	char symbols[3] = { ' ', '*', '-' };

	// Print the board
    vector<vector<int>> board = currentState.GetBoard();
	cout << endl << "Current board:" << endl << endl;
	//cout << "Player 1 is " << colors[1] << symbols[0] << endl;
	//cout << "Player 2 is " << colors[2] << symbols[1] << endl;
	cout << "Player 1 is " << symbols[1] << endl;
	cout << "Player 2 is " << symbols[2] << endl << endl;
	cout << "     0   1   2   3   4   5   6   7  " << endl;
	cout << "   ---------------------------------" << endl;
	for (int i = 0; i < 8; ++i) {
		cout << "   ";
		for (int j = 0; j < 8; ++j) {
			cout << "|   ";
		}
		cout << "|" << endl << " " << i << " ";
		for (int j = 0; j < 8; ++j) {
			cout << "| ";
			char symbol = symbols[0];
			if (board[i][j] == player1->GetId()) {
				symbol = symbols[1];
			} else if (board[i][j] == player2->GetId()) {
				symbol = symbols[2];
			}
			char * color = colors[0];
			if (board[i][j] == player1->GetId()) {
				color = colors[1];
			} else if (board[i][j] == player2->GetId()) {
				color = colors[2];
			}
			cout << symbol << " ";
			//cout << "| " << color << symbol << colors[0] << " ";
		}
		cout << "|" << endl << "   ";
		for (int j = 0; j < 8; ++j) {
			cout << "|   ";
		}
		cout << "|" << endl;
		cout << "   ---------------------------------" << endl;
	}
}

void Game::Move() {
	Player * enemyPlayer = new HumanPlayer();
	cout << "Player ";
	if (currentPlayer->GetId() == player1->GetId()) {
		cout << "1";
		enemyPlayer = player2;
	} else if (currentPlayer->GetId() == player2->GetId()) {
		cout << "2";
		enemyPlayer = player1;
	}
	cout << " to move:" << endl;

	// If no legal moves, output notice and return
	if (LegalMoves(currentState, currentPlayer->GetId()).size() == 0) {
		// Check for game end
		if (lastSkipped) {
			isOver = true;
		} else {
			cout << "No legal moves available; skipping turn" << endl;
			lastSkipped = true;
		}
		return;
	}

	// Reset last skipped flag
	lastSkipped = false;


	// Get move from player; legality is checked here, so we will always get a legal move
	Location move = currentPlayer->MakeMove(currentState);

	// Get changed pieces
	vector<Location> changedPieces = GetChangedPieces(currentState, move, currentPlayer->GetId(), enemyPlayer->GetId());

	// Update state
	currentState = GameState::ApplyMove(currentState, changedPieces, currentPlayer->GetId());

	// Switch player
	currentPlayer = enemyPlayer;
}

vector<Location> Game::GetChangedPieces(GameState state, Location move, int currentId, int enemyId) {
	vector<vector<int>> board = state.GetBoard();

	// Compile a vector of all converted pieces
	vector<Location> changedPieces;
	changedPieces.push_back(move);

	// Check each direction for flanked enemies and add each one to changePieces

	vector<Location> adjacentEnemies = getAdjacentLocations(state, move, enemyId);
	for (unsigned int i = 0; i < adjacentEnemies.size(); ++i) {
		// We can include duplicates here since checking for them is simply less efficient than processing them twice (without repercussions)

		// Check direction of adjacent square
		if (adjacentEnemies[i].extras == "RIGHT") {
			// Create temporary vector to keep track of valid pieces
			vector<Location> tmpLocs;
			tmpLocs.push_back(adjacentEnemies[i]);

			// Enemy on the right, keep checking right
			int tmp = adjacentEnemies[i].column + 1;
			while (tmp < 8) {
				if (board[move.row][tmp] == 0) {
					break;
				} else if (board[move.row][tmp] == enemyId) {
					tmpLocs.push_back(Location(move.row, tmp));
				} else if (board[move.row][tmp] == currentId) {
					for (unsigned int j = 0; j < tmpLocs.size(); ++j) {
						changedPieces.push_back(tmpLocs[j]);
					}
					break;
				}
				++tmp;
			}
		} else if (adjacentEnemies[i].extras == "DOWNRIGHT") {
			// Create temporary vector to keep track of valid pieces
			vector<Location> tmpLocs;
			tmpLocs.push_back(adjacentEnemies[i]);

			// Empty on down right, keep checking down right
			int tmp1 = adjacentEnemies[i].row + 1, tmp2 = adjacentEnemies[i].column + 1;
			while (tmp1 < 8 && tmp2 < 8) {
				if (board[tmp1][tmp2] == 0) {
					break;
				} else if (board[tmp1][tmp2] == enemyId) {
					tmpLocs.push_back(Location(tmp1, tmp2));
				} else if (board[tmp1][tmp2] == currentId) {
					for (unsigned int j = 0; j < tmpLocs.size(); ++j) {
						changedPieces.push_back(tmpLocs[j]);
					}
					break;
				}
				++tmp1;
				++tmp2;
			}
		} else if (adjacentEnemies[i].extras == "DOWN") {
			// Create temporary vector to keep track of valid pieces
			vector<Location> tmpLocs;
			tmpLocs.push_back(adjacentEnemies[i]);

			// Enemy on the down, keep checking down
			int tmp = adjacentEnemies[i].row + 1;
			while (tmp < 8) {
				if (board[tmp][move.column] == 0) {
					break;
				} else if (board[tmp][move.column] == enemyId) {
					tmpLocs.push_back(Location(tmp, move.column));
				} else if (board[tmp][move.column] == currentId) {
					for (unsigned int j = 0; j < tmpLocs.size(); ++j) {
						changedPieces.push_back(tmpLocs[j]);
					}
					break;
				}
				++tmp;
			}
		} else if (adjacentEnemies[i].extras == "DOWNLEFT") {
			// Create temporary vector to keep track of valid pieces
			vector<Location> tmpLocs;
			tmpLocs.push_back(adjacentEnemies[i]);

			// Empty on down left, keep checking down left
			int tmp1 = adjacentEnemies[i].row + 1, tmp2 = adjacentEnemies[i].column - 1;
			while (tmp1 < 8 && tmp2 >= 0) {
				if (board[tmp1][tmp2] == 0) {
					break;
				} else if (board[tmp1][tmp2] == enemyId) {
					tmpLocs.push_back(Location(tmp1, tmp2));
				} else if (board[tmp1][tmp2] == currentId) {
					for (unsigned int j = 0; j < tmpLocs.size(); ++j) {
						changedPieces.push_back(tmpLocs[j]);
					}
					break;
				}
				++tmp1;
				--tmp2;
			}
		} else if (adjacentEnemies[i].extras == "LEFT") {
			// Create temporary vector to keep track of valid pieces
			vector<Location> tmpLocs;
			tmpLocs.push_back(adjacentEnemies[i]);

			// Enemy on the left, keep checking left
			int tmp = adjacentEnemies[i].column - 1;
			while (tmp >= 0) {
				if (board[move.row][tmp] == 0) {
					break;
				} else if (board[move.row][tmp] == enemyId) {
					tmpLocs.push_back(Location(move.row, tmp));
				} else if (board[move.row][tmp] == currentId) {
					for (unsigned int j = 0; j < tmpLocs.size(); ++j) {
						changedPieces.push_back(tmpLocs[j]);
					}
					break;
				}
				--tmp;
			}
		} else if (adjacentEnemies[i].extras == "UPLEFT") {
			// Create temporary vector to keep track of valid pieces
			vector<Location> tmpLocs;
			tmpLocs.push_back(adjacentEnemies[i]);

			// Empty on up left, keep checking up left
			int tmp1 = adjacentEnemies[i].row - 1, tmp2 = adjacentEnemies[i].column - 1;
			while (tmp1 >= 0 && tmp2 >= 0) {
				if (board[tmp1][tmp2] == 0) {
					break;
				} else if (board[tmp1][tmp2] == enemyId) {
					tmpLocs.push_back(Location(tmp1, tmp2));
				} else if (board[tmp1][tmp2] == currentId) {
					for (unsigned int j = 0; j < tmpLocs.size(); ++j) {
						changedPieces.push_back(tmpLocs[j]);
					}
					break;
				}
				--tmp1;
				--tmp2;
			}
		} else if (adjacentEnemies[i].extras == "UP") {
			// Create temporary vector to keep track of valid pieces
			vector<Location> tmpLocs;
			tmpLocs.push_back(adjacentEnemies[i]);

			// Enemy on the down, keep checking down
			int tmp = adjacentEnemies[i].row - 1;
			while (tmp >= 0) {
				if (board[tmp][move.column] == 0) {
					break;
				} else if (board[tmp][move.column] == enemyId) {
					tmpLocs.push_back(Location(tmp, move.column));
				} else if (board[tmp][move.column] == currentId) {
					for (unsigned int j = 0; j < tmpLocs.size(); ++j) {
						changedPieces.push_back(tmpLocs[j]);
					}
					break;
				}
				--tmp;
			}
		} else if (adjacentEnemies[i].extras == "UPRIGHT") {
			// Create temporary vector to keep track of valid pieces
			vector<Location> tmpLocs;
			tmpLocs.push_back(adjacentEnemies[i]);

			// Empty on up right, keep checking up right
			int tmp1 = adjacentEnemies[i].row - 1, tmp2 = adjacentEnemies[i].column + 1;
			while (tmp1 >= 0 && tmp2 < 8) {
				if (board[tmp1][tmp2] == 0) {
					break;
				} else if (board[tmp1][tmp2] == enemyId) {
					tmpLocs.push_back(Location(tmp1, tmp2));
				} else if (board[tmp1][tmp2] == currentId) {
					for (unsigned int j = 0; j < tmpLocs.size(); ++j) {
						changedPieces.push_back(tmpLocs[j]);
					}
					break;
				}
				--tmp1;
				++tmp2;
			}
		}

	}

	return changedPieces;
}

void Game::PrintResults() {
	// Compile statistics
	int player1Count = 0, player2Count = 0;
	vector<vector<int>> board = currentState.GetBoard();
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			if (board[i][j] == player1->GetId()) {
				++player1Count;
			} else if (board[i][j] == player2->GetId()) {
				++player2Count;
			}
		}
	}
	
	// Display results
	cout << "Game over!" << endl;
	cout << "Player " << (player1Count > player2Count ? "1" : "2") << " wins!" << endl;
	cout << player1Count << " - " << player2Count << endl;
}

std::vector<Location> Game::LegalMoves(GameState state, int id) {
	vector<vector<int>> board = state.GetBoard();

	vector<Location> validLocations;

	// Populate vectors of player locations and enemy locations
	vector<Location> playerLocations, enemyLocations;
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			if (board[i][j] == id) {
				playerLocations.push_back(Location(i, j));
			} else if (board[i][j] && board[i][j] != id) {
				enemyLocations.push_back(Location(i, j));
			}
		}
	}

	// LEGAL MOVES ALGORITHM:
	// For each enemy location
	//     Find adjacent open squares
	//	   For each adjacent open square
	//	       Check if that square is flanked by another friendly piece incident upon the current enemy piece
	for (unsigned int i = 0; i < enemyLocations.size(); ++i) {
		// Get adjacent locations
		vector<Location> emptyAdjacent = getAdjacentLocations(state, enemyLocations[i], 0);

		for (unsigned int j = 0; j < emptyAdjacent.size(); ++j) {
			// Don't check location if previously verified as valid
			if (std::find(validLocations.begin(), validLocations.end(), emptyAdjacent[j]) != validLocations.end()) {
				continue;
			}

			// Check direction of adjacent square
			if (emptyAdjacent[j].extras == "RIGHT") {
				// Empty on right, check squares to the left
				int tmp = enemyLocations[i].column - 1;
				while (tmp >= 0) {
					if (board[emptyAdjacent[j].row][tmp] == 0) {
						break;
					}
					if (board[emptyAdjacent[j].row][tmp] == id) {
						validLocations.push_back(emptyAdjacent[j]);
						break;
					}
					--tmp;
				}
			} else if (emptyAdjacent[j].extras == "DOWNRIGHT") {
				// Empty on down right, check squares to the up left
				int tmp1 = enemyLocations[i].row - 1, tmp2 = enemyLocations[i].column - 1;
				while (tmp1 >= 0 && tmp2 >= 0) {
					if (board[tmp1][tmp2] == 0) {
						break;
					}
					if (board[tmp1][tmp2] == id) {
						validLocations.push_back(emptyAdjacent[j]);
						break;
					}
					--tmp1;
					--tmp2;
				}
			} else if (emptyAdjacent[j].extras == "DOWN") {
				// Empty on down, check squares to the up
				int tmp = enemyLocations[i].row - 1;
				while (tmp >= 0) {
					if (board[tmp][emptyAdjacent[j].column] == 0) {
						break;
					}
					if (board[tmp][emptyAdjacent[j].column] == id) {
						validLocations.push_back(emptyAdjacent[j]);
						break;
					}
					--tmp;
				}
			} else if (emptyAdjacent[j].extras == "DOWNLEFT") {
				// Empty on down left, check squares to the up right
				int tmp1 = enemyLocations[i].row - 1, tmp2 = enemyLocations[i].column + 1;
				while (tmp1 >= 0 && tmp2 < 8) {
					if (board[tmp1][tmp2] == 0) {
						break;
					}
					if (board[tmp1][tmp2] == id) {
						validLocations.push_back(emptyAdjacent[j]);
						break;
					}
					--tmp1;
					++tmp2;
				}
			} else if (emptyAdjacent[j].extras == "LEFT") {
				// Empty on left, check squares to the right
				int tmp = enemyLocations[i].column + 1;
				while (tmp < 8) {
					if (board[emptyAdjacent[j].row][tmp] == 0) {
						break;
					}
					if (board[emptyAdjacent[j].row][tmp] == id) {
						validLocations.push_back(emptyAdjacent[j]);
						break;
					}
					++tmp;
				}
			} else if (emptyAdjacent[j].extras == "UPLEFT") {
				// Empty on up left, check squares to the down right
				int tmp1 = enemyLocations[i].row + 1, tmp2 = enemyLocations[i].column + 1;
				while (tmp1 < 8 && tmp2 < 8) {
					if (board[tmp1][tmp2] == 0) {
						break;
					}
					if (board[tmp1][tmp2] == id) {
						validLocations.push_back(emptyAdjacent[j]);
						break;
					}
					++tmp1;
					++tmp2;
				}
			} else if (emptyAdjacent[j].extras == "UP") {
				// Empty on up, check squares to the down
				int tmp = enemyLocations[i].row + 1;
				while (tmp < 8) {
					if (board[tmp][emptyAdjacent[j].column] == 0) {
						break;
					}
					if (board[tmp][emptyAdjacent[j].column] == id) {
						validLocations.push_back(emptyAdjacent[j]);
						break;
					}
					++tmp;
				}
			} else if (emptyAdjacent[j].extras == "UPRIGHT") {
				// Empty on up right, check squares to the down left
				int tmp1 = enemyLocations[i].row + 1, tmp2 = enemyLocations[i].column - 1;
				while (tmp1 < 8 && tmp2 >= 0) {
					if (board[tmp1][tmp2] == 0) {
						break;
					}
					if (board[tmp1][tmp2] == id) {
						validLocations.push_back(emptyAdjacent[j]);
						break;
					}
					++tmp1;
					--tmp2;
				}
			}

		}
	}

	return validLocations;
}

Game Game::FromFile(string fileName) {
	std::ifstream file(fileName);

	vector<vector<int>> b;
	for (int i = 0; i < 8; ++i) {
		vector<int> v;
		for (int j = 0; j < 8; ++j) {
			int a;
			file >> a;
			v.push_back(a);
		}
		b.push_back(v);
	}
	GameState state(b);

	string line;
	std::getline(file, line);
	std::getline(file, line);
	std::istringstream iss(line);
	int currentPlayerId;
	iss >> currentPlayerId;

	std::getline(file, line);
	std::istringstream iss2(line);
	int time;
	iss2 >> time;

	return Game(new ComputerPlayer(), new ComputerPlayer(), time, GameState(b), currentPlayerId);
}

vector<Location> Game::getAdjacentLocations(GameState state, Location l, int id) {
	vector<vector<int>> board = state.GetBoard();
	vector<Location> adjacent;

	if (l.row > 0) {
		if (board[l.row - 1][l.column] == id) {
			adjacent.push_back(Location(l.row - 1, l.column, "UP"));
		}
		
		if (l.column > 0) {
			if (board[l.row - 1][l.column - 1] == id) {
				adjacent.push_back(Location(l.row - 1, l.column - 1, "UPLEFT"));
			}
		}
		if (l.column < 7) {
			if (board[l.row - 1][l.column + 1] == id) {
				adjacent.push_back(Location(l.row - 1, l.column + 1, "UPRIGHT"));
			}
		}
	}

	if (l.row < 7) {
		if (board[l.row + 1][l.column] == id) {
			adjacent.push_back(Location(l.row + 1, l.column, "DOWN"));
		}

		if (l.column > 0) {
			if (board[l.row + 1][l.column - 1] == id) {
				adjacent.push_back(Location(l.row + 1, l.column - 1, "DOWNLEFT"));
			}
		}
		if (l.column < 7) {
			if (board[l.row + 1][l.column + 1] == id) {
				adjacent.push_back(Location(l.row + 1, l.column + 1, "DOWNRIGHT"));
			}
		}
	}

	if (l.column > 0) {
		if (board[l.row][l.column - 1] == id) {
			adjacent.push_back(Location(l.row, l.column - 1, "LEFT"));
		}
	}
	if (l.column < 7) {
		if (board[l.row][l.column + 1] == id) {
			adjacent.push_back(Location(l.row, l.column + 1, "RIGHT"));
		}
	}

	return adjacent;
}

double Game::MinimaxSearch(GameState state, int depth, int maxDepth, int currentId, int enemyId) {
	// Compile vector of children
	vector<GameState> children = getChildren(state, enemyId, currentId); // Get children from enemy's point of view

	// Simply evaluate if we are a leaf node or we are not searching any further
	if (!(maxDepth - depth) || !children.size()) {
		return heuristic(state, currentId, enemyId);
	}

	// Determine whether the current state is a max node or a min node based on depth
	bool maxNode = (depth + 1) % 2 == 0; // Since we are starting at min states (driver passes children of current state) we need to offset by 1

	if (maxNode) {
		double val = INT_MIN;
		for (unsigned int i = 0; i < children.size(); ++i) {
			double tmpVal = MinimaxSearch(children[i], depth + 1, maxDepth, currentId, enemyId); // Don't swap the enemy and current id!(????)
			if (tmpVal > val) {
				val = tmpVal;
			}
		}
		return val;
	} else {
		double val = INT_MAX;
		for (unsigned int i = 0; i < children.size(); ++i) {
			double tmpVal = MinimaxSearch(children[i], depth + 1, maxDepth, currentId, enemyId); // Don't swap the enemy and current id!(????)
			if (tmpVal < val) {
				val = tmpVal;
			}
		}
		return val;
	}
}

double Game::heuristic(GameState state, int currentId, int enemyId) {
	vector<vector<int>> grid = state.GetBoard();

	int myTiles = 0, enemyTiles = 0, i, j, k, myFrontTiles = 0, enemyFrontTiles = 0, x, y;
	double p = 0, c = 0, l = 0, m = 0, f = 0, d = 0;

	int X1 [] = { -1, -1, 0, 1, 1, 1, 0, -1 };
	int Y1 [] = { 0, 1, 1, 1, 0, -1, -1, -1 };
	int V[8][8] = {
		{ 20, -3, 11, 8, 8, 11, -3, 20 },
		{ -3, -7, -4, 1, 1, -4, -7, -3 },
		{ 11, -4, 2, 2, 2, 2, -4, 11 },
		{ 8, 1, 2, -3, -3, 2, 1, 8 },
		{ 8, 1, 2, -3, -3, 2, 1, 8 },
		{ 11, -4, 2, 2, 2, 2, -4, 11 },
		{ -3, -7, -4, 1, 1, -4, -7, -3 },
		{ 20, -3, 11, 8, 8, 11, -3, 20 }
	};

	// Piece difference, frontier disks and disk squares
	for (i = 0; i<8; i++)
		for (j = 0; j<8; j++)  {
			if (grid[i][j] == currentId)  {
				d += V[i][j];
				myTiles++;
			} else if (grid[i][j] == enemyId)  {
				d -= V[i][j];
				enemyTiles++;
			}
			if (grid[i][j] != 0)   {
				for (k = 0; k<8; k++)  {
					x = i + X1[k]; y = j + Y1[k];
					if (x >= 0 && x < 8 && y >= 0 && y < 8 && grid[x][y] == 0) {
						if (grid[i][j] == currentId)  myFrontTiles++;
						else enemyFrontTiles++;
						break;
					}
				}
			}
		}
	if (myTiles > enemyTiles)
		p = (100.0 * myTiles) / (myTiles + enemyTiles);
	else if (myTiles < enemyTiles)
		p = -(100.0 * enemyTiles) / (myTiles + enemyTiles);
	else p = 0;

	if (myFrontTiles > enemyFrontTiles)
		f = -(100.0 * myFrontTiles) / (myFrontTiles + enemyFrontTiles);
	else if (myFrontTiles < enemyFrontTiles)
		f = (100.0 * enemyFrontTiles) / (myFrontTiles + enemyFrontTiles);
	else f = 0;

	// Corner occupancy
	myTiles = enemyTiles = 0;
	if (grid[0][0] == currentId) myTiles++;
	else if (grid[0][0] == enemyId) enemyTiles++;
	if (grid[0][7] == currentId) myTiles++;
	else if (grid[0][7] == enemyId) enemyTiles++;
	if (grid[7][0] == currentId) myTiles++;
	else if (grid[7][0] == enemyId) enemyTiles++;
	if (grid[7][7] == currentId) myTiles++;
	else if (grid[7][7] == enemyId) enemyTiles++;
	c = 25 * (myTiles - enemyTiles);

	// Corner closeness
	myTiles = enemyTiles = 0;
	if (grid[0][0] == 0)   {
		if (grid[0][1] == currentId) myTiles++;
		else if (grid[0][1] == enemyId) enemyTiles++;
		if (grid[1][1] == currentId) myTiles++;
		else if (grid[1][1] == enemyId) enemyTiles++;
		if (grid[1][0] == currentId) myTiles++;
		else if (grid[1][0] == enemyId) enemyTiles++;
	}
	if (grid[0][7] == 0)   {
		if (grid[0][6] == currentId) myTiles++;
		else if (grid[0][6] == enemyId) enemyTiles++;
		if (grid[1][6] == currentId) myTiles++;
		else if (grid[1][6] == enemyId) enemyTiles++;
		if (grid[1][7] == currentId) myTiles++;
		else if (grid[1][7] == enemyId) enemyTiles++;
	}
	if (grid[7][0] == 0)   {
		if (grid[7][1] == currentId) myTiles++;
		else if (grid[7][1] == enemyId) enemyTiles++;
		if (grid[6][1] == currentId) myTiles++;
		else if (grid[6][1] == enemyId) enemyTiles++;
		if (grid[6][0] == currentId) myTiles++;
		else if (grid[6][0] == enemyId) enemyTiles++;
	}
	if (grid[7][7] == 0)   {
		if (grid[6][7] == currentId) myTiles++;
		else if (grid[6][7] == enemyId) enemyTiles++;
		if (grid[6][6] == currentId) myTiles++;
		else if (grid[6][6] == enemyId) enemyTiles++;
		if (grid[7][6] == currentId) myTiles++;
		else if (grid[7][6] == enemyId) enemyTiles++;
	}
	l = -12.5 * (myTiles - enemyTiles);

	// Mobility
	myTiles = LegalMoves(state, currentId).size();
	enemyTiles = LegalMoves(state, enemyId).size();
	if (myTiles > enemyTiles)
		m = (100.0 * myTiles) / (myTiles + enemyTiles);
	else if (myTiles < enemyTiles)
		m = -(100.0 * enemyTiles) / (myTiles + enemyTiles);
	else m = 0;

	// Final weighted score
	double score = (10 * p) + (801.724 * c) + (382.026 * l) + (78.922 * m) + (74.396 * f) + (10 * d);
	return score;
}

vector<GameState> Game::getChildren(GameState state, int currentId, int enemyId) {
	// Get all resulting states from legal moves
	std::vector<Location> legal = Game::LegalMoves(state, currentId);
	std::vector<GameState> newStates;
	for (unsigned int i = 0; i < legal.size(); ++i) {
		newStates.push_back(GameState::ApplyMove(state, Game::GetChangedPieces(state, legal[i], currentId, enemyId), currentId));
	}
	return newStates;
}