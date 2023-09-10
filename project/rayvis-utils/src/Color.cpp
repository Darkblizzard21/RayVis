/* Copyright (c) 2023 Pirmin Pfeifer */
#include "Color.h"

namespace {
    std::vector<Float3> DEFAULT_PALLET = {Float3(0.12, 0.47, 0.71),
                                          Float3(1, 0.5, 0.05),
                                          Float3(0.17, 0.63, 0.17),
                                          Float3(0.58, 0.4, 0.74),
                                          Float3(0.55, 0.34, 0.29),
                                          Float3(0.89, 0.47, 0.76),
                                          Float3(0.74, 0.74, 0.13),
                                          Float3(0.09, 0.75, 0.81),
                                          Float3(1, 0.5, 0.05)};

    std::vector<Float3> PLASMA_PALLET_10 = {Float3(0.05, 0.03, 0.53),
                                            Float3(0.27, 0.01, 0.62),
                                            Float3(0.45, 0, 0.66),
                                            Float3(0.61, 0.09, 0.62),
                                            Float3(0.74, 0.21, 0.52),
                                            Float3(0.84, 0.34, 0.42),
                                            Float3(0.93, 0.47, 0.33),
                                            Float3(0.98, 0.62, 0.23),
                                            Float3(0.99, 0.78, 0.15),
                                            Float3(0.94, 0.97, 0.13)};
}  // namespace

namespace color {
    const Float3& Iterator::operator*() const
    {
        return colors[i];
    }

    const Float3* Iterator::operator->()
    {
        return &colors[i];
    }

    Iterator& Iterator::operator++()
    {
        i = (i + 1) % colors.size();
        return *this;
    }

    Iterator Iterator::operator++(int)
    {
        Iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    Iterator Iterator::operator+(int n) const
    {
        auto itr = Iterator(colors);
        itr.i    = (i + n) % colors.size();
        return itr;
    }

    bool operator==(const Iterator& a, const Iterator& b)
    {
        return a.i == b.i && (a.colors == b.colors);
    }

    bool operator!=(const Iterator& a, const Iterator& b)
    {
        return a.i != b.i || (a.colors != b.colors);
    }

    Iterator DefaultPalettIterator()
    {
        return Iterator(DEFAULT_PALLET);
    }

    Float3 GetDefaultPalett(size_t i)
    {
        return DEFAULT_PALLET[i % DEFAULT_PALLET.size()];
    }

    Float3 Plasma(float value)
    {
        value = std::clamp(value, 0.f, 1.f);

        float scale = value * (PLASMA_PALLET_10.size() - 1);
        float lower = floor(scale);
        float upper = ceil(scale);
        float lerpV = scale - lower;
        return lerp(PLASMA_PALLET_10[lower], PLASMA_PALLET_10[upper], lerpV);
    }
}  // namespace color