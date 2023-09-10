#include "StringUtils.h"

#include <xlocbuf>

namespace core {

    std::wstring s2ws(const std::string_view str)
    {
        std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>, wchar_t> converter;

        const auto begin = &*str.cbegin();
        const auto end   = begin + str.size();

        return converter.from_bytes(begin, end);
    }

    std::string ws2s(const std::wstring_view str)
    {
        std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>, wchar_t> converter;

        const auto begin = &*str.cbegin();
        const auto end   = begin + str.size();

        return converter.to_bytes(begin, end);
    }

    std::size_t GetPrefixLength(const std::string_view a, const std::string_view b)
    {
        std::size_t prefixLength = 0;

        auto itA = a.begin();
        auto itB = b.begin();

        while ((itA != a.end()) && (itB != b.end())) {
            if (*itA != *itB) {
                return prefixLength;
            }

            ++prefixLength;
            ++itA;
            ++itB;
        }

        return prefixLength;
    }

    std::string FormatTime(const double time)
    {
        if (time < 1e-4) {
            // TODO: return micro-seconds with U+03BC
            return std::format("{:.4f}ms", time * 1e3);
        } else if (time < 1.0) {
            return std::format("{:.2f}ms", time * 1e3);
        } else {
            return std::format("{:.2f}s", time);
        }
    }

    std::string FormatCount(const double count)
    {
        if (count < 1e3) {
            return std::format("{:.2f}", count);
        } else if (count < 1e6) {
            return std::format("{:.2f} K", count / 1e3);
        } else if (count < 1e9) {
            return std::format("{:.2f} M", count / 1e6);
        } else {
            return std::format("{:.2f} B", count / 1e9);
        }
    }

    std::string FormatByteSize(const double size)
    {
        constexpr size_t kibAmount = (1 << 10);
        constexpr size_t mibAmount = (kibAmount << 10);
        constexpr size_t gibAmount = (mibAmount << 10);

        if (size < kibAmount) {
            return std::format("{:L} B", size);
        } else if (size < mibAmount) {
            return std::format("{:.2f} KiB", size / kibAmount);
        } else if (size < gibAmount) {
            return std::format("{:.2f} MiB", size / mibAmount);
        } else {
            return std::format("{:.2f} GiB", size / gibAmount);
        }
    }

}  // namespace core