#ifndef GET_REPLY_H
#define GET_REPLY_H

#include <vector>
#include <cstdint>
#include <libserial/SerialPort.h>

namespace moza {

std::vector<uint8_t> get_reply(LibSerial::SerialPort &port, const std::vector<uint8_t>& request, int retries = 2);

}

#endif // GET_REPLY_H
