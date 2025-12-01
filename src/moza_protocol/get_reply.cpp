#include "proto.h"
#include <iostream>
#include <iomanip>

namespace {

const size_t timeout = 200; // ms

std::vector<uint8_t> receive_answer(LibSerial::SerialPort &port, const std::vector<uint8_t> &req)
{
    enum { WAIT, STARTED, END} state = WAIT;

    std::vector<uint8_t> ret;

    uint8_t byte;
    std::vector<uint8_t> buffer;

    while (state != END) {
        switch(state) {
        case WAIT:
            port.ReadByte(byte, timeout);   // can throw ReadTimeout
            if (byte == 0x7e) {
                ret.push_back(byte);
                state = STARTED;
            }
            break;
        case STARTED:
            port.Read(buffer, 3, timeout); // header
            ret.insert(ret.end(),buffer.begin(), buffer.end());

            port.Read(buffer, ret[1], timeout);
            port.ReadByte(byte, timeout);

            if (req[2] != (ret[2] & 0x7f) ||
                req[3] != ((ret[3] & 0xf) << 4 | (ret[3] & 0xf0) >> 4)) {
                ret.clear();
                state = WAIT;
                break;
            }

            ret.insert(ret.end(), buffer.begin(), buffer.end());
            if (byte != moza::chksum(ret)) {
                throw NOK_error("NOK"); // NOK
            }
            ret.push_back(byte);
            state = END;
            break;
        default:
            break;
        }
    }

    return ret;
}

} // namespace

namespace moza {

std::vector<uint8_t> get_reply(LibSerial::SerialPort &port, const std::vector<uint8_t>& request, int retries)
{
    int tries = retries;
    std::vector<uint8_t> v;

    while (tries > 0) {
        if (debug) {
            for (auto b: request) {
                std::cout << std::hex << std::setw(2) << int(b) << ' ';
            }
            std::cout << std::endl;
        }
        port.DrainWriteBuffer();
        port.Write(request);
        if (port.IsDataAvailable()) port.FlushInputBuffer();

        try {
            v = receive_answer(port, request);
            break;
        } catch(const NOK_error &e) {
            if (--tries == 0) {
                std::cerr << e.what() << ", failed" << std::endl;
                throw;
            }
            std::cerr << e.what() << ", retrying" << std::endl;
        }
    }

    if (debug) {
        std::cout << '\t';
        for (auto b: v) {
            std::cout << std::hex << std::setw(2) << int(b) << ' ';
        }
        std::cout << std::endl;
    }

    return v;
}
} // namespace moza
