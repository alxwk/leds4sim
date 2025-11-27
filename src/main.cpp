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
#include <threshold.h>

#include <iterator>

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

//    int mfd = shm_open(mmap_fname.c_str(), O_RDONLY, 0);
    int mfd = open(mmap_fname.c_str(), O_RDONLY);

    if (mfd < 0 && (errno == ENOENT || errno == EINVAL)) {
        cerr << "Telemetry not found in shared memory, waiting for the game start (Ctrl+C to cancel)." << endl;
    }

    while (mfd < 0) {
        sleep(1);
        mfd = open(mmap_fname.c_str(), O_RDONLY);
    }

    const volatile void *const data = mmap(NULL, int(cfg.lookup("mmap_size")), PROT_READ, MAP_PRIVATE, mfd, 0);

    const auto& rpm_colors_conf = cfg.lookup("rpm.colors");

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
    const auto &btn_colors_conf = cfg.lookup("button_leds");
    vector<moza::color_n> p1;
    vector<int> used_btns;
    vector<pair<int, threshold_base*> > th;

    for (int i = 0; i < btn_colors_conf.getLength(); ++i) {
        const Setting &conf = btn_colors_conf[i];
        int n = int(conf["n"])-1;
        used_btns.push_back(n);

        const Setting &color = conf["color"];

        p1.push_back(make_pair(n, rgb_from_setting(color)));
        th.push_back(make_pair(n, make_threshold(conf.lookup("value"), data)));
    }
    moza::set_telemetry_colors(port, moza::button, p1);

    uint32_t used_bits = 0;
    for (auto n: used_btns) {
        used_bits |= 1 << n;
    }
    const uint32_t unused = 0x3fff & ~used_bits;

    const auto& rpm_val = cfg.lookup("rpm.value");


    auto b = make_threshold(rpm_val, data);

    for(;;) {
        uint32_t bits = 0;

        for (auto p: th) {
            if (p.second->exceeded()) {
                bits |= 1 << p.first;
            }
        }

/*
        moza::send_telemetry(port, moza::button, bits | unused);
    */
        usleep(500000L);
    }

    return 0;
}
