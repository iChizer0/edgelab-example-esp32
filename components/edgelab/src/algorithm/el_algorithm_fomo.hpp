/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Seeed Technology Co.,Ltd
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
#include <type_traits>

#include "el_algorithm_base.hpp"
#include "el_cv.h"
#include "el_types.h"

namespace edgelab {
namespace algorithm {

template <typename InferenceEngine, typename ImageType, typename BoxType>
class Fomo : public edgelab::algorithm::base::Algorithm<InferenceEngine, ImageType, BoxType> {
    using ScoreType = edgelab::algorithm::base::Algorithm<InferenceEngine, ImageType, BoxType>::ScoreType;

   private:
    std::forward_list<BoxType> _results;
    el_shape_t                 _input_shape;
    el_shape_t                 _output_shape;
    el_quant_param_t           _input_quant;
    el_quant_param_t           _output_quant;
    float                      _w_scale;
    float                      _h_scale;

   public:
    ImageType _img;

   protected:
    EL_STA preprocess() override;
    EL_STA postprocess() override;

   public:
    Fomo(InferenceEngine& engine, ScoreType score_threshold = 80);
    ~Fomo();

    EL_STA init() override;
    EL_STA deinit() override;

    const std::forward_list<BoxType>& get_results() override;
};

template <typename InferenceEngine, typename ImageType, typename BoxType>
Fomo<InferenceEngine, ImageType, BoxType>::Fomo(InferenceEngine& engine, ScoreType score_threshold)
    : edgelab::algorithm::base::Algorithm<InferenceEngine, ImageType, BoxType>(engine, score_threshold) {
    _w_scale = 1.0f;
    _h_scale = 1.0f;
}

template <typename InferenceEngine, typename ImageType, typename BoxType>
Fomo<InferenceEngine, ImageType, BoxType>::~Fomo() {
    deinit();
}

template <typename InferenceEngine, typename ImageType, typename BoxType>
EL_STA Fomo<InferenceEngine, ImageType, BoxType>::init() {
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

template <typename InferenceEngine, typename ImageType, typename BoxType>
EL_STA Fomo<InferenceEngine, ImageType, BoxType>::deinit() {
    _results.clear();
    return EL_OK;
}

template <typename InferenceEngine, typename ImageType, typename BoxType>
EL_STA Fomo<InferenceEngine, ImageType, BoxType>::preprocess() {
    EL_STA ret{EL_OK};
    auto*  i_img{this->__p_input};

    // convert image
    el_printf("%d, %d\n", _img.width, _img.height);
    ret = rgb_to_rgb(i_img, &_img);

    if (ret != EL_OK) {
        return ret;
    }

    for (decltype(ImageType::size) i{0}; i < _img.size; ++i) {
        _img.data[i] -= 128;
    }

    _w_scale = static_cast<float>(i_img->width) / static_cast<float>(_img.width);
    _h_scale = static_cast<float>(i_img->height) / static_cast<float>(_img.height);

    return EL_OK;
}

template <typename InferenceEngine, typename ImageType, typename BoxType>
EL_STA Fomo<InferenceEngine, ImageType, BoxType>::postprocess() {
    _results.clear();

    // get output
    auto*   data{static_cast<int8_t*>(this->__p_engine->get_output(0))};
    auto    width{this->__p_input->width};
    auto    height{this->__p_input->height};
    float   scale{_output_quant.scale};
    int32_t zero_point{_output_quant.zero_point};
    auto    pred_w{_output_shape.dims[2]};
    auto    pred_h{_output_shape.dims[1]};
    auto    pred_t{_output_shape.dims[3]};

    auto bw{static_cast<decltype(BoxType::w)>(width / pred_w)};
    auto bh{static_cast<decltype(BoxType::h)>(height / pred_h)};

    for (decltype(pred_h) i{0}; i < pred_h; ++i) {
        for (decltype(pred_w) j{0}; j < pred_w; ++j) {
            ScoreType                 max_score{0};
            decltype(BoxType::target) max_target{0};
            for (decltype(pred_t) t{0}; t < pred_t; ++t) {
                auto score{
                  static_cast<ScoreType>((data[i * pred_w * pred_t + j * pred_t + t] - zero_point) * 100 * scale)};
                if (score > max_score) {
                    max_score  = score;
                    max_target = t;
                }
            }
            if (max_score > this->__score_threshold && max_target != 0) {
                // only unsigned is supported for fast div by 2 (>> 1)
                static_assert(std::is_unsigned<decltype(bw)>::value && std::is_unsigned<decltype(bh)>::value);
                _results.emplace_front(BoxType{.x      = static_cast<decltype(BoxType::x)>(j * bw + (bw >> 1)),
                                               .y      = static_cast<decltype(BoxType::y)>(i * bh + (bh >> 1)),
                                               .w      = bw,
                                               .h      = bh,
                                               .score  = max_score,
                                               .target = max_target});
            }
        }
    }
    _results.sort([](const BoxType& a, const BoxType& b) { return a.x < b.x; });

#if CONFIG_EL_DEBUG >= 4
    for (const auto& box : _results) {
        LOG_D("\tbox (image space) -> cx_cy_w_h: [%d, %d, %d, %d] t: [%d] s: [%d]",
              box.x,
              box.y,
              box.w,
              box.h,
              box.score,
              box.target);
    }
#endif

    return EL_OK;
}

template <typename InferenceEngine, typename ImageType, typename BoxType>
const std::forward_list<BoxType>& Fomo<InferenceEngine, ImageType, BoxType>::get_results() {
    return _results;
}

}  // namespace algorithm
}  // namespace edgelab

#endif
