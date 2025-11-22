#include "rgb.h"
#include <stdexcept>

RGB RGB::from_int(int n)
{
    return RGB(n >> 16 & 0xff, n >> 8 & 0xff, n & 0xff);
}

RGB RGB::from_name(const std::string& s)
{
    if (s == "red") {
        return RGB::red;
    } else if (s == "gold") {
        return RGB::gold;
    } else if (s == "yellow") {
        return RGB::yellow;
    } else if (s == "green") {
        return RGB::green;
    } else if (s == "blue") {
        return RGB::blue;
    } else if (s == "cyan") {
        return RGB::cyan;
    } else if (s == "mozacyan") {
        return RGB::mozacyan;
    } else if (s == "magenta") {
        return RGB::magenta;
    } else if (s == "white") {
        return RGB::white;
    } else if (s == "black") {
        return RGB::black;
    } else {
        throw std::runtime_error("invalid color name");
    }
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
