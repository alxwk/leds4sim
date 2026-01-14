#include "indicator.h"
#include <algorithm>
#include <stdexcept>
#include <libconfig.h++>

namespace {

RGB rgb_from_setting(const libconfig::Setting& s)
{
    switch (s.getType()) {
    case libconfig::Setting::TypeString:
        return RGB::from_name(s);
    case libconfig::Setting::TypeInt:
        return RGB::from_int(s);
    default:
        throw std::runtime_error("wrong color name in config");
    }
}

} // namespace

indicator::indicator(const libconfig::Setting &s, const volatile uint8_t *baseaddr)
    : m_total_p((int*)nullptr), has_total(false)
{
    const libconfig::Setting *v = nullptr;
    const libconfig::Setting *r = &s;

    try {
        for(;;) {
            if (r->exists("value")) {
                v = &(r->lookup("value"));
                break;
            } else {
                r = &(r->getParent());
            }
        }
    } catch(const libconfig::SettingNotFoundException& e) {
        throw std::runtime_error("value clause not found for " + s.getPath());
    }

    const volatile uint8_t *const p = baseaddr + (unsigned int)(v->lookup("offset"));

    if (s.exists("inv")) {
        const auto &i = s.lookup("inv");
        if (i.isAggregate()) {
            std::transform(i.begin(), i.end(), std::back_inserter(m_inv),
                           [](const auto& arg) {return bool(arg);});
        } else {
            m_inv.push_back(bool(i));
        }
    } else {
        m_inv.push_back(false);
    }

    if (v->exists("total")) {
        const auto &total_s = v->lookup("total");
        if (total_s.isAggregate()) {
            const std::string t = total_s.lookup("type");
            const volatile uint8_t *p = baseaddr + (unsigned long)(total_s.lookup("offset"));

            if (t == "float")           m_total_p = (float*)p;
            else if (t == "double")     m_total_p = (double*)p;
            else if (t == "int")        m_total_p = (int*)p;
            else if (t == "long")       m_total_p = (long*)p;
            else {
                throw std::runtime_error("invalid type at " + s.getPath());
            }
        } else {
            m_total_val = double(total_s);
        }
        has_total = true;
    }

    if (s.exists("level")) {
        const auto &l = s.lookup("level");
        if (l.isAggregate()) {
            std::transform(l.begin(), l.end(), std::back_inserter(m_levels),
                           [](const auto &arg){ return double(arg); });
        } else {
            m_levels.push_back(double(l));
        }
    } else if (s.exists("level_p")) {
        if (!has_total) {
            throw std::runtime_error("no total specified for a percent value at " + s.getPath());
        }
        const auto &l = s.lookup("level_p");

        if (l.isAggregate()) {
            std::transform(l.begin(), l.end(), std::back_inserter(m_levels_p),
                           [](const auto &arg){ return double(arg) * 0.01; });
            std::fill_n(std::back_inserter(m_levels), l.getLength(), double());
        } else {
            m_levels_p.push_back(double(l) * 0.01);
            m_levels.push_back(double());
        }
    } else {
        m_levels.push_back(double());
    }

    if (v->exists("type")) {
        const auto& t = v->lookup("type");
        auto t_s = std::string(t);

        if (t_s == "float")         m_p = (float*)p;
        else if (t_s == "double")   m_p = (double*)p;
        else if (t_s == "int")      m_p = (int*)p;
        else if (t_s == "long")     m_p = (long*)p;
        else if (t_s == "bool")     m_p = (bool*)p;
        else {
            throw std::runtime_error("wrong type in config:" + t.getPath());
        }
    } else {
        // bool by default
        m_p = (bool*)p;
    }

    update();

    std::fill_n(std::back_inserter(m_inv), m_levels.size() - m_inv.size(), false);

    m_n = int(s.lookup("n")) - 1;

    const auto &c = s.lookup("color");
    if (c.isAggregate()) {
        std::transform(c.begin(), c.end(), std::back_inserter(m_colors),
                       [](const auto &arg){ return rgb_from_setting(arg); });
    } else {
        m_colors.push_back(rgb_from_setting(c));
    }
}

void indicator::update()
{
    std::visit([this](auto arg) { m_val = *arg; }, m_p);

    if (has_total) {
        if (m_total_p.index() != INT || std::get<INT>(m_total_p) != nullptr) {
            std::visit([this](auto arg) { m_total_val = *arg; }, m_total_p);
        }
        if (!m_levels_p.empty()) {
            std::transform(m_levels_p.begin(), m_levels_p.end(), m_levels.begin(),
                           [this](auto arg){ return double(m_total_val * arg); });
        }
    }
}

bool indicator::is_on() const
{
    bool b = false;
    int n = 0;

    if (m_p.index() == BOOL) {
        b = std::get<BOOL>(m_val);
    } else {
        auto p = std::visit([this](auto arg) -> auto
        {
            return std::upper_bound(m_levels.cbegin(), m_levels.cend(), arg);
        }, m_val);

        if (p != m_levels.cbegin()) {
            n = std::distance(m_levels.cbegin(), std::prev(p));
            b = true;
        }
    }

    return m_inv.at(n)? !b : b;
}

RGB indicator::color() const
{
    if (!is_multicolor() || m_p.index() == BOOL) {
        // I can't quite imagine what a "multi-color" bool is, so let's leave it at this
        return m_colors[0];
    } else {
        auto p = std::visit([this](auto arg) -> auto {
            return std::upper_bound(m_levels.cbegin(), m_levels.cend(), arg);
        }, m_val);

        if (p == m_levels.cbegin()) {
            return RGB::black; // ?
        } else {
            auto n = std::distance(m_levels.cbegin(), std::prev(p));
            return ((unsigned long int)n < m_colors.size())? m_colors.at(n) : m_colors.back();
        }
    }
}
