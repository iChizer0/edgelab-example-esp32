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

#ifndef _EL_SERIAL_ESP_H_
#define _EL_SERIAL_ESP_H_

#include <driver/usb_serial_jtag.h>

#include "el_serial.h"

namespace edgelab {

class SerialEsp : public Serial {
   private:
    usb_serial_jtag_driver_config_t _driver_config;

   protected:
    bool _is_present;

   public:
    SerialEsp(usb_serial_jtag_driver_config_t driver_config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT());
    ~SerialEsp() override;
    EL_STA init() override;
    EL_STA deinit() override;
    char   echo(bool only_visible = true) override;
    char   get_char() override;
    size_t get_line(char* buffer, size_t size, const char delim = 0x0d) override;
    size_t write_bytes(const char* buffer, size_t size) override;

    operator bool() { return _is_present; }
};

}  // namespace edgelab

#endif