#ifndef PROTO_H
#define PROTO_H

#include <cstdint>
#include <vector>
#include <stdexcept>
#include <libserial/SerialPort.h>
#include "rgb.h"

class NOK_error : public std::runtime_error {
public:
explicit NOK_error(const std::string& whatArg [[maybe_unused]])
    : runtime_error(whatArg)
    {}
};


namespace moza {

typedef std::pair<uint8_t, RGB> color_n;
enum led_set : uint8_t { rpm, button };
enum mode : uint8_t { off, telemetry, on };

uint8_t chksum(const std::vector<uint8_t>& data);

RGB get_led_color(LibSerial::SerialPort &port, led_set ctl, uint8_t n);
mode get_leds_mode(LibSerial::SerialPort &port, led_set ctl);

void set_led_color(LibSerial::SerialPort &port, led_set ctl, uint8_t n, RGB color);
void set_rpm_mode(LibSerial::SerialPort &port, mode m); // doesn't work with buttons, they are always seem to be in telemetry mode
void set_telemetry_colors(LibSerial::SerialPort &port, led_set ctl, const std::vector<color_n> &set);
void send_telemetry(LibSerial::SerialPort &port, led_set ctl, uint32_t mask);

}	// namespace moza

#endif
