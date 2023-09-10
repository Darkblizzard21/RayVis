#include "ListUtils.h"

#include <algorithm>

#include "MathUtils.h"

namespace core {

    std::size_t ByteHash::operator()(const std::span<const std::byte>& data) const
    {
        std::size_t hash = 0;
        for (const auto b : data) {
            hash = core::CombineHash(hash, b);
        }
        return hash;
    }

    std::size_t ByteEqual::operator()(const std::span<const std::byte>& a, const std::span<const std::byte>& b) const
    {
        if (a.size() != b.size()) {
            return false;
        }
        if (a.data() == b.data()) {
            return true;
        }
        return std::ranges::equal(a, b);
    }

}  // namespace core