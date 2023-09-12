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

#ifndef _MA_COMMON_H_
#define _MA_COMMON_H_

#include "ma_compiler.h"
#include "ma_config_internal.h"
#include "ma_debug.h"
#include "ma_misc.h"
#include "ma_ctypes.h"

#define MA_VERSION                 "2023.09.11"
#define MA_VERSION_LENTH_MAX       32

#define MA_CONCAT(a, b)            a##b
#define MA_STRINGIFY(s)            #s

#define MA_UNUSED(x)               (void)(x)
#define MA_ARRAY_SIZE(x)           (sizeof(x) / sizeof((x)[0]))

#define MA_MAX(a, b)               ((a) > (b) ? (a) : (b))
#define MA_MIN(a, b)               ((a) < (b) ? (a) : (b))
#define MA_CLIP(x, a, b)           ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))

#define MA_ABS(a)                  ((a) < 0 ? -(a) : (a))
#define MA_ALIGN(x, a)             (((x) + ((a)-1)) & ~((a)-1))
#define MA_ALIGN_DOWN(x, a)        ((x) & ~((a)-1))
#define MA_ALIGN_UP(x, a)          (((x) + ((a)-1)) & ~((a)-1))
#define MA_IS_ALIGNED(x, a)        (((x) & ((typeof(x))(a)-1)) == 0)
#define MA_IS_ALIGNED_DOWN(x, a)   (((x) & ((typeof(x))(a)-1)) == 0)
#define MA_IS_ALIGNED_UP(x, a)     (((x) & ((typeof(x))(a)-1)) == 0)

#define MA_BIT(n)                  (1 << (n))
#define MA_BIT_MASK(n)             (MA_BIT(n) - 1)
#define MA_BIT_SET(x, n)           ((x) |= MA_BIT(n))
#define MA_BIT_CLR(x, n)           ((x) &= ~MA_BIT(n))
#define MA_BIT_GET(x, n)           (((x) >> (n)) & 1)
#define MA_BIT_SET_MASK(x, n, m)   ((x) |= ((m) << (n)))
#define MA_BIT_CLR_MASK(x, n, m)   ((x) &= ~((m) << (n)))
#define MA_BIT_GET_MASK(x, n, m)   (((x) >> (n)) & (m))
#define MA_BIT_SET_MASKED(x, n, m) ((x) |= ((m) & (MA_BIT_MASK(n) << (n))))
#define MA_BIT_CLR_MASKED(x, n, m) ((x) &= ~((m) & (MA_BIT_MASK(n) << (n))))
#define MA_BIT_GET_MASKED(x, n, m) (((x) >> (n)) & (m))

#endif
