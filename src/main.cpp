#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cstring>

#include <unistd.h>

#include <libserial/SerialPort.h>
#include <libconfig.h++>

#include <getopt.h>
#include <basedir.h>
#include <basedir_fs.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include <rgb.h>
#include <proto.h>
#include "indicator.h"

using namespace std;
namespace fs = std::filesystem;

namespace {

LibSerial::SerialPort port;

void init_port()
{
    fs::directory_entry dir("/dev/serial/by-id");
    const string base_serial_filename = "Base";

    if (!dir.exists()) {
        throw runtime_error("Serial path doesn't exist, connect the wheel.");
    }

    auto const& p = find_if(fs::directory_iterator(dir), fs::directory_iterator(),
                            [&](auto const &f) {
                                return f.path().filename().string().find(base_serial_filename)
                                        != string::npos;
                            });

    if (p != fs::directory_iterator()) {
        cout << p->path() << endl;
    } else {
        throw runtime_error("No serial device.");
    }

    port.Open(p->path());
}

bool no_wheel = false;

void check_opts(int argc, char* argv[])
{
    int optc;

    for (;;) {
        int option_index = 0;
        static struct option long_options[] = {
            {"debug", no_argument, 0, 0},
            {"no-wheel", no_argument, 0, 0},
        };

        optc = getopt_long(argc, argv, "dn", long_options, &option_index);
        if (optc == -1 ) break;

        switch (optc) {
            case 0:
            case 'd':
                moza::debug = true;
                break;
            case 1:
            case 'n':
                no_wheel = true;
                break;
            default:
                cerr << "Usage: " << argv[0] << " [-d|--debug] [-n|--no-wheel]" << endl;
                cerr << "\t-d, --debug\tprint serial data" << endl;
                cerr << "\t-n, --no-wheel\tdon't interact with the real device, useful for debugging" << endl;
                exit(EXIT_FAILURE);
        }
    }
}

string config_name()
{
    string conf_fname = "leds4sim.conf";

    if (!fs::exists(conf_fname)) { // not in current directory
        xdgHandle xdg;
        const char *p = nullptr;

        if (xdgInitHandle(&xdg) != nullptr) {
            p = xdgConfigFind(conf_fname.c_str(), &xdg);
        }
        if (p && *p)    conf_fname = p;
        else            conf_fname.clear();
    }
    return conf_fname;
}

} // namespace

int main(int argc, char* argv[])
{
    using namespace libconfig;

    check_opts(argc, argv);

    if (!no_wheel) init_port();

    Config cfg;

    cfg.setAutoConvert(true);

    const string conf_fname = config_name();

    if (conf_fname.empty()) {
        cerr << "Config file not found, can't work without it." << endl;
        return EXIT_FAILURE;
    }

    try {
        cfg.readFile(conf_fname);
    } catch (const ParseException &ex) {
        cerr << "config parse error " << ex.getFile() << ":" << ex.getLine()
             << " - " << ex.getError() << std::endl;
        return EXIT_FAILURE;
    }

    string mmap_fname = cfg.lookup("mmap_file").c_str();
    int mfd = open(mmap_fname.c_str(), O_RDONLY);

    if (mfd < 0 && (errno == ENOENT || errno == EINVAL)) {
        cerr << "Telemetry not found in shared memory, waiting for the game to start (Ctrl+C to cancel)." << endl;
    }

    while (mfd < 0) {
        sleep(1);
        mfd = open(mmap_fname.c_str(), O_RDONLY);
    }

    const volatile uint8_t *const data = (uint8_t*)mmap(NULL, int(cfg.lookup("mmap_size")), PROT_READ, MAP_PRIVATE, mfd, 0);
    int cycle = cfg.lookup("cycle_ms");
    const auto& rpm_leds = cfg.lookup("rpm.leds");
    vector<indicator> rpm_indicators;
    vector<moza::color_n> rpm_colors;

    for (const Setting &c: rpm_leds) {
        indicator i = indicator(c, data);

        rpm_indicators.push_back(i);
        rpm_colors.push_back(make_pair(i.n(), i.color()));
    }

    moza::set_telemetry_colors(port, moza::RPM, rpm_colors);
    moza::set_rpm_mode(port, moza::TELEMETRY);
    moza::send_telemetry(port, moza::RPM, 0);

    vector<moza::color_n> p1;

    // get current idle button colors
    for (int i = 0; i < 14; ++i) {
        p1.push_back(make_pair(i, moza::get_led_color(port, moza::BUTTON, i)));
    }

    // set new colors for the leds used for telemetry
    const auto &btn_leds = cfg.lookup("button_leds");
    vector<indicator> btn_indicators;
    vector<int> used_btns;
    vector<moza::color_n> btn_colors = p1;

    for (const Setting &c: btn_leds) {
        indicator i = indicator(c, data);

        btn_indicators.push_back(i);
        used_btns.push_back(i.n());
        btn_colors.at(i.n()) = make_pair(i.n(), i.color());
    }
    moza::set_telemetry_colors(port, moza::BUTTON, btn_colors);

    uint32_t used_bits = 0;

    for (auto n: used_btns) {
        used_bits |= 1 << n;
    }

    const uint32_t unused = 0x3fff & ~used_bits;

    for(;;) {
        uint32_t bits = 0;

        // inactive or paused
        if (!data[0] || data[4])    goto sleep;

        btn_colors.clear();
        for (auto &p: btn_indicators) {
            p.update();

            if (p.is_on()) {
                bits |= 1 << p.n();
                if (p.is_multicolor()) {
                    btn_colors.push_back(make_pair(p.n(), p.color()));
                }
            }
        }

        if (!btn_colors.empty()) {
            moza::set_telemetry_colors(port, moza::BUTTON, btn_colors);
            usleep(1000);
        }
        moza::send_telemetry(port, moza::BUTTON, bits | unused);

        rpm_colors.clear();
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
        if (!rpm_colors.empty()) {
            moza::set_telemetry_colors(port, moza::RPM, rpm_colors);
            usleep(1000);
        }
        moza::send_telemetry(port, moza::RPM, bits);
sleep:
        usleep(cycle*1000L);
    }

    return 0;
}
