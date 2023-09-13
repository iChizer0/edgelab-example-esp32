#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

#include "frontend/definations.hpp"
#include "frontend/static_resourse.hpp"
#include "frontend/utility.hpp"

namespace frontend::callback {

using namespace frontend::utility;
using namespace frontend::static_resource;

void set_info(const std::vector<std::string>& argv) {
    auto os = std::ostringstream(std::ios_base::ate);
    char info[CMD_MAX_LENGTH]{};
    auto ret = EL_EINVAL;

    std::strncpy(info, argv[1].c_str(), sizeof(info) - 1);
    ret = storage->emplace(el_make_storage_kv("edgelab_info", info)) ? EL_OK : EL_EINVAL;

    os << REPLY_CMD_HEADER << "\"name\": \"" << argv[0] << "\", \"code\": " << static_cast<int>(ret)
       << ", \"data\": {\"crc16_maxim\": "
       << static_cast<unsigned>(el_crc16_maxim(reinterpret_cast<uint8_t*>(&info[0]), std::strlen(&info[0]))) << "}}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void get_info(const std::string& cmd) {
    auto os = std::ostringstream(std::ios_base::ate);
    char info[CMD_MAX_LENGTH]{};
    auto ret = EL_OK;

    if (storage->contains("edgelab_info"))
        ret = storage->get(el_make_storage_kv("edgelab_info", info)) ? EL_OK : EL_EINVAL;

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(ret)
       << ", \"data\": {\"crc16_maxim\": "
       << static_cast<unsigned>(el_crc16_maxim(reinterpret_cast<uint8_t*>(&info[0]), std::strlen(&info[0])))
       << ", \"info\": " << string_2_str(info) << "}}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

}  // namespace frontend::callback
