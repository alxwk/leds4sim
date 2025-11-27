#include "threshold.h"
#include <stdexcept>
#include <string>

namespace {

template <typename T>
std::vector<T> prepare_vector(const libconfig::Setting &s)
{
    std::vector<T> v;

    if (s.exists("limits")) {
        const auto &a = s.lookup("limits");

        std::transform(a.begin(), a.end(), std::back_inserter(v), [](const auto &e){return T(e);});
    } else {
        v.push_back(T());
    }
    return v;
}

} // namespace

threshold_base *make_threshold(const libconfig::Setting &s, const volatile void* a)
{
    const uint8_t *const p = (uint8_t*)(a) + (unsigned int)(s.lookup("offset"));

    if (!s.exists("type")) {
        return new threshold<bool>((bool*)p);
    }

    const auto &t = s.lookup("type");
    const auto t_s = std::string(t);

    if (t_s == "float") {
        return new threshold<float>((float*)p, prepare_vector<float>(s));
    } else if (t_s == "double") {
        return new threshold<double>((double*)p, prepare_vector<double>(s));
    } else if (t_s == "int") {
        return new threshold<int>((int*)p, prepare_vector<int>(s));
    } else if (t_s == "bool") {
        return new threshold<bool>((bool*)p);
    } else {
        throw std::runtime_error("wrong type in config:" + t.getPath());
    }
}
