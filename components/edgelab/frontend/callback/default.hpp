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

void echo_cb(el_err_code_t ret, const std::string& msg) {
    auto os = std::ostringstream(std::ios_base::ate);

    if (ret != EL_OK)
        os << REPLY_LOG_HEADER << "\"name\": \"AT\", \"code\": " << static_cast<int>(ret)
           << ", \"data\": " << string_2_str(msg) << "}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void print_help(std::forward_list<el_repl_cmd_t> cmd_list) {
    auto os = std::ostringstream(std::ios_base::ate);

    for (const auto& cmd : cmd_list) {
        os << "  AT+" << cmd.cmd;
        if (cmd.args.size()) os << "=<" << cmd.args << ">";
        os << "\n    " << cmd.desc << "\n";
    }

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void get_device_id(const std::string& cmd) {
    auto os = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK) << ", \"data\": \""
       << std::uppercase << std::hex << device->get_device_id() << "\"}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void get_device_name(const std::string& cmd) {
    auto os = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK) << ", \"data\": \""
       << device->get_device_name() << "\"}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void get_device_status(const std::string& cmd, int32_t boot_count) {
    auto os = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK)
       << ", \"data\": {\"boot_count\": " << static_cast<unsigned>(boot_count)
       << ", \"is_ready\": " << (is_ready.load() ? 1 : 0) << "}}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void get_version(const std::string& cmd) {
    auto os = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK)
       << ", \"data\": {\"software\": \"" << EL_VERSION << "\", \"hardware\": \""
       << static_cast<unsigned>(device->get_chip_revision_id()) << "\"}}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void break_task(const std::string& cmd) {
    auto os = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK)
       << ", \"data\": " << el_get_time_ms() << "}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void task_status(const std::string& cmd, const std::atomic<bool>& sta) {
    auto os = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK)
       << ", \"data\": " << (sta.load() ? 1 : 0) << "}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

}  // namespace frontend::callback
