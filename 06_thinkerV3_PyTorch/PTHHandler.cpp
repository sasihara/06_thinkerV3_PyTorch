#include <torch/script.h>
#include <torch/torch.h>
#include <iostream>
// ... その他の include
#include <iostream>
#include <vector>
#include "PTHHandler.hpp"
#include "externalThinkerMessages.hpp"
#include "thinkV1.hpp"
#include "logging.h"

// Windows特有のERRORマクロとの衝突を避ける
#ifdef ERROR
#undef ERROR
#endif

extern Logging logging;


//int load_model(Model* model, RunningMode _runningMode)
//{
//    try {
//        // TorchScriptモデルの読み込み
//        model->module = std::make_shared<torch::jit::script::Module>(
//            torch::jit::load("04_OthelloDeepModel/best_cpu.pt")
//        );
//        model->module->eval(); // 推論モード
//    }
//    catch (const c10::Error& e) {
//        std::cerr << "Error loading the model\n";
//        return -1;
//    }
//    return 0;
//}
int load_model(Model* model, RunningMode _runningMode) {
    if (model == nullptr) return -1;

    torch::Device device(torch::kCPU);
    std::string model_name = "";
    int error_code = 0;

    // 1. モードに応じたデバイスとファイル名の決定
    switch (_runningMode) {
    case RunningMode::RUNNINGMODE_CPU:
        device = torch::Device(torch::kCPU);
        model_name = "best_cpu.pt";
        error_code = -2;
        break;

    case RunningMode::RUNNINGMODE_GPU:
        device = torch::Device(torch::kCUDA);
        model_name = "best_gpu.pt";
        error_code = -3;
        break;

    case RunningMode::RUNNINGMODE_AUTO:
    default:
        if (torch::cuda::is_available()) {
            device = torch::Device(torch::kCUDA);
            model_name = "best_gpu.pt";
        }
        else {
            device = torch::Device(torch::kCPU);
            model_name = "best_cpu.pt";
        }
        error_code = -4;
        break;
    }

    // 2. ロード処理
    std::string model_path = "04_OthelloDeepModel/" + model_name;

    try {
        // ロード実行（指定デバイスへ転送）
        auto loaded_module = torch::jit::load(model_path, device);
        loaded_module.eval();

        // 構造体に格納
        model->module = std::make_shared<torch::jit::script::Module>(std::move(loaded_module));

        std::cout << "[SUCCESS] モデルロード完了: " << model_path
            << " (" << (device.is_cuda() ? "CUDA" : "CPU") << ")" << std::endl;
        return 0;

    }
    catch (const c10::Error& e) {
        std::cerr << "[ERROR] ロード失敗: " << model_path << std::endl;
        std::cerr << "詳細: " << e.msg() << std::endl;
        return error_code; // 各モード指定の負数値を返す
    }
}

int free_model(Model* model)
{
    model->module.reset();
    return 0;
}

int predict(Model* model, State _state, float* _policies, float* _value)
{
    int ret = 0;

    LOGOUT(LOGLEVEL_TRACE, "predict() start.");
    logging.logprintf("プレイヤーの石の色: %s\n", _state.currentPlayer == DISKCOLORS::COLOR_BLACK ? "●" : _state.currentPlayer == DISKCOLORS::COLOR_WHITE ? "○" : "");
    logging.logprintf("対戦相手の石の色: %s\n", _state.opponent == DISKCOLORS::COLOR_BLACK ? "●" : _state.opponent == DISKCOLORS::COLOR_WHITE ? "○" : "");
    ret = logoutBoard(logging, _state.board);

    // 1. 入力データの作成 (PyTorchは Channel First: [1, 2, 8, 8])
    // Python版に合わせ [Player面, Enemy面] の順で格納
    std::vector<float> input_tensor_values(1 * 2 * 8 * 8);

    for (int i = 0; i < 64; i++) {
        // Channel 0: Current Player
        input_tensor_values[i] = (_state.board[i] == _state.currentPlayer) ? 1.0f : 0.0f;
        // Channel 1: Opponent
        input_tensor_values[i + 64] = (_state.board[i] == _state.opponent) ? 1.0f : 0.0f;
    }

    // 2. std::vector から torch::Tensor への変換
    torch::Tensor input_tensor = torch::from_blob(
        input_tensor_values.data(), { 1, 2, 8, 8 }, torch::kFloat32
    ).clone(); // データの所有権を確保

    try {
        // 3. 推論実行
        // モデルは2つの出力を返す想定: (policy, value)
        auto outputs = model->module->forward({ input_tensor }).toTuple();

        torch::Tensor p_tensor = outputs->elements()[0].toTensor();
        torch::Tensor v_tensor = outputs->elements()[1].toTensor();

        // 4. 結果の取り出し
        // Policy (65次元)
        auto p_data = p_tensor.accessor<float, 2>();
        for (int i = 0; i < DN_OUTPUT_SIZE; i++) {
            _policies[i] = p_data[0][i];
        }

        // Value (1次元)
        auto v_data = v_tensor.accessor<float, 2>();
        *_value = v_data[0][0];

    }
    catch (const c10::Error& e) {
        std::cerr << "Inference error: " << e.msg() << std::endl;
        return -1;
    }

    return 0;
}