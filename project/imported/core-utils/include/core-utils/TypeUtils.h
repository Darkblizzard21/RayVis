#pragma once

#include <concepts>
#include <type_traits>
#include <variant>

namespace core {

    template <typename T, class Hash = std::hash<T>, class Comparator = std::equal_to<T>>
    concept HashMapKey = requires(T a, T b) {
                             {
                                 Hash{}(a)
                                 } -> std::convertible_to<std::size_t>;
                             {
                                 Comparator{}(a, b)
                                 } -> std::same_as<bool>;
                         };

    // Helper concept to determine if a type is included in a variant.
    // Recursive concepts do not work anymore, so we specifically test the first 64 types
    template <typename T, typename Variant>
    concept VariantContains =
        (std::variant_size_v<Variant> > 0 && std::same_as<T, std::variant_alternative_t<0, Variant>>) ||
        (std::variant_size_v<Variant> > 1 && std::same_as<T, std::variant_alternative_t<1, Variant>>) ||
        (std::variant_size_v<Variant> > 2 && std::same_as<T, std::variant_alternative_t<2, Variant>>) ||
        (std::variant_size_v<Variant> > 3 && std::same_as<T, std::variant_alternative_t<3, Variant>>) ||
        (std::variant_size_v<Variant> > 4 && std::same_as<T, std::variant_alternative_t<4, Variant>>) ||
        (std::variant_size_v<Variant> > 5 && std::same_as<T, std::variant_alternative_t<5, Variant>>) ||
        (std::variant_size_v<Variant> > 6 && std::same_as<T, std::variant_alternative_t<6, Variant>>) ||
        (std::variant_size_v<Variant> > 7 && std::same_as<T, std::variant_alternative_t<7, Variant>>) ||
        (std::variant_size_v<Variant> > 8 && std::same_as<T, std::variant_alternative_t<8, Variant>>) ||
        (std::variant_size_v<Variant> > 9 && std::same_as<T, std::variant_alternative_t<9, Variant>>) ||
        (std::variant_size_v<Variant> > 10 && std::same_as<T, std::variant_alternative_t<10, Variant>>) ||
        (std::variant_size_v<Variant> > 11 && std::same_as<T, std::variant_alternative_t<11, Variant>>) ||
        (std::variant_size_v<Variant> > 12 && std::same_as<T, std::variant_alternative_t<12, Variant>>) ||
        (std::variant_size_v<Variant> > 13 && std::same_as<T, std::variant_alternative_t<13, Variant>>) ||
        (std::variant_size_v<Variant> > 14 && std::same_as<T, std::variant_alternative_t<14, Variant>>) ||
        (std::variant_size_v<Variant> > 15 && std::same_as<T, std::variant_alternative_t<15, Variant>>) ||
        (std::variant_size_v<Variant> > 16 && std::same_as<T, std::variant_alternative_t<16, Variant>>) ||
        (std::variant_size_v<Variant> > 17 && std::same_as<T, std::variant_alternative_t<17, Variant>>) ||
        (std::variant_size_v<Variant> > 18 && std::same_as<T, std::variant_alternative_t<18, Variant>>) ||
        (std::variant_size_v<Variant> > 19 && std::same_as<T, std::variant_alternative_t<19, Variant>>) ||
        (std::variant_size_v<Variant> > 20 && std::same_as<T, std::variant_alternative_t<20, Variant>>) ||
        (std::variant_size_v<Variant> > 21 && std::same_as<T, std::variant_alternative_t<21, Variant>>) ||
        (std::variant_size_v<Variant> > 22 && std::same_as<T, std::variant_alternative_t<22, Variant>>) ||
        (std::variant_size_v<Variant> > 23 && std::same_as<T, std::variant_alternative_t<23, Variant>>) ||
        (std::variant_size_v<Variant> > 24 && std::same_as<T, std::variant_alternative_t<24, Variant>>) ||
        (std::variant_size_v<Variant> > 25 && std::same_as<T, std::variant_alternative_t<25, Variant>>) ||
        (std::variant_size_v<Variant> > 26 && std::same_as<T, std::variant_alternative_t<26, Variant>>) ||
        (std::variant_size_v<Variant> > 27 && std::same_as<T, std::variant_alternative_t<27, Variant>>) ||
        (std::variant_size_v<Variant> > 28 && std::same_as<T, std::variant_alternative_t<28, Variant>>) ||
        (std::variant_size_v<Variant> > 29 && std::same_as<T, std::variant_alternative_t<29, Variant>>) ||
        (std::variant_size_v<Variant> > 30 && std::same_as<T, std::variant_alternative_t<30, Variant>>) ||
        (std::variant_size_v<Variant> > 31 && std::same_as<T, std::variant_alternative_t<31, Variant>>) ||
        (std::variant_size_v<Variant> > 32 && std::same_as<T, std::variant_alternative_t<32, Variant>>) ||
        (std::variant_size_v<Variant> > 33 && std::same_as<T, std::variant_alternative_t<33, Variant>>) ||
        (std::variant_size_v<Variant> > 34 && std::same_as<T, std::variant_alternative_t<34, Variant>>) ||
        (std::variant_size_v<Variant> > 35 && std::same_as<T, std::variant_alternative_t<35, Variant>>) ||
        (std::variant_size_v<Variant> > 36 && std::same_as<T, std::variant_alternative_t<36, Variant>>) ||
        (std::variant_size_v<Variant> > 37 && std::same_as<T, std::variant_alternative_t<37, Variant>>) ||
        (std::variant_size_v<Variant> > 38 && std::same_as<T, std::variant_alternative_t<38, Variant>>) ||
        (std::variant_size_v<Variant> > 39 && std::same_as<T, std::variant_alternative_t<39, Variant>>) ||
        (std::variant_size_v<Variant> > 40 && std::same_as<T, std::variant_alternative_t<40, Variant>>) ||
        (std::variant_size_v<Variant> > 41 && std::same_as<T, std::variant_alternative_t<41, Variant>>) ||
        (std::variant_size_v<Variant> > 42 && std::same_as<T, std::variant_alternative_t<42, Variant>>) ||
        (std::variant_size_v<Variant> > 43 && std::same_as<T, std::variant_alternative_t<43, Variant>>) ||
        (std::variant_size_v<Variant> > 44 && std::same_as<T, std::variant_alternative_t<44, Variant>>) ||
        (std::variant_size_v<Variant> > 45 && std::same_as<T, std::variant_alternative_t<45, Variant>>) ||
        (std::variant_size_v<Variant> > 46 && std::same_as<T, std::variant_alternative_t<46, Variant>>) ||
        (std::variant_size_v<Variant> > 47 && std::same_as<T, std::variant_alternative_t<47, Variant>>) ||
        (std::variant_size_v<Variant> > 48 && std::same_as<T, std::variant_alternative_t<48, Variant>>) ||
        (std::variant_size_v<Variant> > 49 && std::same_as<T, std::variant_alternative_t<49, Variant>>) ||
        (std::variant_size_v<Variant> > 50 && std::same_as<T, std::variant_alternative_t<50, Variant>>) ||
        (std::variant_size_v<Variant> > 51 && std::same_as<T, std::variant_alternative_t<51, Variant>>) ||
        (std::variant_size_v<Variant> > 52 && std::same_as<T, std::variant_alternative_t<52, Variant>>) ||
        (std::variant_size_v<Variant> > 53 && std::same_as<T, std::variant_alternative_t<53, Variant>>) ||
        (std::variant_size_v<Variant> > 54 && std::same_as<T, std::variant_alternative_t<54, Variant>>) ||
        (std::variant_size_v<Variant> > 55 && std::same_as<T, std::variant_alternative_t<55, Variant>>) ||
        (std::variant_size_v<Variant> > 56 && std::same_as<T, std::variant_alternative_t<56, Variant>>) ||
        (std::variant_size_v<Variant> > 57 && std::same_as<T, std::variant_alternative_t<57, Variant>>) ||
        (std::variant_size_v<Variant> > 58 && std::same_as<T, std::variant_alternative_t<58, Variant>>) ||
        (std::variant_size_v<Variant> > 59 && std::same_as<T, std::variant_alternative_t<59, Variant>>) ||
        (std::variant_size_v<Variant> > 60 && std::same_as<T, std::variant_alternative_t<60, Variant>>) ||
        (std::variant_size_v<Variant> > 61 && std::same_as<T, std::variant_alternative_t<61, Variant>>) ||
        (std::variant_size_v<Variant> > 62 && std::same_as<T, std::variant_alternative_t<62, Variant>>) ||
        (std::variant_size_v<Variant> > 63 && std::same_as<T, std::variant_alternative_t<63, Variant>>);

    // Magic helper struct to allow std::visit with lambdas for each type
    template <class... Ts>
    struct VisitorOverload : Ts... {
        using Ts::operator()...;
    };

}  // namespace core