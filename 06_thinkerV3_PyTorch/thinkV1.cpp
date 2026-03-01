// Othello Thinker Version 3.00
// Copyright (C) 1994  T.Sashihara
// This program goes with Othello for Windows Ver 3.00

#include <stdio.h>
#include <memory.h>
#include <limits.h>
#include "externalThinkerMessages.hpp"
#include "thinkV1.hpp"

extern Logging logging;

//
//	Function Name: SetParams
//	Summary: Set parameters for thinker.
//	
//	In:
//		_turn:	The turn value.
//		_board:	The board data.
//
//	Return:
//		0: Succeed
//
int ThinkerV1::SetParams(int _turn, DISKCOLORS _board[64])
{
	memcpy(board, _board, sizeof(board));
	turn = _turn;
	return 0;
}

//
//	Function Name: think
//	Summary: Set the search depth and then call findBestPlaceForCurrentPlayer to find the best place for the current player.
//	
//	In:
//		No parameters.
//
//	Return:
//		Position for the disk. If the position is (x, y), the result value is x * 10 + y.
//
int ThinkerV1::think()
{
	int depth;
	int ret;
	int numSpaceLeft;

	LOGOUT(LOGLEVEL_TRACE, "★==================== think() start. ====================★");

#ifdef _DEBUG
	logging.logprintf("*** 現在の盤面 ***\n");
	logging.logprintf("プレイヤーの石の色: %s\n", currentPlayer == DISKCOLORS::COLOR_BLACK ? "●" : currentPlayer == DISKCOLORS::COLOR_WHITE ? "○" : "");
	ret = logoutBoard(logging, board);
#endif

	// Decide thinking mode.
	numSpaceLeft = CountDisk(DISKCOLORS::COLOR_NONE, board);
	LOGOUT(LOGLEVEL_TRACE, "残り石数 = %d.", numSpaceLeft);

	if (numSpaceLeft <= NUM_FOR_GAMESTATE_END) {
		depth = INT_MAX;
		thinkerState = GAMESTATE::GAMESTATE_END;
		LOGOUT(LOGLEVEL_TRACE, "完全読みモードへ移ります.");
	}
	else if (numSpaceLeft <= NUM_FOR_GAMESTATE_MIDFIELD) {
		depth = SEARCH_DEPTH;
		thinkerState = GAMESTATE::GAMESTATE_MIDFIELD;
		LOGOUT(LOGLEVEL_TRACE, "中盤モードで進めます.");
	}
	else {
		depth = SEARCH_DEPTH;
		thinkerState = GAMESTATE::GAMESTATE_EARLY_STAGE;
		LOGOUT(LOGLEVEL_TRACE, "序盤モードで進めます.");
	}

	// Player detection
	currentPlayer = CURRENTPLAYER(turn);
	opponent = OPPONENT(currentPlayer);

	// Start to think.
	ret = findBestPlaceForCurrentPlayer(depth);

	LOGOUT(LOGLEVEL_TRACE, "★==================== think() end. ====================★");
	return ret;
}

//
//	Function Name: CountDisk
//	Summary: Count the number of dice of the specified color on the specified board.
//	
//	In:
//		_color: The disk color to count.
//		_board: The board to count.
//
//	Return:
//		The number of dice.
//
int ThinkerV1::CountDisk(DISKCOLORS color, DISKCOLORS _board[64])
{
	int c = 0, i;

	for (i = 0; i < 64; i++) {
		if (_board[i] == color) c++;
	}
	return c;
}

//
//	Function Name: findBestPlaceForCurrentPlayer
//	Summary: Find the best place for the current player in the specified depth.
//	
//	In:
//		lv:	Search depth.
//
//	Return:
//		The best place to put current player's disk. If the place is (x, y), the return value is x * 10 + y.
//
int ThinkerV1::findBestPlaceForCurrentPlayer(int lv)
{
	int i, eval = INT_MIN, score;
	int flag;
	DISKCOLORS tmpBoard[64];
	char x = -1, y = -1;

	for (i = 0; i < 60; i++) {
		if ((flag = check(board, CheckPosX[i], CheckPosY[i], currentPlayer)) > 0) {
			memcpy(tmpBoard, board, sizeof(tmpBoard));
			turnDisk(tmpBoard, CheckPosX[i], CheckPosY[i], currentPlayer, flag);
			score = MinLevel(lv - 1, false, eval, tmpBoard);
			if (eval < score) {
				x = CheckPosX[i];
				y = CheckPosY[i];
				eval = score;
			}
		}
	}

	return x * 10 + y;
}

//
//	Function Name: MaxLevel
//	Summary: Get the score for the current board for the current player.
//	
//	In:
//		lv:	Depth to search.
//		f:	
//		beta:
//		_board: Board to evaluate.
//
//	Return:
//
int ThinkerV1::MaxLevel(int lv, bool f, int beta, DISKCOLORS _board[64])
{
	bool pf;
	int alpha = INT_MIN, i, a;
	DISKCOLORS tmpBoard[64];
	int flag;

	if (lv == 0) return evcal(_board);
	pf = false;

	for (i = 0; i < 60; i++) {
		if ((flag = check(_board, CheckPosX[i], CheckPosY[i], currentPlayer)) > 0) {
			pf = true;
			memcpy(tmpBoard, _board, sizeof(tmpBoard));
			turnDisk(tmpBoard, CheckPosX[i], CheckPosY[i], currentPlayer, flag);
			a = MinLevel(lv - 1, false, alpha, tmpBoard);
			if (alpha < a) alpha = a;
			if (alpha >= beta) return alpha;
		}
	}
	if (pf != false) return alpha;
	if (f == true) return evcal(_board);
	return MinLevel(lv, true, alpha, _board);
}

//
//	Function Name: MinLevel
//	Summary: Get the score for the specified board for the the opponent of the current player.
//	
//	In:
//		lv: 
//		f: 
//		alpha: 
//		_board: 
//
//	Return:
//		Score.
//
int ThinkerV1::MinLevel(int lv, bool f, int alpha, DISKCOLORS _board[64])
{
	bool pf;
	int beta = INT_MAX, i, a;
	DISKCOLORS tmpBoard[64];
	int flag;

	if (lv == 0) return evcal(_board);
	pf = false;

	for (i = 0; i < 60; i++) {
		if ((flag = check(_board, CheckPosX[i], CheckPosY[i], opponent)) > 0) {
			pf = true;
			memcpy(tmpBoard, _board, sizeof(tmpBoard));
			turnDisk(tmpBoard, CheckPosX[i], CheckPosY[i], opponent, flag);
			a = MaxLevel(lv - 1, false, beta, tmpBoard);
			if (beta > a) beta = a;
			if (beta <= alpha) return beta;
		}
	}
	if (pf != false) return beta;
	if (f == true) return evcal(_board);
	return MaxLevel(lv, true, beta, _board);
}

//
//	Function Name: evcal
//	Summary: Evaluate the board for the current player.
//	
//	In:
//		board:	Board to evaluate.
//
//	Return:
//		Score.
//
int ThinkerV1::evcal(DISKCOLORS board[64])
{
	size_t i;
	int c = 0;
	int result[64];
	int ret;

	LOGOUT(LOGLEVEL_TRACE, "evcal() start.");

#ifdef _DEBUG
	logging.logprintf("プレイヤーの石の色: %s\n", currentPlayer == DISKCOLORS::COLOR_BLACK ? "●" : currentPlayer == DISKCOLORS::COLOR_WHITE ? "○" : "");
	ret = logoutBoard(logging, board);
#endif

	if (thinkerState == GAMESTATE::GAMESTATE_END) {
		//for (i = 0; i < 60; i++) {
		//	if (board[CheckPosX[i] * 8 + CheckPosY[i]] == currentPlayer) c++;
		//	else if (board[CheckPosX[i] * 8 + CheckPosY[i]] == opponent) c--;
		//}
		for (i = 0; i < 64; i++) {
			if (board[i] == currentPlayer) c++;
			else if (board[i] == opponent) c--;
		}

		LOGOUT(LOGLEVEL_TRACE, "完全読みモードでの評価値 = %d", c);
		LOGOUT(LOGLEVEL_TRACE, "evcal() end.");

		return c;
	}
	else {
		analyzeDiskCharacter(board, result);
#ifdef _DEBUG
		logging.logprintf("***** 分析結果 *****\n");
		logoutAnalysisResult(logging, result);
#endif

		for (i = 0; i < 64; i++) {
			if (board[i] == currentPlayer) {
				if ((result[i] & (DISKCHARFLAG_EXISTENCE | DISKCHARFLAG_CHANGABLE)) == DISKCHARFLAG_EXISTENCE)
					c += FIXED_DISK_WEIGHT;
				else c += weight[(int)thinkerState][i / 8][i % 8];
			}
			else if (board[i] == opponent) {
				if ((result[i] & (DISKCHARFLAG_EXISTENCE | DISKCHARFLAG_CHANGABLE)) == DISKCHARFLAG_EXISTENCE)
					c -= FIXED_DISK_WEIGHT;
				else c -= weight[(int)thinkerState][i / 8][i % 8];
			}
		}

		LOGOUT(LOGLEVEL_TRACE, "序盤：中盤モードでの評価値 = %d", c);
		LOGOUT(LOGLEVEL_TRACE, "evcal() end.");

		return c;
	}
}

//
//	Function Name: check
//	Summary: Check if the specified color's disk can put the specified place or not.
//	
//	In:
//		board:		The pointer to the board array.
//		xPos, yPos:	The position to put the disk.
//		color:		The disk color to put.
//
//	Return:
//		The flag which direction's disks will turn by putting the specified disk at the specified place.
//			bit 1:	Upper.
//			bit 2:	Upper right.
//			bit 3:	Right.
//			bit 4:	Lower right.
//			bit 5:	Lower.
//			bit 6:	Lower left.
//			bit 7:	Left.
//			bit 8:	Upper Left.

int ThinkerV1::check(DISKCOLORS board[64], int xPos, int yPos, DISKCOLORS color)
{
	int ret = 0;

	if (xPos < 0 || xPos > 7 || yPos < 0 || yPos > 7) return 0;
	if (board[xPos * 8 + yPos] != DISKCOLORS::COLOR_NONE) return 0;
	if (checkOneDir(board, xPos, yPos, color, 0, -1) > 0) ret = ret | 1;
	if (checkOneDir(board, xPos, yPos, color, 1, -1) > 0) ret = ret | 2;
	if (checkOneDir(board, xPos, yPos, color, 1, 0) > 0) ret = ret | 4;
	if (checkOneDir(board, xPos, yPos, color, 1, 1) > 0) ret = ret | 8;
	if (checkOneDir(board, xPos, yPos, color, 0, 1) > 0) ret = ret | 16;
	if (checkOneDir(board, xPos, yPos, color, -1, 1) > 0) ret = ret | 32;
	if (checkOneDir(board, xPos, yPos, color, -1, 0) > 0) ret = ret | 64;
	if (checkOneDir(board, xPos, yPos, color, -1, -1) > 0) ret = ret | 128;

	return ret;
}

//
//	Function Name: checkOneDir
//	Summary: Return the number of disks will be turned in the specified direction if the specified disk put to the specified place.
//
//	In:
//		board:			The pointer to the board array.
//		xPos, yPos:		The position to put the disk.
//		color:			The disk color to put.
//		xStep, yStep:	The direction to be checked.
//
//	Return:	The number of disks to be turned.
//
int ThinkerV1::checkOneDir(DISKCOLORS board[64], int xPos, int yPos, DISKCOLORS color, int xStep, int yStep)
{
	// Check parameters
	if (xPos < 0 || xPos > 7 || yPos < 0 || yPos > 7) return 0;
	if (color != DISKCOLORS::COLOR_BLACK && color != DISKCOLORS::COLOR_WHITE) return 0;
	if (xStep < -1 || xStep > 1 || yStep < -1 || yStep > 1) return 0;

	// Check if the position is empty or not
	if (board[xPos * 8 + yPos] != DISKCOLORS::COLOR_NONE) return 0;

	// Check if the next place is on the board or not
	if (xPos + xStep < 0 || xPos + xStep > 7 || yPos + yStep < 0 || yPos + yStep > 7) return 0;

	// Count the number of disks to be turned
	int x, y, NumTurned = 0;
	for (x = xPos + xStep, y = yPos + yStep; 0 <= x && x <= 7 && 0 <= y && y <= 7; x += xStep, y += yStep) {
		int i = x * 8 + y;
		if (board[i] == color) return NumTurned;				// meeting player's disk
		else if (board[i] == DISKCOLORS::COLOR_NONE) return 0;				// reached to empty without meeting player's disk
		else if (board[i] == OPPONENT(color)) NumTurned++;		// meeting opponent's disk
		else return 0;												// Illegal case
	}

	// In this case, reached to the edge without meeting player's disk
	return 0;
}

//
//	Function Name: turnDisk
//	Summary: Turn disks by putting a disk at the specified place of the specified color.
//
//	In:
//		board:			The pointer to the board array.
//		xPos, yPos:		The position to put the disk.
//		color:			The disk color to put.
//		flag:			The result of check() function.
//
//	Return:	The number of disks to be turned.
//
int ThinkerV1::turnDisk(DISKCOLORS board[64], int xPos, int yPos, DISKCOLORS color, int flag)
{
	if (xPos < 0 || xPos > 7 || yPos < 0 || yPos > 7) return 0;
	if (board[xPos * 8 + yPos] != DISKCOLORS::COLOR_NONE) return 0;
	if (flag & 1) turnDiskOneDir(board, xPos, yPos, color, 0, -1);
	if (flag & 2) turnDiskOneDir(board, xPos, yPos, color, 1, -1);
	if (flag & 4) turnDiskOneDir(board, xPos, yPos, color, 1, 0);
	if (flag & 8) turnDiskOneDir(board, xPos, yPos, color, 1, 1);
	if (flag & 16) turnDiskOneDir(board, xPos, yPos, color, 0, 1);
	if (flag & 32) turnDiskOneDir(board, xPos, yPos, color, -1, 1);
	if (flag & 64) turnDiskOneDir(board, xPos, yPos, color, -1, 0);
	if (flag & 128) turnDiskOneDir(board, xPos, yPos, color, -1, -1);

	return 0;
}

//
//	Function Name: turnDiskOneDir
//	Summary: Turn disks by putting a disk at the specified place of the specified color in the specified direction.
//
//	In:
//		board:			The pointer to the board array.
//		xPos, yPos:		The position to put the disk.
//		color:			The disk color to put.
//		xStep, yStep:	The direction to be turned.
//
//	Return:	The number of disks to be turned.
//
int ThinkerV1::turnDiskOneDir(DISKCOLORS board[64], int xPos, int yPos, DISKCOLORS color, int xStep, int yStep)
{
	// Place player's disk on the board
	board[xPos * 8 + yPos] = color;

	// Turn disks
	int x, y;
	for (x = xPos + xStep, y = yPos + yStep; 0 <= x && x <= 7 && 0 <= y && y <= 7; x += xStep, y += yStep) {
		int i = x * 8 + y;
		if (board[i] == color) break;
		board[i] = color;
	}

	return 0;
}

//
//	Function Name: analyzeDiskCharacter
//	Summary: Analyze the characteristic for dice on the board.
//	
//	In:
//		board:	Board to evaluate.
//		result:	Address to store the result.
//
//	Return:
//		No return value.
//
void ThinkerV1::analyzeDiskCharacter(DISKCOLORS board[64], int result[64])
{
	int i;

	memset(result, 0, sizeof(result));

	for (i = 0; i < 64; i++) {
		if (board[i] != DISKCOLORS::COLOR_NONE) result[i] |= DISKCHARFLAG_EXISTENCE;
		if (board[i] != DISKCOLORS::COLOR_NONE && isFixed(i / 8, i % 8, board) == false) {
			result[i] |= DISKCHARFLAG_CHANGABLE;
		}
	}
}

//
//	Function Name: isFixed
//	Summary: Check if the specified disk will never be changed or not.
//	
//	In:
//		x, y: The position for the disk to check.
//		board: The board to check.
//
//	Return:
//		true:	The disk will never be changed.
//		false:	The disk can be changed.
//
bool ThinkerV1::isFixed(int x, int y, DISKCOLORS board[64])
{
	if (isFixedOneDir(x, y, board, 1, 0) == false) return false;
	if (isFixedOneDir(x, y, board, 1, 1) == false) return false;
	if (isFixedOneDir(x, y, board, 0, 1) == false) return false;
	if (isFixedOneDir(x, y, board, -1, 1) == false) return false;
	return true;
}

//
//	Function Name: isFixedOneDir
//	Summary: Check if the specified disk will never be changed or not for the specified direction.
//	
//	In:
//		x, y: The position for the disk to check.
//		board: The board to check.
//		dx, dy: The direction to check.
//
//	Return:
//		true:	The disk will never be changed for the specified direction.
//		false:	The disk can be changed for the specified direction.
//
bool ThinkerV1::isFixedOneDir(int x, int y, DISKCOLORS board[64], int dx, int dy)
{
	int cx, cy;
	DISKCOLORS playerColorInThisCheck, opponentColorInThisCheck;
	bool isExistOpponentDisk = false;

	// Input check
	if (x < 0 || x >= 8) return false;
	if (y < 0 || y >= 8) return false;
	if (board[x * 8 + y] == DISKCOLORS::COLOR_NONE) return false;

	// Store disk colors in this check
	playerColorInThisCheck = board[x * 8 + y];
	opponentColorInThisCheck = OPPONENT(playerColorInThisCheck);

	// Check if the disk color at the specified place continues to the edge
	// Check forward
	cx = x + dx; cy = y + dy;
	for (;; cx += dx, cy += dy) {
		// If player's color continues to the edge, this disk will never change to the opponent
		if (cx < 0 || cx >= 8 || cy < 0 || cy >= 8) 
			return true;
		// if empty square exists before reaching to the edge, 
		else if (board[cx * 8 + cy] == DISKCOLORS::COLOR_NONE || board[cx * 8 + cy] == opponentColorInThisCheck)
			break;
	}

	// Check backward
	cx = x - dx; cy = y - dy;
	for (;; cx -= dx, cy -= dy) {
		// If player's color continues to the edge, this disk will never change to the opponent
		if (cx < 0 || cx >= 8 || cy < 0 || cy >= 8)
			return true;
		// if empty square exists before reaching to the edge, 
		else if (board[cx * 8 + cy] == DISKCOLORS::COLOR_NONE || board[cx * 8 + cy] == opponentColorInThisCheck)
			break;
	}

	// Check if the disk pattern for the right and left side of the checking disk are the pattern
	// which the checking disk will never turn to the opponent or not.
	// At first, check the right side.
	int patternCode = 0;
	cx = x + dx; 
	cy = y + dy;

	// Convert the disks for the right side into its code
	for (; 0 <= cx && cx < 8 && 0 <= cy && cy < 8 ; cx += dx, cy += dy) {
		int i = cx * 8 + cy;
		patternCode = patternCode << 2;
		if (board[i] == DISKCOLORS::COLOR_NONE) {
			patternCode = patternCode | 0x01;
		}
		else if (board[i] == opponentColorInThisCheck) {
			patternCode = patternCode | 0x02;
		}
		else if (board[i] == playerColorInThisCheck) {
			patternCode = patternCode | 0x03;
		}
	}

	// Check if the pattern exists in the patterns to fix the checking disk.
	if (isPatternToFix(patternCode) == false) return false;
	
	// Check the left side.
	patternCode = 0;
	cx = x - dx;
	cy = y - dy;

	// Convert the disks for the left side into its code
	for (; 0 <= cx && cx < 8 && 0 <= cy && cy < 8; cx -= dx, cy -= dy) {
		int i = cx * 8 + cy;
		patternCode = patternCode << 2;
		if (board[i] == DISKCOLORS::COLOR_NONE) {
			patternCode = patternCode | 0x01;
		}
		else if (board[i] == opponentColorInThisCheck) {
			patternCode = patternCode | 0x02;
		}
		else if (board[i] == playerColorInThisCheck) {
			patternCode = patternCode | 0x03;
		}
	}

	// Check if the pattern exists in the patterns to fix the checking disk.
	if (isPatternToFix(patternCode) == false) return false;
	else return true;
}

//
//	Function Name: isPatternToFix
//	Summary: Check if the dice pattern satisfies the condition to decide the disk will never be changed or not.
//	
//	In:
//		code: The code of the dice pattern.
//
//	Return:
//		true: The pattern satisfies the condition.
//		false: The pattern doesn't satisfy the condition.
//
bool ThinkerV1::isPatternToFix(int code)
{
	int patternsToFix[] = {
		0,2,3,9,10,11,14,15,38,39,
		41,42,43,45,46,47,57,58,59,63,
		154,155,158,159,166,167,169,170,171,173,
		174,175,181,182,183,185,186,187,189,190,
		191,230,231,233,234,235,237,238,239,249,
		250,251,254,255,614,615,617,618,619,622,
		623,633,634,635,638,639,666,667,670,671,
		678,679,681,682,683,685,686,687,693,694,
		695,697,698,699,701,702,703,726,727,729,
		730,731,733,734,735,741,742,743,746,747,
		749,750,751,757,758,759,761,762,763,767,
		921,922,923,926,927,934,935,937,938,939,
		941,942,943,949,950,951,953,954,955,957,
		958,959,998,999,1001,1002,1003,1005,1006,1007,
		1017,1018,1019,1022,1023,2457,2458,2459,2462,2463,
		2470,2471,2473,2474,2475,2478,2479,2489,2490,2491,
		2494,2495,2534,2535,2537,2538,2539,2541,2542,2543,
		2553,2554,2555,2558,2559,2665,2666,2667,2669,2670,
		2671,2681,2682,2683,2686,2687,2714,2715,2718,2719,
		2726,2727,2729,2730,2731,2733,2734,2735,2741,2742,
		2743,2745,2746,2747,2749,2750,2751,2774,2775,2777,
		2778,2779,2781,2782,2783,2789,2790,2791,2793,2794,
		2795,2797,2798,2799,2805,2806,2807,2809,2810,2811,
		2813,2814,2815,2905,2906,2907,2910,2911,2918,2919,
		2921,2922,2923,2925,2926,2927,2933,2934,2935,2937,
		2938,2939,2941,2942,2943,2965,2966,2967,2969,2970,
		2971,2973,2974,2975,2981,2982,2983,2985,2986,2987,
		2989,2990,2991,2997,2998,2999,3001,3002,3003,3005,
		3006,3007,3030,3031,3033,3034,3035,3037,3038,3039,
		3045,3046,3047,3049,3050,3051,3053,3054,3055,3061,
		3062,3063,3065,3066,3067,3069,3070,3071,3686,3687,
		3689,3690,3694,3695,3705,3706,3710,3711,3737,3738,
		3739,3741,3742,3743,3750,3751,3753,3754,3755,3757,
		3758,3759,3765,3766,3767,3769,3770,3771,3773,3774,
		3775,3798,3799,3801,3802,3803,3805,3806,3807,3813,
		3814,3815,3817,3818,3819,3821,3822,3823,3829,3830,
		3831,3833,3834,3835,3837,3838,3839,3993,3994,3995,
		3997,3998,3999,4006,4007,4009,4010,4011,4013,4014,
		4015,4021,4022,4023,4025,4026,4027,4029,4030,4031,
		4070,4071,4073,4074,4075,4077,4078,4079,4089,4090,
		4091,4094,4095,-1
	};

	for (int i = 0; patternsToFix[i] >= 0 ; i++) {
		if (code == patternsToFix[i]) return true;
	}

	return false;
}

int logoutBoard(Logging logging, DISKCOLORS* _board)
{
	for (int y = 0; y < 8; y++) {
		for (int x = 0; x < 8; x++) {
			switch (_board[x * 8 + y]) {
			case DISKCOLORS::COLOR_BLACK:
				logging.logprintf(LOGLEVEL_TRACE, "●");
				break;
			case DISKCOLORS::COLOR_WHITE:
				logging.logprintf(LOGLEVEL_TRACE, "○");
				break;
			default:
				logging.logprintf(LOGLEVEL_TRACE, "・");
				break;
			}
		}
		logging.logprintf(LOGLEVEL_TRACE, "\n");
	}

	return 0;
}

int logoutAnalysisResult(Logging logging, int *_result)
{
	for (int y = 0; y < 8; y++) {
		for (int x = 0; x < 8; x++) {
			switch (_result[x * 8 + y] & (DISKCHARFLAG_EXISTENCE | DISKCHARFLAG_CHANGABLE)) {
			case DISKCHARFLAG_EXISTENCE:
				logging.logprintf(LOGLEVEL_TRACE, "◎");
				break;
			case DISKCHARFLAG_EXISTENCE | DISKCHARFLAG_CHANGABLE:
				logging.logprintf(LOGLEVEL_TRACE, "○");
				break;
			default:
				logging.logprintf(LOGLEVEL_TRACE, "・");
				break;
			}
		}
		logging.logprintf(LOGLEVEL_TRACE, "\n");
	}

	return 0;
}