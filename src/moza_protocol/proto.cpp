#include "proto.h"

#include <numeric>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>

#include "get_reply.h"


namespace {

const int MAGIC_VALUE = 0x0d;

void debug_print(const std::vector<uint8_t> &v)
{
    for (auto b: v) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << int(b) << ' ';
    }
    std::cout << std::endl;
}

} // namespace

namespace moza {

bool debug = false;

uint8_t chksum(const std::vector<uint8_t>& data)
{
    unsigned int ret = std::accumulate(data.begin(), data.end(), MAGIC_VALUE);

    return uint8_t(ret % 0x100);
}

void set_led_color(LibSerial::SerialPort &port, led_set ctl, uint8_t n, RGB color)
{
    const auto &c = color.rgb();
    std::vector<uint8_t> req = {0x7e, 7, 0x3f, 0x17, 0x1f, ctl, 0xff, n,
                              uint8_t(std::get<0>(c)),
                              uint8_t(std::get<1>(c)),
                              uint8_t(std::get<2>(c))}; // 7

    req.push_back(moza::chksum(req));
    if (debug) debug_print(req);

    if (port.IsOpen()) {
        port.DrainWriteBuffer();
        port.Write(req);
    }
}

void set_rpm_mode(LibSerial::SerialPort &port, mode m)
{
    std::vector<uint8_t> req = {0x7e, 3, 0x3f, 0x17,
                              0x1c, 0, m}; // 3

    req.push_back(moza::chksum(req));
    if (debug) debug_print(req);

    if (port.IsOpen()) {
        port.DrainWriteBuffer();
        port.Write(req);
    }
}

void set_telemetry_colors(LibSerial::SerialPort &port, led_set ctl, const std::vector<color_n> &set)
{
    const std::vector<uint8_t> head = { 0x7e, 0, 0x3f, 0x17, 0x19, ctl };
    std::vector<std::vector<uint8_t> > req;
    std::vector s = set;

    // better have it sorted by led numbers
    std::sort(s.begin(), s.end(), [](const auto &a, const auto &b)->bool {
        return (a.first < b.first);
    });

    // form necessary number of requests containing max 5 items each
    for (auto p = s.begin(); p < s.end();) {
        auto t = head;
        int j = 0;

        for (; j < 5 && p < s.end(); ++j, ++p) {
            t.push_back(p->first);

            auto c = p->second.rgb();

            t.push_back(uint8_t(std::get<0>(c)));
            t.push_back(uint8_t(std::get<1>(c)));
            t.push_back(uint8_t(std::get<2>(c)));
        }
        t[1] = (j * 4) + 2;
        t.push_back(moza::chksum(t));
        req.push_back(t);
    }

    for (auto const &e: req) {
        if (debug) debug_print(e);

        if (port.IsOpen()) {
            port.DrainWriteBuffer();
            port.Write(e);
        }
    }
}

void send_telemetry(LibSerial::SerialPort &port, led_set ctl, uint32_t mask)
{
    std::vector<uint8_t> req = {0x7e, 6, 0x3f, 0x17, 0x1a, ctl,
                                uint8_t(mask & 0xff), uint8_t(mask >> 8 & 0xff),
                                uint8_t(mask >> 16 & 0xff), uint8_t(mask  >> 24 & 0xff)}; // 6

    req.push_back(moza::chksum(req));
    if (debug) debug_print(req);

    if (port.IsOpen()) {
        port.DrainWriteBuffer();
        port.Write(req);
    }
}

mode get_leds_mode(LibSerial::SerialPort &port, led_set ctl)
{
    std::vector<uint8_t> req = {0x7e, 3, 0x40, 0x17,
                              0x1c, ctl, 0}; // 3

    req.push_back(moza::chksum(req));

    uint8_t m = 0;

    if (debug) debug_print(req);

    if (port.IsOpen())  {
        auto ans = get_reply(port, req);

        if (debug)  debug_print(ans);
        m = ans[6];
    }
    assert(m <= on);
    return mode(m);
}

RGB get_led_color(LibSerial::SerialPort &port, led_set ctl, uint8_t n)
{
    std::vector<uint8_t> req = {0x7e, 7, 0x40, 0x17, 0x1f, ctl, 0xff, n, 0, 0, 0};

    req.push_back(moza::chksum(req));

    std::vector<uint8_t> ans = {0};

    if (debug) debug_print(req);

    if (port.IsOpen()) {
        ans = get_reply(port, req);
        if (debug)  debug_print(ans);
    }
    return RGB(ans[8], ans[9], ans[10]);
}

} // namespace moza
