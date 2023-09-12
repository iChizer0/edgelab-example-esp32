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

#ifndef _MA_BASE_ALGORITHM_H_
#define _MA_BASE_ALGORITHM_H_

#include <cstdint>

#include "engine/ma_engine_base.h"
#include "ma_ctypes.h"

namespace sscma::algorithm {

namespace types {

struct ma_algorithm_info_t {
    ma_algorithm_type_t type;
    ma_algorithm_cat_t  categroy;
    ma_sensor_type_t    input_from;
};

}  // namespace types

using namespace sscma::algorithm::types;

class AlgorithmBase {
   protected:
    using EngineType = sscma::engine::EngineBase;
    using InfoType   = ma_algorithm_info_t;

   public:
    AlgorithmBase(EngineType* engine, const InfoType& info);
    virtual ~AlgorithmBase();

    InfoType get_algorithm_info() const;

    uint32_t get_preprocess_time() const;
    uint32_t get_run_time() const;
    uint32_t get_postprocess_time() const;

   protected:
    ma_err_code_t underlying_run(void* input);

    virtual ma_err_code_t preprocess()  = 0;
    virtual ma_err_code_t postprocess() = 0;

    EngineType* __p_engine;

    void* __p_input;

    ma_shape_t __input_shape;
    ma_shape_t __output_shape;

    ma_quant_param_t __input_quant;
    ma_quant_param_t __output_quant;

   private:
    InfoType __algorithm_info;

    uint32_t __preprocess_time;   // ms
    uint32_t __run_time;          // ms
    uint32_t __postprocess_time;  // ms
};

}  // namespace sscma::algorithm

#endif
