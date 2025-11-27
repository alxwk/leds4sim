#ifndef THRESHOLD_H
#define THRESHOLD_H

#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <libconfig.h++>

class threshold_base {
public:
    virtual unsigned int exceeded() const = 0;
};

template<typename T> class threshold : public threshold_base {
public:
    threshold(T* p, std::vector<T> v): m_p(p), m_levels(v) {}

    virtual unsigned int exceeded() const {
        return std::distance(m_levels.begin(), std::upper_bound(m_levels.begin(), m_levels.end(), *m_p));
    }

private:
    const volatile T* m_p;
    std::vector<T> m_levels;
};

template<> class threshold<bool> : public threshold_base {
public:
    threshold(bool* p): m_p(p) {}
    virtual unsigned int exceeded() const {return *m_p;}

private:
    const volatile bool* m_p;
};

threshold_base * make_threshold(const libconfig::Setting &s, const volatile void* a);

#endif // THRESHOLD_H
