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

void get_available_sensors(const std::string& cmd) {
    auto        os                 = std::ostringstream(std::ios_base::ate);
    const auto& registered_sensors = device->get_all_sensor_info();

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK) << ", \"data\": [";
    DELIM_RESET;
    for (const auto& i : registered_sensors) {
        DELIM_PRINT(os);
        os << sensor_info_2_json(i);
    }
    os << "]}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void set_sensor(const std::string& cmd, uint8_t sensor_id, bool enable, uint8_t& current_sensor_id) {
    auto os          = std::ostringstream(std::ios_base::ate);
    auto sensor_info = device->get_sensor_info(sensor_id);
    auto ret         = sensor_info.id ? EL_OK : EL_EINVAL;

    if (ret != EL_OK) [[unlikely]]
        goto SensorReply;

    // camera
    if (sensor_info.type == el_sensor_type_t::EL_SENSOR_TYPE_CAM) {
        auto* camera = device->get_camera();

        device->set_sensor_state(sensor_id, EL_SENSOR_STA_LOCKED);
        if (static_cast<bool>(*camera)) {
            ret = camera->deinit();
            if (ret != EL_OK) [[unlikely]]
                goto SensorError;
        }

        if (enable) [[likely]] {
            ret = camera->init(240, 240);  // TODO: custom resolution
            if (ret != EL_OK) [[unlikely]]
                goto SensorError;
        }
        device->set_sensor_state(sensor_id, EL_SENSOR_STA_AVAIL);
        sensor_info = device->get_sensor_info(sensor_id);

        if (current_sensor_id != sensor_id) {
            current_sensor_id = sensor_id;
            if (is_ready.load()) [[likely]]
                *storage << el_make_storage_kv("current_sensor_id", current_sensor_id);
        }
    } else
        ret = EL_ENOTSUP;
    goto SensorReply;

SensorError:
    current_sensor_id = 0;

SensorReply:
    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(ret)
       << ", \"data\": {\"sensor\": " << sensor_info_2_json(sensor_info) << "}}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

void get_sensor_info(const std::string& cmd, uint8_t current_sensor_id) {
    const auto& sensor_info = device->get_sensor_info(current_sensor_id);
    auto        os          = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK)
       << ", \"data\": " << sensor_info_2_json(sensor_info) << "}\n";

    const auto& str{os.str()};
    serial->send_bytes(str.c_str(), str.size());
}

}  // namespace frontend::callback
