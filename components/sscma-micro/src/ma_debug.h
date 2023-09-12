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

#ifndef _MA_DEBUG_H_
#define _MA_DEBUG_H_

#include "ma_common.h"
#include "ma_misc.h"

#if CONFIG_MA_DEBUG

    #if CONFIG_MA_DEBUG_COLOR
        #define MA_DEBUG_COLOR_RED "\033[31m"
        #define MA_DEBUG_COLOR_GREEN "\033[32m"
        #define MA_DEBUG_COLOR_YELLOW "\033[33m"
        #define MA_DEBUG_COLOR_BLUE "\033[34m"
        #define MA_DEBUG_COLOR_MAGENTA "\033[35m"
        #define MA_DEBUG_COLOR_CYAN "\033[36m"
        #define MA_DEBUG_COLOR_RESET "\033[0m"
    #else
        #define MA_DEBUG_COLOR_RED
        #define MA_DEBUG_COLOR_GREEN
        #define MA_DEBUG_COLOR_YELLOW
        #define MA_DEBUG_COLOR_BLUE
        #define MA_DEBUG_COLOR_MAGENTA
        #define MA_DEBUG_COLOR_CYAN
        #define MA_DEBUG_COLOR_RESET
    #endif

    #if CONFIG_MA_DEBUG_MORE_INFO
        #define MA_DEBUG_MORE_INFO() ma_printf("(%llu) %s: ", ma_get_time_ms(), __func__);
    #else
        #define MA_DEBUG_MORE_INFO()
    #endif

    #if CONFIG_MA_DEBUG >= 1
        #define MA_ELOG(...)                            \
            do {                                        \
                ma_printf(MA_DEBUG_COLOR_RED "E ");     \
                MA_DEBUG_MORE_INFO();                   \
                ma_printf(__VA_ARGS__);                 \
                ma_printf(MA_DEBUG_COLOR_RESET "\r\n"); \
            } while (0)
    #else
        #define MA_ELOG(...)
    #endif

    #if CONFIG_MA_DEBUG >= 2
        #define MA_LOGW(...)                            \
            do {                                        \
                ma_printf(MA_DEBUG_COLOR_YELLOW "W ");  \
                MA_DEBUG_MORE_INFO();                   \
                ma_printf(__VA_ARGS__);                 \
                ma_printf(MA_DEBUG_COLOR_RESET "\r\n"); \
            } while (0)
    #else
        #define MA_LOGW(...)
    #endif
    #if CONFIG_MA_DEBUG >= 3
        #define MA_LOGI(...)            \
            do {                        \
                ma_printf("I ");        \
                MA_DEBUG_MORE_INFO();   \
                ma_printf(__VA_ARGS__); \
                ma_printf("\r\n");      \
            } while (0)
    #else
        #define MA_LOGI(...)
    #endif
    #if CONFIG_MA_DEBUG >= 4
        #define LOG_D(...)                              \
            do {                                        \
                ma_printf(MA_DEBUG_COLOR_GREEN "V ");   \
                MA_DEBUG_MORE_INFO();                   \
                ma_printf(__VA_ARGS__);                 \
                ma_printf(MA_DEBUG_COLOR_RESET "\r\n"); \
            } while (0)
    #else
        #define LOG_D(...)
    #endif
#else
    #define MA_ELOG(...)
    #define MA_LOGW(...)
    #define MA_LOGI(...)
    #define LOG_D(...)
#endif

#if CONFIG_MA_ASSERT
    #define MA_ASSERT(expr)                                    \
        do {                                                   \
            if (!(expr)) {                                     \
                ma_printf(MA_DEBUG_COLOR_RED "[ASSERT]");      \
                MA_DEBUG_MORE_INFO();                          \
                ma_printf("Failed assertion `%s'\r\n", #expr); \
                ma_printf(MA_DEBUG_COLOR_RESET);               \
                while (1) {                                    \
                }                                              \
            }                                                  \
        } while (0)
#else
    #define MA_ASSERT(expr)
#endif

#endif
