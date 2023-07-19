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

#ifndef _EL_ALGORITHM_FOMO_H_
#define _EL_ALGORITHM_FOMO_H_

#include <forward_list>

#include "el_algorithm_base.hpp"
#include "el_cv.h"
#include "el_types.h"

namespace edgelab {
namespace algorithm {

template <typename InferenceEngine, typename InputType, typename OutputType>
class Fomo : public edgelab::algorithm::base::Algorithm<InferenceEngine, InputType, OutputType> {
   private:
    std::forward_list<OutputType> _results;
    el_shape_t                    _input_shape;
    el_shape_t                    _output_shape;
    el_quant_param_t              _input_quant;
    el_quant_param_t              _output_quant;
    float                         _w_scale;
    float                         _h_scale;

   public:
    InputType _img;

   protected:
    EL_STA preprocess() override;
    EL_STA postprocess() override;

   public:
    Fomo(InferenceEngine& engine, int8_t score_threshold = 80);
    ~Fomo();

    EL_STA init() override;
    EL_STA deinit() override;

    const std::forward_list<OutputType>& get_results() override;
};

template <typename InferenceEngine, typename InputType, typename OutputType>
Fomo<InferenceEngine, InputType, OutputType>::Fomo(InferenceEngine& engine, int8_t score_threshold)
    : edgelab::algorithm::base::Algorithm<InferenceEngine, InputType, OutputType>(engine, score_threshold) {
    _w_scale = 1.0f;
    _h_scale = 1.0f;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
Fomo<InferenceEngine, InputType, OutputType>::~Fomo() {
    deinit();
}

template <typename InferenceEngine, typename InputType, typename OutputType>
EL_STA Fomo<InferenceEngine, InputType, OutputType>::init() {
    _results.clear();
    _input_shape  = this->__p_engine->get_input_shape(0);
    _output_shape = this->__p_engine->get_output_shape(0);
    _input_quant  = this->__p_engine->get_input_quant_param(0);
    _output_quant = this->__p_engine->get_output_quant_param(0);

    _img.data   = static_cast<uint8_t*>(this->__p_engine->get_input(0));
    _img.width  = _input_shape.dims[1];
    _img.height = _input_shape.dims[2];
    _img.size   = _img.width * _img.height * _input_shape.dims[3];

    if (_input_shape.dims[3] == 3) {
        _img.format = EL_PIXEL_FORMAT_RGB888;
    } else if (_input_shape.dims[3] == 1) {
        _img.format = EL_PIXEL_FORMAT_GRAYSCALE;
    } else {
        return EL_EINVAL;
    }

    return EL_OK;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
EL_STA Fomo<InferenceEngine, InputType, OutputType>::deinit() {
    _results.clear();
    return EL_OK;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
EL_STA Fomo<InferenceEngine, InputType, OutputType>::preprocess() {
    EL_STA     ret   = EL_OK;
    InputType* i_img = this->__p_input;

    // convert image
    el_printf("%d, %d\n", _img.width, _img.height);
    ret = rgb_to_rgb(i_img, &_img);

    if (ret != EL_OK) {
        return ret;
    }

    for (size_t i = 0; i < _img.size; i++) {
        _img.data[i] -= 128;
    }

    _w_scale = static_cast<float>(i_img->width) / static_cast<float>(_img.width);
    _h_scale = static_cast<float>(i_img->height) / static_cast<float>(_img.height);

    return EL_OK;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
EL_STA Fomo<InferenceEngine, InputType, OutputType>::postprocess() {
    _results.clear();

    // get output
    int8_t*  data       = static_cast<int8_t*>(this->__p_engine->get_output(0));
    uint32_t width      = this->__p_input->width;
    uint32_t height     = this->__p_input->height;
    float    scale      = _output_quant.scale;
    int32_t  zero_point = _output_quant.zero_point;
    auto     pred_h     = _output_shape.dims[1];
    auto     pred_w     = _output_shape.dims[2];
    auto     pred_t     = _output_shape.dims[3];

    uint16_t bw = width / pred_w;
    uint16_t bh = height / pred_h;

    for (int i = 0; i < pred_h; ++i) {
        for (int j = 0; j < pred_w; ++j) {
            uint8_t max_conf   = 0;
            uint8_t max_target = 0;
            for (int t = 0; t < pred_t; ++t) {
                uint8_t conf = (data[i * pred_w * pred_t + j * pred_t + t] - zero_point) * scale * 100;
                if (conf > max_conf) {
                    max_conf   = conf;
                    max_target = t;
                }
            }
            if (max_conf > this->__score_threshold && max_target != 0) {
                _results.emplace_front(OutputType{.x      = static_cast<uint16_t>(j * bw + bw / 2),
                                                  .y      = static_cast<uint16_t>(i * bh + bh / 2),
                                                  .w      = bw,
                                                  .h      = bh,
                                                  .score  = max_conf,
                                                  .target = max_target});
            }
        }
    }

    for (auto& box : _results) {
        LOG_D("x: %d, y: %d, w: %d, h: %d, score: %d, target: %d", box.x, box.y, box.w, box.h, box.score, box.target);
    }
    _results.sort([](const OutputType& a, const OutputType& b) { return a.x < b.x; });

    return EL_OK;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
const std::forward_list<OutputType>& Fomo<InferenceEngine, InputType, OutputType>::get_results() {
    return _results;
}

}  // namespace algorithm
}  // namespace edgelab

#endif
