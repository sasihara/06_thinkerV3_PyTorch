#pragma once
#include "externalThinkerMessages.hpp"
#include "main.hpp"
//#include "TFHandler.hpp"
#include "PTHHandler.hpp"

#define MODELINFO	"(Unknown Model)"

#define SP_TEMPERATURE 1.0
//#define SP_TEMPERATURE 4.0
#define PV_EVALUATE_COUNT 100

typedef int Action;

typedef struct _POLICY {
	int x;
	int y;
	float policy;
} Policy;

class Thinker {
public:
	int init(RunningMode _runningMode, double _spTemperature, int _numIterations = PV_EVALUATE_COUNT, bool _isBreadthFirst = false, bool _limitTemperaturePeriod = false);
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

	int CountDisk(DISKCOLORS color, DISKCOLORS _board[64]);
};