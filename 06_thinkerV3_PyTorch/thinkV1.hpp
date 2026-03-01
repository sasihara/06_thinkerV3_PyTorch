#pragma once
#include "othello.hpp"
#include "logging.h"

// Bit for disk character
#define DISKCHARFLAG_EXISTENCE	0x01		// Indicates disk exsits or not
#define DISKCHARFLAG_CHANGABLE	0x02		// Indicates disk can be changed or not

// Parameters for thinker
#define SEARCH_DEPTH				6
#define NUM_FOR_GAMESTATE_END		12
//#define SEARCH_DEPTH				7
//#define NUM_FOR_GAMESTATE_END		14
#define NUM_FOR_GAMESTATE_MIDFIELD	48

#define	FIXED_DISK_WEIGHT			32

enum class GAMESTATE {
	GAMESTATE_EARLY_STAGE = 0,
	GAMESTATE_MIDFIELD,
	GAMESTATE_END
};

class ThinkerV1 {
public:
	int SetParams(int turn, DISKCOLORS board[64]);
	int think();
	void analyzeDiskCharacter(DISKCOLORS board[64], int result[64]);

private:
	DISKCOLORS board[64];
	int turn;
	GAMESTATE thinkerState = GAMESTATE::GAMESTATE_EARLY_STAGE;
	DISKCOLORS currentPlayer, opponent;
	int CheckPosX[60] = {
		0, 0, 7, 7,
		2, 2, 3, 4, 5, 5, 3, 4,
		1, 1, 3, 4, 6, 6, 3, 4,
		2, 2, 5, 5,
		0, 0, 2, 5, 7, 7, 2, 5,
		1, 1, 2, 5, 6, 6, 2, 5,
		0, 0, 3, 4, 7, 7, 3, 4,
		0, 0, 1, 6, 7, 7, 1, 6,
		1, 1, 6, 6
	};
	
	int CheckPosY[60] = {
		0, 7, 0, 7,
		3, 4, 5, 5, 3, 4, 2, 2,
		3, 4, 6, 6, 3, 4, 1, 1,
		2, 5, 5, 2,
		2, 5, 7, 7, 2, 5, 0, 0,
		2, 5, 6, 6, 2, 5, 1, 1,
		3, 4, 7, 7, 3, 4, 0, 0,
		1, 6, 7, 7, 1, 6, 0, 0,
		1, 6, 1, 6
	};

	int weight[2][8][8] = {
		// Weight for Early stage (1 - 16)
		{
			 32,-24, 16,  2,  2, 16,-24, 32,
			-24,-24,-12, -4, -4,-12,-24,-24,
			 16,-12,  4,  1,  1,  4,-12, 16,
			  2, -4,  1,  1,  1,  1, -4,  2,
			  2, -4,  1,  1,  1,  1, -4,  2,
			 16,-12,  4,  1,  1,  4,-12, 16,
			-24,-24,-12, -4, -4,-12,-24,-24,
			 32,-24, 16,  2,  2, 16,-24, 32
		},
		// Weight for Mid stage (17 - 36)
		{
			 32,-24, 16,  2,  2, 16,-24, 32,
			-24,-24,-12, -4, -4,-12,-24,-24,
			 16,-12,  4,  1,  1,  4,-12, 16,
			  2, -4,  1,  1,  1,  1, -4,  2,
			  2, -4,  1,  1,  1,  1, -4,  2,
			 16,-12,  4,  1,  1,  4,-12, 16,
			-24,-24,-12, -4, -4,-12,-24,-24,
			 32,-24, 16,  2,  2, 16,-24, 32
		}
	};
	
	int CountDisk(DISKCOLORS color, DISKCOLORS _board[64]);
	int findBestPlaceForCurrentPlayer(int lv);
	int MaxLevel(int lv, bool f, int beta, DISKCOLORS _board[64]);
	int MinLevel(int lv, bool f, int alpha, DISKCOLORS _board[64]);
	int evcal(DISKCOLORS _board[64]);
	bool isFixed(int x, int y, DISKCOLORS board[64]);
	bool isFixedOneDir(int x, int y, DISKCOLORS board[64], int dx, int dy);
	bool isPatternToFix(int code);
	int check(DISKCOLORS board[64], int xPos, int yPos, DISKCOLORS color);
	int checkOneDir(DISKCOLORS board[64], int xPos, int yPos, DISKCOLORS color, int xStep, int yStep);
	int turnDisk(DISKCOLORS board[64], int xPos, int yPos, DISKCOLORS color, int flag);
	int turnDiskOneDir(DISKCOLORS board[64], int xPos, int yPos, DISKCOLORS color, int xStep, int yStep);
};

int logoutBoard(Logging logging, DISKCOLORS* _board);
int logoutAnalysisResult(Logging logging, int* _result);
