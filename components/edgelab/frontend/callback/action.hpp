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
#include "internal/action_helper.hpp"

namespace frontend::callback {

using namespace frontend::utility;
using namespace frontend::static_resource;

void set_action(const std::vector<std::string>& argv) {
    auto        os              = std::ostringstream(std::ios_base::ate);
    auto*       action_delegate = ActionDelegate::get_delegate();
    std::string cmd{};
    char        action[CMD_MAX_LENGTH]{};
    uint16_t    crc16_maxim = 0xffff;
    auto        ret         = EL_OK;

    if (argv[1].size()) [[likely]] {
        ret = action_delegate->set_condition(argv[1]) ? EL_OK : EL_EINVAL;
        if (ret != EL_OK) [[unlikely]]
            goto ActionReply;
    }
    if (argv[2].size()) [[likely]] {
        cmd = argv[2];
        cmd.insert(0, "AT+");
        action_delegate->set_true_cb([=]() {
            el_err_code_t ret = instance->exec_non_lock(cmd);
            auto          os  = std::ostringstream(std::ios_base::ate);
            os << REPLY_EVT_HEADER << "\"name\": \"" << argv[0] << "\", \"code\": " << static_cast<int>(ret)
               << ", \"data\": {\"true\": " << string_2_str(argv[2]) << "}}\n";
            auto str{os.str()};
            serial->send_bytes(str.c_str(), str.size());
        });
    }
    if (argv[3].size()) [[likely]] {
        cmd = argv[3];
        cmd.insert(0, "AT+");
        action_delegate->set_false_exception_cb([=]() { instance->exec_non_lock(cmd); });
    }

    if (is_ready.load()) [[likely]] {
        auto builder = std::ostringstream(std::ios_base::ate);
        builder << argv[1] << '\t' << argv[2] << '\t' << argv[3];
        cmd = builder.str();

        crc16_maxim = el_crc16_maxim(reinterpret_cast<const uint8_t*>(cmd.c_str()), cmd.size());
        std::strncpy(action, cmd.c_str(), sizeof(action) - 1);
        ret = storage->emplace(el_make_storage_kv("edgelab_action", action)) ? EL_OK : EL_EIO;
    }

ActionReply:
    os << REPLY_CMD_HEADER << "\"name\": \"" << argv[0] << "\", \"code\": " << static_cast<int>(ret)
       << ", \"data\": {\"crc16_maxim\": " << static_cast<unsigned>(crc16_maxim)
       << ", \"cond\": " << string_2_str(argv[1]) << ", \"true\": " << string_2_str(argv[2])
       << ", \"false_or_exception\": " << string_2_str(argv[3]) << "}}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void get_action(const std::string& cmd) {
    auto     os              = std::ostringstream(std::ios_base::ate);
    auto*    action_delegate = ActionDelegate::get_delegate();
    char     action[CMD_MAX_LENGTH]{};
    uint16_t crc16_maxim = 0xffff;
    auto     ret         = EL_OK;

    if (action_delegate->has_condition() && storage->contains("edgelab_action")) {
        ret         = storage->get(el_make_storage_kv("edgelab_action", action)) ? EL_OK : EL_EINVAL;
        crc16_maxim = el_crc16_maxim(reinterpret_cast<const uint8_t*>(&action[0]), std::strlen(&action[0]));
    }

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(ret)
       << ", \"data\": {\"crc16_maxim\": " << static_cast<unsigned>(crc16_maxim) << ", " << action_str_2_json(action)
       << "}}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

}  // namespace frontend::callback
