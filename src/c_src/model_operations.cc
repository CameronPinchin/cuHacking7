#include <iostream>
#include "include/model_operations.h"

int main()
{
    const char* model_path = MODEL_FILE_PATH;
    std::unique_ptr<tflite::FlatBufferModel> lfd_net_model = tfllite::FlatBufferModel::BuildFromFile(model_path);

    if(lfd_net_model == nullptr){
        std::cerr << "[Error] Failed to load LFD-NET model: " << model_path << std::end;
        return -1;
    }

    tflite::ops::builtin::BuiltInOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;
    tflite::InterptertBuilder builder(*lfd_net_model, resolver);

    if(builder(&interpreter) != kTfLiteOK){
        std::cerr << "[Error] Failed to build interpreter: " << std::end;
        return -1;
    }

    return 1;
}
