#pragma once

#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace core {

    std::wstring s2ws(std::string_view str);
    std::string  ws2s(std::wstring_view str);

    std::size_t GetPrefixLength(std::string_view a, std::string_view b);

    // String helper for unordered map.
    // Allows to have an unordered map with a sting key and use a string_view to search inside the unordered map
    // See https://abseil.io/tips/144
    struct StringHash {
        using is_transparent = void;

        auto operator()(std::string_view str) const noexcept
        {
            return std::hash<std::string_view>()(str);
        }
    };

    // String helper for unordered map.
    // Allows to have an unordered map with a sting key and use a string_view to search inside the unordered map
    // See https://abseil.io/tips/144
    struct StringEqual {
        using is_transparent = void;

        bool operator()(std::string_view l, std::string_view r) const noexcept
        {
            return l == r;
        }
    };

    template <std::input_iterator It>
    std::string Join(const It begin, const It end, const std::string_view join)
    {
        if (begin == end) {
            return "";
        }

        std::string result = std::format("{}", *begin);

        auto iterator = begin;
        for (++iterator; iterator != end; ++iterator) {
            result += std::format("{}{}", join, *iterator);
        }

        return result;
    }

    auto Split(const std::string_view s, const char delimiter)
    {
        return s | std::views::split(delimiter) |
               std::views::transform([](const auto rng) { return std::string_view(rng.begin(), rng.end()); });
    }

    std::string FormatTime(double time);
    std::string FormatCount(double count);
    std::string FormatByteSize(double size);

}  // namespace core

template <typename T>
struct std::formatter<std::vector<T>> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto pos = ctx.begin();
        while (pos != ctx.end() && *pos != '}') {
            separator_ += *pos;
            ++pos;
        }

        if (separator_.empty()) {
            separator_ = ",";
        }

        return pos;
    }

    auto format(const std::vector<T>& vec, std::format_context& context)
    {
        return std::format_to(context.out(), "{}", core::Join(vec.begin(), vec.end(), separator_));
    }

private:
    std::string separator_;
};

template <typename T>
struct std::formatter<std::unordered_set<T>> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto pos = ctx.begin();
        while (pos != ctx.end() && *pos != '}') {
            separator_ += *pos;
            ++pos;
        }

        if (separator_.empty()) {
            separator_ = ",";
        }

        return pos;
    }

    auto format(const std::unordered_set<T>& vec, std::format_context& context)
    {
        return std::format_to(context.out(), "{}", core::Join(vec.begin(), vec.end(), separator_));
    }

private:
    std::string separator_;
};