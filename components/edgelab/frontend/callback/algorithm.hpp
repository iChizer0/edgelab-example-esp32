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

void get_available_algorithms(const std::string& cmd) {
    auto        os                    = std::ostringstream(std::ios_base::ate);
    const auto& registered_algorithms = algorithm_delegate->get_all_algorithm_info();

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK) << ", \"data\": [";
    DELIM_RESET;
    for (const auto& i : registered_algorithms) {
        DELIM_PRINT(os);
        os << algorithm_info_2_json_str(i);
    }
    os << "]}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void set_algorithm(const std::string&   cmd,
                   el_algorithm_type_t  algorithm_type,
                   el_algorithm_type_t& current_algorithm_type) {
    auto        os             = std::ostringstream(std::ios_base::ate);
    const auto& algorithm_info = algorithm_delegate->get_algorithm_info(algorithm_type);
    auto        ret            = algorithm_type == algorithm_info.type ? EL_OK : EL_EINVAL;

    if (algorithm_info.type != current_algorithm_type) [[likely]] {
        current_algorithm_type = algorithm_info.type;
        if (is_ready.load()) [[likely]]
            *storage << el_make_storage_kv("current_algorithm_type", current_algorithm_type);
    }

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(ret) << ", \"data\": ";
    switch (algorithm_info.type) {
    case EL_ALGO_TYPE_FOMO: {
        el_algorithm_fomo_config_t info_and_conf{};
        auto                       kv{el_make_storage_kv_from_type(info_and_conf)};
        if (storage->contains(kv.key)) *storage >> el_make_storage_kv_from_type(info_and_conf);
        os << algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;
    case EL_ALGO_TYPE_IMCLS: {
        el_algorithm_imcls_config_t info_and_conf{};
        auto                        kv{el_make_storage_kv_from_type(info_and_conf)};
        if (storage->contains(kv.key)) *storage >> el_make_storage_kv_from_type(info_and_conf);
        os << algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;
    case EL_ALGO_TYPE_PFLD: {
        el_algorithm_pfld_config_t info_and_conf{};
        auto                       kv{el_make_storage_kv_from_type(info_and_conf)};
        if (storage->contains(kv.key)) *storage >> el_make_storage_kv_from_type(info_and_conf);
        os << algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;
    case EL_ALGO_TYPE_YOLO: {
        el_algorithm_yolo_config_t info_and_conf{};
        auto                       kv{el_make_storage_kv_from_type(info_and_conf)};
        if (storage->contains(kv.key)) *storage >> el_make_storage_kv_from_type(info_and_conf);
        os << algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;
    default:
        os << "{\"type\": " << static_cast<unsigned>(algorithm_info.type)
           << ", \"categroy\": " << static_cast<unsigned>(algorithm_info.categroy)
           << ", \"input_from\": " << static_cast<unsigned>(algorithm_info.input_from) << ", \"config\": {}}";
    }
    os << "}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void get_algorithm_info(const std::string& cmd, const el_algorithm_type_t& current_algorithm_type) {
    auto        os             = std::ostringstream(std::ios_base::ate);
    const auto& algorithm_info = algorithm_delegate->get_algorithm_info(current_algorithm_type);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK) << ", \"data\": ";
    switch (algorithm_info.type) {
    case EL_ALGO_TYPE_FOMO: {
        el_algorithm_fomo_config_t info_and_conf{};
        auto                       kv{el_make_storage_kv_from_type(info_and_conf)};
        if (storage->contains(kv.key)) *storage >> el_make_storage_kv_from_type(info_and_conf);
        os << algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;
    case EL_ALGO_TYPE_IMCLS: {
        el_algorithm_imcls_config_t info_and_conf{};
        auto                        kv{el_make_storage_kv_from_type(info_and_conf)};
        if (storage->contains(kv.key)) *storage >> el_make_storage_kv_from_type(info_and_conf);
        os << algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;
    case EL_ALGO_TYPE_PFLD: {
        el_algorithm_pfld_config_t info_and_conf{};
        auto                       kv{el_make_storage_kv_from_type(info_and_conf)};
        if (storage->contains(kv.key)) *storage >> el_make_storage_kv_from_type(info_and_conf);
        os << algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;
    case EL_ALGO_TYPE_YOLO: {
        el_algorithm_yolo_config_t info_and_conf{};
        auto                       kv{el_make_storage_kv_from_type(info_and_conf)};
        if (storage->contains(kv.key)) *storage >> el_make_storage_kv_from_type(info_and_conf);
        os << algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;
    default:
        os << "{\"type\": " << static_cast<unsigned>(algorithm_info.type)
           << ", \"categroy\": " << static_cast<unsigned>(algorithm_info.categroy)
           << ", \"input_from\": " << static_cast<unsigned>(algorithm_info.input_from) << ", \"config\": {}}";
    }
    os << "}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

}  // namespace frontend::callback
