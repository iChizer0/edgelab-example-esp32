/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Hongtai Liu, nullptr (Seeed Technology Inc.)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _EL_ALGORITHM_YOLO_H_
#define _EL_ALGORITHM_YOLO_H_

#include <forward_list>

#include "el_algorithm_base.hpp"
#include "el_cv.h"
#include "el_nms.h"
#include "el_types.h"

namespace edgelab {
namespace algorithm {

template <typename InferenceEngine, typename InputType, typename OutputType>
class Yolo : public edgelab::algorithm::base::Algorithm<InferenceEngine, InputType, OutputType> {
   private:
    std::forward_list<OutputType> _results;
    el_shape_t                    _input_shape;
    el_shape_t                    _output_shape;
    el_quant_param_t              _input_quant;
    el_quant_param_t              _output_quant;
    float                         _w_scale;
    float                         _h_scale;
    uint8_t                       _nms_threshold;

   protected:
    EL_STA preprocess() override;
    EL_STA postprocess() override;

    enum {
        INDEX_X = 0,
        INDEX_Y = 1,
        INDEX_W = 2,
        INDEX_H = 3,
        INDEX_S = 4,
        INDEX_T = 5,
    };

   public:
    Yolo(InferenceEngine& engine, int8_t score_threshold = 40, int8_t nms_threshold = 45);
    ~Yolo();

    EL_STA init() override;
    EL_STA deinit() override;

    EL_STA  set_nms_threshold(uint8_t threshold);
    uint8_t get_nms_threshold() const;

    const std::forward_list<OutputType>& get_results() override;
};

template <typename InferenceEngine, typename InputType, typename OutputType>
Yolo<InferenceEngine, InputType, OutputType>::Yolo(InferenceEngine& engine,
                                                   int8_t           score_threshold,
                                                   int8_t           nms_threshold)
    : edgelab::algorithm::base::Algorithm<InferenceEngine, InputType, OutputType>(engine, score_threshold),
      _nms_threshold(nms_threshold) {
    _w_scale = 1.0f;
    _h_scale = 1.0f;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
Yolo<InferenceEngine, InputType, OutputType>::~Yolo() {
    deinit();
}

template <typename InferenceEngine, typename InputType, typename OutputType>
EL_STA Yolo<InferenceEngine, InputType, OutputType>::init() {
    _results.clear();
    _input_shape  = this->__p_engine->get_input_shape(0);
    _output_shape = this->__p_engine->get_output_shape(0);
    _input_quant  = this->__p_engine->get_input_quant_param(0);
    _output_quant = this->__p_engine->get_output_quant_param(0);
    return EL_OK;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
EL_STA Yolo<InferenceEngine, InputType, OutputType>::deinit() {
    _results.clear();
    return EL_OK;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
EL_STA Yolo<InferenceEngine, InputType, OutputType>::preprocess() {
    EL_STA     ret   = EL_OK;
    InputType  img   = InputType();
    InputType* i_img = this->__p_input;

    img.data   = static_cast<uint8_t*>(this->__p_engine->get_input(0));
    img.width  = _input_shape.dims[1];
    img.height = _input_shape.dims[2];
    img.size   = img.width * img.height * _input_shape.dims[3];
    if (_input_shape.dims[3] == 3) {
        img.format = EL_PIXEL_FORMAT_RGB888;
    } else if (_input_shape.dims[3] == 1) {
        img.format = EL_PIXEL_FORMAT_GRAYSCALE;
    } else {
        return EL_EINVAL;
    }

    // convert image
    ret = el_img_convert(i_img, &img);
    if (ret != EL_OK) {
        return ret;
    }

    for (int i = 0; i < img.size; i++) {
        img.data[i] -= 128;
    }

    _w_scale = static_cast<float>(i_img->width) / static_cast<float>(img.width);
    _h_scale = static_cast<float>(i_img->height) / static_cast<float>(img.height);

    return EL_OK;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
EL_STA Yolo<InferenceEngine, InputType, OutputType>::postprocess() {
    _results.clear();

    // get output
    int8_t*  data        = static_cast<int8_t*>(this->__p_engine->get_output(0));
    uint16_t width       = _input_shape.dims[1];
    uint16_t height      = _input_shape.dims[2];
    float    scale       = _output_quant.scale;
    int32_t  zero_point  = _output_quant.zero_point;
    uint32_t num_record  = _output_shape.dims[1];
    uint32_t num_element = _output_shape.dims[2];
    uint32_t num_class   = num_element - 5;

    bool rescale = scale < 0.1 ? true : false;

    // parse output
    for (int i = 0; i < num_record; i++) {
        size_t idx   = i * num_element;
        float  score = static_cast<float>(data[idx + INDEX_S] - zero_point) * scale;
        score        = rescale ? score * 100 : score;
        if (score > this->__score_threshold) {
            static OutputType box;
            box.score  = score;
            box.target = 0;
            int8_t max = -128;

            // get box target
            for (uint16_t j = 0; j < num_class; j++) {
                if (max < data[idx + INDEX_T + j]) {
                    max        = data[idx + INDEX_T + j];
                    box.target = j;
                }
            }
            // get box position
            float x = static_cast<float>(data[idx + INDEX_X] - zero_point) * scale;
            float y = static_cast<float>(data[idx + INDEX_Y] - zero_point) * scale;
            float w = static_cast<float>(data[idx + INDEX_W] - zero_point) * scale;
            float h = static_cast<float>(data[idx + INDEX_H] - zero_point) * scale;

            if (rescale) {
                box.x = EL_CLIP(static_cast<uint16_t>(x * width), 0, width);
                box.y = EL_CLIP(static_cast<uint16_t>(y * height), 0, height);
                box.w = EL_CLIP(static_cast<uint16_t>(w * width), 0, width);
                box.h = EL_CLIP(static_cast<uint16_t>(h * height), 0, height);
            } else {
                box.x = EL_CLIP(static_cast<uint16_t>(x), 0, width);
                box.y = EL_CLIP(static_cast<uint16_t>(y), 0, height);
                box.w = EL_CLIP(static_cast<uint16_t>(w), 0, width);
                box.h = EL_CLIP(static_cast<uint16_t>(h), 0, height);
            }

            box.w = box.x + box.w > width ? width - box.x : box.w;
            box.h = box.y + box.h > height ? height - box.y : box.h;

            _results.emplace_front(box);
        }
    }

    el_nms(_results, _nms_threshold, this->__score_threshold);

    // for (auto &box : _results) {
    //     LOG_D("x: %d, y: %d, w: %d, h: %d, score: %d, target: %d",
    //                box.x,
    //                box.y,
    //                box.w,
    //                box.h,
    //                box.score,
    //                box.target);
    // }
    _results.sort([](const OutputType& a, const OutputType& b) { return a.x < b.x; });

    for (auto& box : _results) {
        box.x = box.x * _w_scale;
        box.y = box.y * _h_scale;
        box.w = box.w * _w_scale;
        box.h = box.h * _h_scale;
    }

    return EL_OK;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
const std::forward_list<OutputType>& Yolo<InferenceEngine, InputType, OutputType>::get_results() {
    return _results;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
EL_STA Yolo<InferenceEngine, InputType, OutputType>::set_nms_threshold(uint8_t threshold) {
    _nms_threshold = threshold;
    return EL_OK;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
uint8_t Yolo<InferenceEngine, InputType, OutputType>::get_nms_threshold() const {
    return _nms_threshold;
}

}  // namespace algorithm
}  // namespace edgelab

#endif
