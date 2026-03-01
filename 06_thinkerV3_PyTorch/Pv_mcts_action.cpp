#include <float.h>
#include <algorithm>
#include "common.h"
#include "Pv_mcts.action.hpp"
//#include "TFHandler.hpp"
#include "PTHHandler.hpp"
#include "Node.hpp"
#include "logging.h"
#include "history.hpp"

// Global
extern Logging logging;
extern History history;

Pv_mcts_action::Pv_mcts_action(Model* _model, Temperature _temperature, int _numIterations, bool _isBreadthFirst)
{
	model = _model;
	temperature = _temperature;
	numIterations = _numIterations;
	isBreadthFirst = _isBreadthFirst;
}

int Pv_mcts_action::run(State* state, Action* action, GameId gameId)
{
	int ret;
	std::vector<Score> scores;

	LOGOUT(LOGLEVEL_TRACE, "========== Pv_mcts_action::run()開始. ==========");

	// 盤面のログ出力
	state->logout(logging);

	// pv_mcts_scores()
	ret = pv_mcts_scores(state, &scores);

	// 戻り値のチェック
	if (ret < 0) {
		return -1;
	}

	// 学習データの保存
	ret = history.add(gameId, state->currentPlayer, state->board, scores);
	if (ret < 0) {
		return -2;
	}

	// np.random.choice
	if (scores.size() > 0) {
		// 合法手が存在する場合
		ret = ranom_choice(state, scores, action);

		// 戻り値のチェック
		if (ret < 0) {
			return -3;
		}
	}
	else {
		// 合法手が存在しなかった場合
		// パスを示すx=8, y=0をセット
		*action = 80;
	}

	// ログ出力
	logging.logout("現在の盤面は以下の通りです.");
	state->logout(logging);

	LOGOUT(LOGLEVEL_TRACE, "========== Pv_mcts_action::run()終了. ==========");

	// リターン
	return 0;
}

int Pv_mcts_action::pv_mcts_scores(State* state, std::vector<Score>* scores)
{
	int ret;

	LOGOUT(LOGLEVEL_TRACE, "pv_mcts_scores()開始.");

	Node* root_node;
	
	try {
		root_node = new Node(model, state, 0.0, isBreadthFirst);
	}
	catch (...) {
		return -1;
	}

	try {
		for (int i = 0; i < numIterations; i++) {
			LOGOUT(LOGLEVEL_TRACE, "===== ★%d回目の評価を開始.★=====", i + 1);

			float value;
			ret = root_node->evaluate(&value);
			if (ret < 0) throw -2;
		}

		//scores = nodes_to_scores(root_node.child_nodes)
		LOGOUT(LOGLEVEL_TRACE, "各合法手の回数のリスト化を開始.");
		ret = root_node->nodes_to_scores(scores);
		if (ret < 0) throw -3;
	}
	catch (int ret) {
		delete root_node;
		return ret;
	}

	// root_nodeと子ノードのメモリ解放
	delete root_node;

	// 合法手有無のチェック
	// 合法手が無い場合はscoresリストのサイズは0となる
	if (scores->size() > 0) {
		//	if temperature == 0:
		LOGOUT(LOGLEVEL_TRACE, "温度を考慮しつつ各合法手の選択確率を設定。温度=%.1f.", temperature);
		if (temperature <= DBL_EPSILON) {
			LOGOUT(LOGLEVEL_TRACE, "温度=0なので、最大スコアの合法手の選択確率=1とします.", temperature);

			// nが最大となるノードを見つける
			int max_n = INT_MIN;
			bool isFound = false;
			size_t max_i = 0;

			//action = np.argmax(scores)
			//	scores = np.zeros(len(scores))
			//	scores[action] = 1
			for (size_t i = 0; i < scores->size(); i++) {
				// この時点でとりあえずすべての要素のprobabilityに0をセットしておく
				scores->at(i).probability = 0.0;

				if (scores->at(i).n >= max_n || isFound == false) {
					max_i = i;
					max_n = scores->at(i).n;
					isFound = true;
				}
			}

			// 最大となったノードのprobabilityを1にセット
			if (isFound == true) {
				scores->at(max_i).probability = 1.0;
			}
			else {
				// この条件が満たされることは無いが念のため
				return -4;
			}
		}
		else {
			LOGOUT(LOGLEVEL_TRACE, "温度≠0なので、ボルツマン分布に従って各合法手の選択確率をセットします.", temperature);

			//scores = boltzman(scores, temperature)
			ret = bolzman(scores, temperature);
			if (ret < 0) {
				return -5;
			}
		}
	}

	LOGOUT(LOGLEVEL_TRACE, "pv_mcts_scores()終了.");

	//	return scores
	return 0;
}

int Pv_mcts_action::bolzman(std::vector<Score>* scores, Temperature temperature)
{
	LOGOUT(LOGLEVEL_TRACE, "bolzman()開始. temperature=%.6f", temperature);

	double sum = 0.0;

	LOGOUT(LOGLEVEL_TRACE, "bolzman()開始.");
	LOGOUT(LOGLEVEL_TRACE, "----- 回数→ボルツマン分布値変換 -----");
	//xs = [x * *(1 / temperature) for x in xs]
	for (size_t i = 0; i < scores->size(); i++) {
		scores->at(i).probability = pow((double)scores->at(i).n, 1.0 / temperature);
		LOGOUT(LOGLEVEL_INFO, "%d: (x =%d, y = %d) %.6f => %.6f", i + 1, scores->at(i).x, scores->at(i).y, (double)scores->at(i).n, scores->at(i).probability);

		sum += scores->at(i).probability;
	}

	//return[x / sum(xs) for x in xs]
	LOGOUT(LOGLEVEL_TRACE, "----- ボルツマン分布値を合計1の値に正規化 -----");
	for (size_t i = 0; i < scores->size(); i++) {
		scores->at(i).probability = scores->at(i).probability / sum;
		LOGOUT(LOGLEVEL_TRACE, "%d: (x =%d, y = %d) => %.6f", i + 1, scores->at(i).x, scores->at(i).y, scores->at(i).probability);
	}

	LOGOUT(LOGLEVEL_TRACE, "bolzman()終了.");
	return 0;
}

//
//	関数名: ranom_choice
//	概要: 指定したスコアに比例した確率分布に従って次の一手を選択する
//	
//	入力:
//		state	現在のステート(盤面・次の石の色)
//		scores	選択する確率分布
//
//	出力:
//		action	選択した次の手
//
//	実装方法:
//		各合法手ごとに乱数を生成し、その値にスコア値(=確率分布)を乗じる。その乗じた結果の中で最大の合法手を選択する。
// 
int Pv_mcts_action::ranom_choice(State *state, std::vector<Score> scores, Action* action)
{
	LOGOUT(LOGLEVEL_TRACE, "ranom_choice()開始.");

	// scoreの内容をコピーする
	std::vector<Score> scoresSorted = scores;
	
	// scoresのprobabilityの大きい順にソートする
	// 別にソートしなくてもprobability値の確率でその手が選択されるから、この処理は必須でないことに注意
	std::sort(scoresSorted.begin(), scoresSorted.end(), compare);
	
	// 0～1までの乱数を生成する
	double randomValue = (double)rand() / (double)RAND_MAX;

	LOGOUT(LOGLEVEL_TRACE, "乱数値 = %.6f", randomValue);

	// 確率分布の値を先頭から加算し乱数値を超えた手前の値を選択する
	double sum = 0.0;
	size_t x = 0, y = 0;

	for (size_t i = 0; i < scoresSorted.size(); i++) {
		sum += scoresSorted[i].probability;

		// 合計値が乱数値以上となった場合はその値を選択
		// 確率分布の合計値は必ず最終的に乱数値の最大値の1.0となるので、必ず以下のif文が実行される
		if (sum >= randomValue - DBL_EPSILON) {
			x = scoresSorted[i].x;
			y = scoresSorted[i].y;

			LOGOUT(LOGLEVEL_TRACE, "%d番目の手を選択。", i + 1);

			*action = (int)x * 10 + (int)y;

			LOGOUT(LOGLEVEL_TRACE, "ranom_choice()終了. 打ち手はx = %d, y = %d です.", x, y);
			return 0;
		}
	}

	// 確率分布の合計値が1.0未満で本来はあり得ない。この場合はエラーとして返す。
	LOGOUT(LOGLEVEL_TRACE, "[ERROR] 入力された確率分布の積分値が1より小さいです。積分値 = %f。", sum);
	LOGOUT(LOGLEVEL_TRACE, "ranom_choice()終了.");
	return -1;
}

bool compare(Score a, Score b)
{
	return(a.probability > b.probability);
}