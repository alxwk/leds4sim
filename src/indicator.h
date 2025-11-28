#ifndef INDICATOR_H
#define INDICATOR_H

#include <variant>
#include <vector>
#include <cstdint>
#include <libconfig.h++>

#include <rgb.h>

class indicator {
public:
    using value_p = std::variant<int*, long*, float*, double*, bool*>;
    using value_t = std::variant<int, long, float, double, bool>;
    enum val_type { INT, LONG, FLOAT, DOUBLE, BOOL };

    indicator(const libconfig::Setting &s, const volatile void* baseaddr);

// to avoid any discrepancy when the value in mmap has changed between
// calls to is_on() and color(), it's better to copy it locally first
    void update();

    uint8_t n() const { return m_n; }

    bool is_on() const;
    bool is_multicolor() const { return m_colors.size() > 1; }
    RGB color() const;

private:
    value_p m_p;
    value_t m_val;
    value_p m_total_p;
    double m_total_val;
    bool has_total;

    uint8_t m_n;

    // having many levels in a bool indicator is absurd, so in this case only
    // the 0th element matters
    std::vector<RGB> m_colors;
    std::vector<double> m_levels;
    std::vector<double> m_levels_p;
    std::vector<bool> m_inv;
};

#endif // INDICATOR_H
