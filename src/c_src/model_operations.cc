#include <iostream>
#include <memory>
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"

#include "include/model_operations.h"

int main()
{
    const char* model_path = MODEL_FILE_PATH;
    std::unique_ptr<tflite::FlatBufferModel> lfd_net_model = tfllite::FlatBufferModel::BuildFromFile(model_path);

    if(lfd_net_model == nullptr){
        std::cerr << "[Error] Failed to load LFD-NET model: " << model_path << std::end;
        return -1;
    }



}
