#include "rgb.h"
#include <stdexcept>
#include <map>

RGB RGB::from_int(int n)
{
    return RGB(n >> 16 & 0xff, n >> 8 & 0xff, n & 0xff);
}

const RGB RGB::red      (0xFF, 0,    0);
const RGB RGB::gold     (0xFF, 0xD7, 0);
const RGB RGB::yellow   (0xFF, 0xFF, 0);
const RGB RGB::green    (0,    0xFF, 0);
const RGB RGB::blue     (0,    0,    0xFF);
const RGB RGB::cyan     (0,    0xFF, 0xFF);
const RGB RGB::mozacyan (0x56, 0xF7, 0xFC);
const RGB RGB::magenta  (0xFF, 0,    0xFF);
const RGB RGB::white    (0xFF, 0xFF, 0xFF);
const RGB RGB::black    (0,    0,    0);

namespace {

const std::map<const std::string, RGB> name_map {
    {"red",     RGB::red},
    {"gold",    RGB::gold},
    {"yellow",  RGB::yellow},
    {"green",   RGB::green},
    {"blue",    RGB::blue},
    {"cyan",    RGB::cyan},
    {"mozacyan", RGB::mozacyan},
    {"magenta", RGB::magenta},
    {"white",   RGB::white},
    {"black",   RGB::black}
};

}

RGB RGB::from_name(const std::string& s)
{
    try {
        return name_map.at(s);
    } catch(const std::out_of_range&) {
        throw std::runtime_error("invalid color name");
    }
}

