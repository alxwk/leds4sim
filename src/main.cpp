#include <iostream>
#include <filesystem>
#include <algorithm>
//#include <getopt.h>
#include <unistd.h>

#include <libserial/SerialPort.h>
#include <libconfig.h++>
#include <rgb.h>
#include <proto.h>

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
    auto type = s.getType();

    if (type == libconfig::Setting::TypeString) {
        return RGB::from_name(s);
    } else if (type == libconfig::Setting::TypeInt) {
        return RGB::from_int(s);
    } else {
        throw runtime_error("wrong color name in config");
    }
}

int main()
{
    using namespace libconfig;

    Config cfg;

    try {
        cfg.readFile("leds4sim.conf");
    } catch (const ParseException &ex) {
        cerr << "config parse error " << ex.getFile() << ":" << ex.getLine()
             << " - " << ex.getError() << std::endl;
        return(EXIT_FAILURE);
    }

    const Setting& rpm_colors_conf = cfg.lookup("rpm.colors");

    vector<moza::color_n> rpm_colors;
    for (int i = 0; i < 10; ++i) {
        const Setting &conf = rpm_colors_conf[i];

        rpm_colors.push_back(make_pair(i, rgb_from_setting(conf)));
    }

    init_port();

    moza::set_telemetry_colors(port, moza::rpm, rpm_colors);

    moza::set_rpm_mode(port, moza::on);
    moza::send_telemetry(port, moza::rpm, 0);

    set_default_button_colors(port);

    // set new colors for the leds used for telemetry
    const Setting &btn_colors_conf = cfg.lookup("button_leds");
    vector<moza::color_n> p1;
    vector<int> used_btns;

    for (int i = 0; i < btn_colors_conf.getLength(); ++i) {
        const Setting &conf = btn_colors_conf[i];
        int n = int(conf["n"])-1;
        used_btns.push_back(n);
        
        const Setting &color = conf["color"];

//        if (color.getLength() > 0) {
//            color = color[0];
//        }

        p1.push_back(make_pair(n, rgb_from_setting(color)));
    }
    moza::set_telemetry_colors(port, moza::button, p1);

    uint32_t bits = 0;
    for (auto n: used_btns) {
        bits |= 1 << n;
    }
    const uint32_t unused = 0x3fff & ~bits;

    for(int i = 0; i < 5; ++i) {
        moza::send_telemetry(port, moza::button, bits | unused);
        cout << '+' << flush;
        usleep(500000L);
        moza::send_telemetry(port, moza::button, ~bits | unused);
        cout << '+' << flush;
        usleep(500000L);
    }

    return 0;
}
