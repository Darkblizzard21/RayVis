#pragma once

#include <core-utils/TypeUtils.h>

#include <cassert>
#include <concepts>
#include <numeric>
#include <type_traits>
#include <vector>

namespace core {

    template <typename T>
    concept Flag = std::unsigned_integral<T> ||
                   (std::is_enum_v<T> && std::unsigned_integral<std::underlying_type_t<T>>);

    template <typename T>
    constexpr inline std::underlying_type_t<T> ToUnderlying(const T value)
    {
        return static_cast<std::underlying_type_t<T>>(value);
    }

    template <Flag T>
    inline T SeparateLowestBit(T& flagOrNumber)
    {
        if (flagOrNumber == static_cast<T>(0)) {
            return static_cast<T>(0);
        }

        const T result = static_cast<T>(1 << std::countr_zero(ToUnderlying(flagOrNumber)));
        flagOrNumber   = static_cast<T>(ToUnderlying(flagOrNumber) ^ ToUnderlying(result));

        return result;
    }

    template <Flag T>
    inline constexpr int CountSetBits(const T flag)
    {
        return std::popcount(ToUnderlying(flag));
    }

    template <Flag T>
    inline constexpr T InvertFlag(const T flag)
    {
        return T(~ToUnderlying(flag));
    }

    template <Flag T>
    inline std::size_t HashFlag(const T flag)
    {
        return std::hash<std::underlying_type_t<T>>{}(ToUnderlying(flag));
    }

    template <Flag T>
    inline void SetFlag(T& flagField, const T flag)
    {
        flagField = static_cast<T>(ToUnderlying(flagField) | ToUnderlying(flag));
    }

    template <Flag T>
    inline void ClearFlag(T& flagField, const T flag)
    {
        flagField = static_cast<T>(ToUnderlying(flagField) & ~ToUnderlying(flag));
    }

    template <Flag T>
    inline constexpr bool IsFlagSet(const T flagField, const T flag)
    {
        assert(std::has_single_bit(ToUnderlying(flag)));
        return ToUnderlying(flagField) & ToUnderlying(flag);
    }

    template <Flag T>
    inline constexpr bool AreAllFlagsSet(const T flagField, const T flags)
    {
        const auto setFlags = ToUnderlying(flagField) & ToUnderlying(flags);
        return setFlags == ToUnderlying(flags);
    }

    template <Flag T>
    inline constexpr bool IsAnyFlagSet(const T flagField, const T flags)
    {
        return ToUnderlying(flagField) & ToUnderlying(flags);
    }

    template <Flag T>
    inline void SetOrClearFlag(T& flagField, const T flag, const bool value)
    {
        if (value) {
            SetFlag(flagField, flag);
        } else {
            ClearFlag(flagField, flag);
        }
    }

    template <Flag T>
    inline constexpr T CombineFlags(const T l, const T r)
    {
        return static_cast<T>(ToUnderlying(l) | ToUnderlying(r));
    }

    // Variadic templates to combine arbitrary amount of flags.
    template <Flag T, Flag... Ts>
    inline constexpr T CombineFlags(const T l, const T r, const Ts... ts)
    {
        return CombineFlags(CombineFlags(l, r), ts...);
    }

    // Returns a list of all bits that are set in a flag
    // e.g. 0b0110 -> [0b0010, 0b0100]
    // result is in ascending order
    template <Flag T>
    std::vector<T> GetFlagBits(const T flags)
    {
        const auto     bitCount = CountSetBits(flags);
        std::vector<T> result;
        result.reserve(bitCount);

        // Iterate over all set bits
        auto tmp = flags;
        while (tmp != static_cast<T>(0)) {
            result.push_back(SeparateLowestBit(tmp));
        }
        return result;
    }

}  // namespace core