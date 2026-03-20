#pragma once
#include "externalThinkerMessages.hpp"
#include "main.hpp"
//#include "TFHandler.hpp"
#include "PTHHandler.hpp"

#define MODELINFO	"(Unknown Model)"

#define SP_TEMPERATURE 1.0
//#define SP_TEMPERATURE 4.0
#define PV_EVALUATE_COUNT 100

enum class THINKARC{
	THINKARC_DEEP = 0,
	THINKARC_MINMAX,
	THINKARC_MINMAX_MP
};

typedef int Action;

typedef struct _POLICY {
	int x;
	int y;
	float policy;
} Policy;

typedef struct _ThinkerInitParam {
	RunningMode runningMode;
	double spTemperature;
	int numIterations = PV_EVALUATE_COUNT;
	bool isBreadthFirst = false;
	bool limitTemperaturePeriod = false;
	int gpuid = -1;
	bool forceGPU = true;
	THINKARC thinkArc = THINKARC::THINKARC_DEEP;
} ThinkerInitParam;

class Thinker {
public:
	int init(ThinkerInitParam *thinkerInitParam);
	int think(int turn, DISKCOLORS* board, int *place, GameId gameId);
	char* getModelInfo();
	~Thinker();

private:
	bool isInitialized = false;
	Model model;
	char modelInfo[1024];
	double spTemperature = SP_TEMPERATURE;
	int numIterations = PV_EVALUATE_COUNT;
	bool isBreadthFirst = false;
	RunningMode runningMode;
	bool limitTemperaturePeriod = false;
	int gpuid = -1;		// -1: not specify, 0-: gpuid
	THINKARC thinkArc;

	int CountDisk(DISKCOLORS color, DISKCOLORS _board[64]);
};