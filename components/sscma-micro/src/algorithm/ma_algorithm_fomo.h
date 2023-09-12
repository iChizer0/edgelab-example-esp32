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

#ifndef _MA_ALGORITHM_FOMO_H_
#define _MA_ALGORITHM_FOMO_H_

#include <atomic>
#include <cstdbool>
#include <cstdint>
#include <forward_list>

#include "ma_algorithm_base.h"
#include "ma_ctypes.h"

namespace sscma::algorithm {

namespace types {

// we're not using inheritance since it not standard layout
struct ma_algorithm_fomo_config_t {
    static constexpr ma_algorithm_info_t info{
      .type = MA_ALGO_TYPE_FOMO, .categroy = MA_ALGO_CAT_DET, .input_from = MA_SENSOR_TYPE_CAM};
    uint8_t score_threshold = 80;
};

}  // namespace types

using namespace sscma::algorithm::types;

class AlgorithmFOMO : public AlgorithmBase {
   public:
    using ImageType  = ma_img_t;
    using BoxType    = ma_box_t;
    using ConfigType = ma_algorithm_fomo_config_t;
    using ScoreType  = decltype(ma_algorithm_fomo_config_t::score_threshold);

    static InfoType algorithm_info;

    AlgorithmFOMO(EngineType* engine, ScoreType score_threshold = 80);
    AlgorithmFOMO(EngineType* engine, const ConfigType& config);
    ~AlgorithmFOMO();

    static bool is_model_valid(const EngineType* engine);

    ma_err_code_t                     run(ImageType* input);
    const std::forward_list<BoxType>& get_results() const;

    void      set_score_threshold(ScoreType threshold);
    ScoreType get_score_threshold() const;

    void       set_algorithm_config(const ConfigType& config);
    ConfigType get_algorithm_config() const;

   protected:
    inline void init();

    ma_err_code_t preprocess() override;
    ma_err_code_t postprocess() override;

   private:
    ImageType _input_img;
    float     _w_scale;
    float     _h_scale;

    std::atomic<ScoreType> _score_threshold;

    std::forward_list<BoxType> _results;
};

}  // namespace sscma::algorithm

#endif
