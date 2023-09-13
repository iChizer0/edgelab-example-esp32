#pragma once

#include <atomic>

#include "core/edgelab.h"
#include "porting/espressif/el_device_esp.h"

namespace frontend::static_resource {

static auto* device = DeviceEsp::get_device();
static auto* serial = device->get_serial();

static auto* repl     = ReplDelegate::get_delegate();
static auto* instance = repl->get_server_handler();
static auto* executor = repl->get_executor_handler();

static auto* data_delegate = DataDelegate::get_delegate();
static auto* models        = data_delegate->get_models_handler();
static auto* storage       = data_delegate->get_storage_handler();

static auto* engine = []() {
    static auto engine{InferenceEngine()};
    return &engine;
}();

auto* algorithm_delegate = AlgorithmDelegate::get_delegate();

static std::atomic<bool> is_ready{false};

}  // namespace frontend::static_resource
