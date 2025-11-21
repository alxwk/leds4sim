#ifndef RGB_H
#define RGB_H

#include <tuple>
#include <string>

class RGB {
public:
    using value_type = std::tuple<unsigned int, unsigned int, unsigned int>;

    RGB(unsigned int r, unsigned int g, unsigned int b) : m_r(r), m_g(g), m_b(b) {}

    [[nodiscard]] value_type rgb() const
    {
    	return {m_r, m_g, m_b};
    }

    value_type operator()() const {
	return rgb();
    }

    static RGB from_int(int n);
    static RGB from_name(const std::string& s);

    const static RGB red;
    const static RGB gold;
    const static RGB yellow;
    const static RGB green;
    const static RGB blue;
    const static RGB cyan;
    const static RGB mozacyan;
    const static RGB magenta;
    const static RGB white;
    const static RGB black;

private:
    unsigned m_r = 0;
    unsigned m_g = 0;
    unsigned m_b = 0;
};

#endif // RGB_H
