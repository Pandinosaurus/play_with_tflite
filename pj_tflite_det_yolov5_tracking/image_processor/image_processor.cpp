/* Copyright 2021 iwatake2222

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
/*** Include ***/
/* for general */
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <memory>

/* for OpenCV */
#include <opencv2/opencv.hpp>

/* for My modules */
#include "common_helper.h"
#include "bounding_box.h"
#include "detection_engine.h"
#include "tracker.h"
#include "image_processor.h"

/*** Macro ***/
#define TAG "ImageProcessor"
#define PRINT(...)   COMMON_HELPER_PRINT(TAG, __VA_ARGS__)
#define PRINT_E(...) COMMON_HELPER_PRINT_E(TAG, __VA_ARGS__)

/*** Global variable ***/
std::unique_ptr<DetectionEngine> s_engine;
Tracker s_tracker;

/*** Function ***/
static cv::Scalar CreateCvColor(int32_t b, int32_t g, int32_t r) {
#ifdef CV_COLOR_IS_RGB
    return cv::Scalar(r, g, b);
#else
    return cv::Scalar(b, g, r);
#endif
}


int32_t ImageProcessor::Initialize(const ImageProcessor::InputParam* input_param)
{
    if (s_engine) {
        PRINT_E("Already initialized\n");
        return -1;
    }

    s_engine.reset(new DetectionEngine());
    if (s_engine->Initialize(input_param->work_dir, input_param->num_threads) != DetectionEngine::kRetOk) {
        s_engine->Finalize();
        s_engine.reset();
        return -1;
    }
    return 0;
}

int32_t ImageProcessor::Finalize(void)
{
    if (!s_engine) {
        PRINT_E("Not initialized\n");
        return -1;
    }

    if (s_engine->Finalize() != DetectionEngine::kRetOk) {
        return -1;
    }

    return 0;
}


int32_t ImageProcessor::Command(int32_t cmd)
{
    if (!s_engine) {
        PRINT_E("Not initialized\n");
        return -1;
    }

    switch (cmd) {
    case 0:
    default:
        PRINT_E("command(%d) is not supported\n", cmd);
        return -1;
    }
}


int32_t ImageProcessor::Process(cv::Mat* mat, ImageProcessor::OutputParam* output_param)
{
    if (!s_engine) {
        PRINT_E("Not initialized\n");
        return -1;
    }

    cv::Mat& original_mat = *mat;
    DetectionEngine::Result result;
    if (s_engine->Process(original_mat, result) != DetectionEngine::kRetOk) {
        return -1;
    }

    s_tracker.Update(result.bbox_list);
    auto& track_list = s_tracker.GetTrackList();
    for (auto& track : track_list) {
        if (track.cnt_detected_ < 3) continue;
        auto& object = track.GetLatestBoundingBox();
        cv::rectangle(original_mat, cv::Rect(object.x, object.y, object.w, object.h), cv::Scalar(255, 255, 0), 3);
        cv::putText(original_mat, object.label, cv::Point(object.x, object.y + 10), cv::FONT_HERSHEY_PLAIN, 1, CreateCvColor(0, 0, 0), 3);
        cv::putText(original_mat, object.label, cv::Point(object.x, object.y + 10), cv::FONT_HERSHEY_PLAIN, 1, CreateCvColor(0, 255, 0), 1);


        auto& track_history = track.GetTrackHistory();
        for (int32_t i = 1; i < track_history.size(); i++) {
            cv::Point p0(track_history[i].bbox.x + track_history[i].bbox.w / 2, track_history[i].bbox.y + track_history[i].bbox.h);
            cv::Point p1(track_history[i - 1].bbox.x + track_history[i - 1].bbox.w / 2, track_history[i - 1].bbox.y + track_history[i - 1].bbox.h);
            cv::line(original_mat, p0, p1, CreateCvColor(255, 0, 0));
        }
    }

    /* Draw the result */
    //for (const auto& object : result.bbox_list) {
    //    cv::rectangle(original_mat, cv::Rect(object.x, object.y, object.w, object.h), cv::Scalar(255, 255, 0), 3);
    //    cv::putText(original_mat, object.label, cv::Point(object.x, object.y + 10), cv::FONT_HERSHEY_PLAIN, 1, CreateCvColor(0, 0, 0), 3);
    //    cv::putText(original_mat, object.label, cv::Point(object.x, object.y + 10), cv::FONT_HERSHEY_PLAIN, 1, CreateCvColor(0, 255, 0), 1);
    //}


    ///* Return the results */
    //int32_t object_num = 0;
    //for (const auto& object : result.bbox_list) {
    //    output_param->object_list[object_num].class_id = object.class_id;
    //    snprintf(output_param->object_list[object_num].label, sizeof(output_param->object_list[object_num].label), "%s", object.label.c_str());
    //    output_param->object_list[object_num].score = object.score;
    //    output_param->object_list[object_num].x = object.x;
    //    output_param->object_list[object_num].y = object.y;
    //    output_param->object_list[object_num].width = object.w;
    //    output_param->object_list[object_num].height = object.h;
    //    object_num++;
    //    if (object_num >= NUM_MAX_RESULT) break;
    //}
    //output_param->object_num = object_num;
    output_param->time_pre_process = result.time_pre_process;
    output_param->time_inference = result.time_inference;
    output_param->time_post_process = result.time_post_process;

    return 0;
}

