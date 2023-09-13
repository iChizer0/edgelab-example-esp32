#pragma once

#include <atomic>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "frontend/definations.hpp"
#include "frontend/static_resourse.hpp"
#include "frontend/utility.hpp"

namespace frontend::callback {

using namespace frontend::utility;
using namespace frontend::static_resource;

void get_available_models(const std::string& cmd) {
    auto& models_info = models->get_all_model_info();
    auto  os          = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK) << ", \"data\": [";
    DELIM_RESET;
    for (const auto& i : models_info) {
        DELIM_PRINT(os);
        os << model_info_2_json(i);
    }
    os << "]}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void set_model(const std::string& cmd, uint8_t model_id, InferenceEngine* engine, uint8_t& current_model_id) {
    auto        os         = std::ostringstream(std::ios_base::ate);
    const auto& model_info = models->get_model_info(model_id);
    auto        ret        = model_info.id ? EL_OK : EL_EINVAL;

    if (ret != EL_OK) [[unlikely]]
        goto ModelReply;

    // TODO: move heap_caps_malloc to port/el_memory or el_system
    static auto* tensor_arena = heap_caps_malloc(TENSOR_ARENA_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    memset(tensor_arena, 0, TENSOR_ARENA_SIZE);

    ret = engine->init(tensor_arena, TENSOR_ARENA_SIZE);
    if (ret != EL_OK) [[unlikely]]
        goto ModelError;

    ret = engine->load_model(model_info.addr_memory, model_info.size);
    if (ret != EL_OK) [[unlikely]]
        goto ModelError;

    if (current_model_id != model_id) {
        current_model_id = model_id;
        if (is_ready.load()) [[likely]]
            *storage << el_make_storage_kv("current_model_id", current_model_id);
    }

    goto ModelReply;

ModelError:
    current_model_id = 0;

ModelReply:
    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(ret)
       << ", \"data\": {\"model\": " << model_info_2_json(model_info) << "}}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void get_model_info(const std::string& cmd, const InferenceEngine* engine, uint8_t current_model_id) {
    auto        os         = std::ostringstream(std::ios_base::ate);
    const auto& model_info = models->get_model_info(current_model_id);
    auto        ret        = model_info.id ? EL_OK : EL_EINVAL;

    if (ret != EL_OK) [[unlikely]]
        goto ModelInfoReply;

ModelInfoReply:
    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(ret)
       << ", \"data\": " << model_info_2_json(model_info) << "}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

}  // namespace frontend::callback
