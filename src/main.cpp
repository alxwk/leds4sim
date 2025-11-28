#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cstring>

//#include <getopt.h>
#include <unistd.h>

#include <libserial/SerialPort.h>
#include <libconfig.h++>

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include <rgb.h>
#include <proto.h>
#include "indicator.h"


using namespace std;

LibSerial::SerialPort port;

void init_port()
{
    using namespace filesystem;

    directory_entry dir("/dev/serial/by-id");
    const string base_serial_filename = "Base";
//    directory_entry dir("/dev");
//    const string base_serial_filename = "ttyS0";

    if (!dir.exists()) {
        throw runtime_error("Serial path doesn't exist, connect the wheel.");
    }

    auto const& p = find_if(directory_iterator(dir), directory_iterator(),
                            [&](auto const &f) {
                                return f.path().filename().string().find(base_serial_filename) 
                                        != string::npos;
                            });

    if (p != directory_iterator()) {
            cout << p->path() << endl;
    } else {
        throw runtime_error("No serial device.");
    }

    port.Open(p->path());
//    port.SetDefaultSerialPortParameters();
}

void set_default_button_colors(LibSerial::SerialPort& port)
{
    vector<moza::color_n> p1;

    // get current idle button colors
    for (int i = 0; i < 14; ++i) {
        p1.push_back(make_pair(i, moza::get_led_color(port, moza::button, i)));
    }
    // set default telemetry colors to the current idle ones
    moza::set_telemetry_colors(port, moza::button, p1);
}

RGB rgb_from_setting(const libconfig::Setting& s)
{
    switch (s.getType()) {
    case libconfig::Setting::TypeString:
        return RGB::from_name(s);
    case libconfig::Setting::TypeInt:
        return RGB::from_int(s);
    default:
        throw runtime_error("wrong color name in config");
    }
}

int main()
{
    using namespace libconfig;

    Config cfg;

    cfg.setAutoConvert(true);

    try {
        cfg.readFile("leds4sim.conf");
    } catch (const ParseException &ex) {
        cerr << "config parse error " << ex.getFile() << ":" << ex.getLine()
             << " - " << ex.getError() << std::endl;
        return(EXIT_FAILURE);
    }

    string mmap_fname = cfg.lookup("mmap_file").c_str();

    int mfd = open(mmap_fname.c_str(), O_RDONLY);

    if (mfd < 0 && (errno == ENOENT || errno == EINVAL)) {
        cerr << "Telemetry not found in shared memory, waiting for the game start (Ctrl+C to cancel)." << endl;
    }

    while (mfd < 0) {
        sleep(1);
        mfd = open(mmap_fname.c_str(), O_RDONLY);
    }

    const volatile void *const data = mmap(NULL, int(cfg.lookup("mmap_size")), PROT_READ, MAP_PRIVATE, mfd, 0);

    int cycle = cfg.lookup("cycle");

    const auto& rpm_leds = cfg.lookup("rpm.leds");
    vector<indicator> rpm_indicators;
    vector<moza::color_n> rpm_colors;
    
    for (const Setting &c: rpm_leds) {
        indicator i = indicator(c, data);

        rpm_indicators.push_back(i);
        rpm_colors.push_back(make_pair(i.n(), i.color()));
    }

   init_port();

   moza::set_telemetry_colors(port, moza::rpm, rpm_colors);

   moza::set_rpm_mode(port, moza::on);
   moza::send_telemetry(port, moza::rpm, 0);

   set_default_button_colors(port);

    // set new colors for the leds used for telemetry
    const auto &btn_leds = cfg.lookup("button_leds");
    vector<indicator> btn_indicators;
    vector<int> used_btns;
    vector<moza::color_n> btn_colors;

    for (const Setting &c: btn_leds) {
        indicator i = indicator(c, data);

        btn_indicators.push_back(i);
        used_btns.push_back(i.n());
        btn_colors.push_back(make_pair(i.n(), i.color()));
    }

    moza::set_telemetry_colors(port, moza::button, btn_colors);

    uint32_t used_bits = 0;
    for (auto n: used_btns) {
        used_bits |= 1 << n;
    }
    const uint32_t unused = 0x3fff & ~used_bits;

    for(;;) {
        uint32_t bits = 0;
        btn_colors.clear();
        rpm_colors.clear();

        for (auto &p: btn_indicators) {
            p.update();

            if (p.is_on()) {
                bits |= 1 << p.n();
                if (p.is_multicolor()) {
                    btn_colors.push_back(make_pair(p.n(), p.color()));
                }
            }
        }
        // cout << "btn: " << hex << bits << endl;
        // for (const auto &i : btn_colors) {
        //     RGB r = i.second;
        //     cout << i.first << '(' << get<0>(r()) << ", " << get<1>(r())  << ", " << get<2>(r()) << ')' << endl;
        // }

        moza::send_telemetry(port, moza::button, bits | unused);

        bits = 0;
        for (auto &p: rpm_indicators) {
            p.update();
            if (p.is_on()) {
                bits |= 1 << p.n();
                if (p.is_multicolor()) {
                    rpm_colors.push_back(make_pair(p.n(), p.color()));
                }
            }
        }
        // cout << "rpm: " << hex << bits << endl;
        // for (const auto &i : rpm_colors) {
        //     RGB r = i.second;
        //     cout << i.first << '(' << get<0>(r()) << ", " << get<1>(r())  << ", " << get<2>(r()) << ')' << endl;
        // }
        moza::send_telemetry(port, moza::rpm, bits);

        usleep(cycle*1000L);
    }

    return 0;
}
