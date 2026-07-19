#ifndef MODEL_OPERATIONS_H
#define MODEL_OPERATIONS_H

#include <memory>
#include <string>
#include <opencv2/opencv.hpp>
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"

#define MODEL_FILE_PATH "/data/source/model/lfd_outdoor_256.tflite"

class RunModel{
public:
    RunModel() = default;
    bool LoadModel(const std::string& lfd_net_model_path);
    bool RunInference(const cv::Mat& preprocessed_img);

    int GetWidth() const { return model_width; }
    int GetHeight() const { return model_height; }

private:
    std::unique_ptr<tflite::FlatBufferModel> model_;
    std::unique_ptr<tflite::interpreter> interpreter_;
    tflite::ops::builtin::BuiltInOpResolver resolver_;

    int model_width_ = 0;
    int model_height_ = 0;
    int model_channels_ = 0;
};

#endif
