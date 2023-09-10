/* Copyright (c) 2023 Pirmin Pfeifer */
#include "rayvis-utils/MathTypes.h"

namespace color {
    typedef std::function<Float3(float value)> ColorMapFunc;

    struct Iterator final {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = const Float3;
        using pointer           = const Float3*;  // or also value_type*
        using reference         = const Float3&;

        Iterator(const std::vector<Float3>& colors) : colors(colors) {}

        const Float3& operator*() const;

        const Float3* operator->();

        // Prefix increment
        Iterator& operator++();

        // Postfix increment
        Iterator operator++(int);

        Iterator operator+(int n) const;

        friend bool operator==(const Iterator& a, const Iterator& b);

        friend bool operator!=(const Iterator& a, const Iterator& b);

    private:
        size_t                     i = 0;
        const std::vector<Float3>& colors;
    };

    Iterator DefaultPalettIterator();
    Float3   GetDefaultPalett(size_t i);

    Float3 Plasma(float value);
}  // namespace color