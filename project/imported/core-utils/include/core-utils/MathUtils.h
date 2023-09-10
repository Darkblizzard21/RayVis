#pragma once

#include <core-utils/Exceptions.h>

#include <concepts>
#include <format>
#include <limits>

namespace core {

    inline std::size_t CombineHash(const std::size_t left)
    {
        return left;
    }

    template <typename T>
    inline std::size_t CombineHash(const std::size_t left, const T& right)
    {
        return left ^ std::hash<T>{}(right) + 0x9e3779b9 + (left << 6) + (left >> 2);
    }

    template <typename T, typename... R>
    inline std::size_t CombineHash(const std::size_t left, const T& right, R&&... remaining)
    {
        const auto result = CombineHash(left, right);
        return CombineHash(result, std::forward<R>(remaining)...);
    }

    template <typename T, typename... R>
    inline std::size_t Hash(const T& t, R&&... remaining)
    {
        auto hash = std::hash<T>{}(t);
        return CombineHash(hash, std::forward<R>(remaining)...);
    }

    template <std::integral T, std::integral U>
    inline constexpr auto RoundToNextMultiple(const T value, const U multiple) -> std::common_type_t<T, U>
    {
        return ((value + multiple - 1) / multiple) * multiple;
    }

    template <std::integral T>
    inline constexpr auto DivideAndRoundUp(const T dividend, const T divisor)
    {
        return (dividend + divisor - 1) / divisor;
    }

    template <typename T>
    inline constexpr auto SizeInDWords()
    {
        return DivideAndRoundUp(sizeof(T), sizeof(std::uint32_t));
    }

    template <std::integral T>
    inline constexpr auto DivideAndRound(const T dividend, const T divisor)
    {
        return (dividend + (divisor / 2)) / divisor;
    }

    template <std::signed_integral T>
    inline constexpr auto PositiveModulo(T a, T b)
    {
        return (a % b + b) % b;
    }

    template <std::unsigned_integral T>
    inline constexpr auto GetAlignmentPadding(T size, T alignment)
    {
        return (alignment - (size % alignment)) % alignment;
    }

    // Performs type cast from unsigned type to signed type and checks if type cast leads to overflow
    template <std::integral DstType, std::unsigned_integral SrcType>
    inline constexpr auto SafeSignedCast(const SrcType v)
    {
        constexpr auto max = std::numeric_limits<DstType>::max();
        if (static_cast<SrcType>(max) < v) {
            throw core::InvalidArgumentException(
                std::format("Cannot perform safe type cast from {0} to {1}. value exceeds value range of {1}",
                            typeid(SrcType).name(),
                            typeid(DstType).name()));
        }
        return static_cast<DstType>(v);
    }

}  // namespace core