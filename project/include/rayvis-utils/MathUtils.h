/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once

#include <concepts>
#include <type_traits>

template <std::integral T, std::integral U>
inline constexpr auto RoundToNextMultiple(const T value, const U multiple) -> std::common_type_t<T, U>
{
    return ((value + multiple - 1) / multiple) * multiple;
}

template <typename E>
constexpr auto to_integral(const E e) -> typename std::underlying_type<E>::type
{
    return static_cast<typename std::underlying_type<E>::type>(e);
}

template <typename E>
constexpr bool is_flag_set(const E flag, const E enumValue)
{
    return (to_integral(flag) & to_integral(enumValue)) == to_integral(flag);
}

template <typename E>
std::string display_name(E e);


template<size_t power>
constexpr size_t const_pow(size_t number)
{
    return number * const_pow<power - 1>(number);
}

template<>
constexpr size_t const_pow<1>(size_t number)
{
    return number;
}

template <>
constexpr size_t const_pow<0>(size_t number)
{
    return 1;
}


constexpr size_t square(size_t n)
{
    return const_pow<2>(n);
}

constexpr size_t cube(size_t n)
{
    return const_pow<3>(n);
}