#pragma once

#include <core-utils/FlagUtils.h>
#include <core-utils/StringUtils.h>

#include <cassert>
#include <format>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace core {

    template <typename T>
    concept Enum = std::is_enum_v<T>;

    template <typename T>
    concept UnsignedEnum = Enum<T> && std::is_unsigned_v<std::underlying_type_t<T>>;

    template <Enum T>
    struct EnumMapping {
        T           value;
        std::string name;
        std::string displayName;
    };

    enum class EnumDefinitionType { None, Enum, Flags };

    template <typename T>
    struct EnumDefinition {
        static constexpr EnumDefinitionType type = EnumDefinitionType::None;

        static std::span<const EnumMapping<T>> GetMapping()
            requires(Enum<T>)
        {
        }
    };

    template <typename T>
    // Concept for all enum classes which can be convertes to and from strings
    concept StringConvertibleEnumType = Enum<T> && (EnumDefinition<T>::type != EnumDefinitionType::None);

    template <typename T>
    // Concept for all enums (explicitily not flags) which can be convertes to and from strings
    concept StringConvertibleEnum = Enum<T> && (EnumDefinition<T>::type == EnumDefinitionType::Enum);

    template <typename T>
    // Concept for all flags (explicitily not enums) which can be convertes to and from strings
    concept StringConvertibleFlags = UnsignedEnum<T> && StringConvertibleEnumType<T> &&
                                     (EnumDefinition<T>::type == EnumDefinitionType::Flags);

    template <StringConvertibleEnumType T>
    struct EnumHelper {
        static std::string ToString(const T value)
        {
            static const auto map = GenerateEnumToStringMap();

            if constexpr (EnumDefinition<T>::type == EnumDefinitionType::Flags) {
                if (CountSetBits(value) > 1) {
                    throw std::runtime_error("Cannot convert flag value with more one bit set");
                }
            }

            if (const auto it = map.find(value); it != map.end()) {
                return it->second;
            }

            throw std::runtime_error(std::format(
                "Could not convert enum {} with value {:d} to string", typeid(T).name(), ToUnderlying(value)));
        }

        static std::string ToDisplayString(const T value)
        {
            static const auto map = GenerateEnumToDisplayStringMap();

            if constexpr (EnumDefinition<T>::type == EnumDefinitionType::Flags) {
                if (CountSetBits(value) > 1) {
                    throw std::runtime_error("Cannot convert flag value with more one bit set");
                }
            }

            if (const auto it = map.find(value); it != map.end()) {
                return it->second;
            }

            throw std::runtime_error(std::format(
                "Could not convert enum {} with value {:d} to display string", typeid(T).name(), ToUnderlying(value)));
        }

        static T FromString(const std::string_view name)
        {
            static const auto map = GenerateStringToEnumMap();

            if (const auto it = map.find(name); it != map.end()) {
                return it->second;
            }

            throw std::runtime_error(std::format("Could not find enum {} for string {}", typeid(T).name(), name));
        }

        // Converts flag field to list of names
        static std::vector<std::string> FlagsToString(const T value)
            requires(StringConvertibleFlags<T>)
        {
            std::vector<std::string> result;

            result.reserve(CountSetBits(value));

            for (const auto bit : GetFlagBits(value)) {
                result.push_back(ToString(bit));
            }

            return result;
        }

        // Converts flag field to set of names
        static std::unordered_set<std::string> FlagsToStringSet(const T value)
            requires(StringConvertibleFlags<T>)
        {
            std::unordered_set<std::string> result;

            result.reserve(CountSetBits(value));

            for (const auto bit : GetFlagBits(value)) {
                result.insert(ToString(bit));
            }

            return result;
        }

        // Converts flag field to list of display names
        static std::vector<std::string> FlagsToDisplayString(const T value)
            requires(StringConvertibleFlags<T>)
        {
            std::vector<std::string> result;

            result.reserve(CountSetBits(value));

            for (const auto bit : GetFlagBits(value)) {
                result.push_back(ToDisplayString(bit));
            }

            return result;
        }

        // Converts list of enum names to flag field
        static T FlagsFromString(const std::span<const std::string> names)
            requires(StringConvertibleFlags<T>)
        {
            T result = static_cast<T>(0);

            for (const auto& name : names) {
                SetFlag(result, FromString(name));
            }

            return result;
        }

        // Converts set of enum names to flag field
        static T FlagsFromString(const std::unordered_set<std::string>& names)
            requires(StringConvertibleFlags<T>)
        {
            T result = static_cast<T>(0);

            for (const auto& name : names) {
                SetFlag(result, FromString(name));
            }

            return result;
        }

        static std::span<const T> Enumerate()
        {
            static const auto list = GenerateEnumerationList();

            return list;
        }

    private:
        static std::unordered_map<T, std::string> GenerateEnumToStringMap()
        {
            const auto mapping = EnumDefinition<T>::GetMapping();

            std::unordered_map<T, std::string> enumToStringMap;

            for (const auto& entry : mapping) {
                const auto& [_, inserted] = enumToStringMap.emplace(entry.value, entry.name);

                // check to validate insert was successful
                // validates mapping only contains unique keys
                assert(inserted);
            }

            return enumToStringMap;
        }

        static std::unordered_map<T, std::string> GenerateEnumToDisplayStringMap()
        {
            const auto mapping = EnumDefinition<T>::GetMapping();

            std::unordered_map<T, std::string> enumToDisplayStringMap;

            for (const auto& entry : mapping) {
                const auto& [_, inserted] = enumToDisplayStringMap.emplace(entry.value, entry.displayName);

                // check to validate insert was successful
                // validates mapping only contains unique keys
                assert(inserted);
            }

            return enumToDisplayStringMap;
        }

        static std::unordered_map<std::string, T, StringHash, StringEqual> GenerateStringToEnumMap()
        {
            const auto mapping = EnumDefinition<T>::GetMapping();

            std::unordered_map<std::string, T, StringHash, StringEqual> stringToEnumMap;

            for (const auto& entry : mapping) {
                const auto& [_, inserted] = stringToEnumMap.emplace(entry.name, entry.value);

                // check to validate insert was successful
                // validates mapping only contains unique keys
                assert(inserted);
            }

            return stringToEnumMap;
        }

        static std::vector<T> GenerateEnumerationList()
        {
            const auto mapping = EnumDefinition<T>::GetMapping();

            std::vector<T> list;
            list.reserve(mapping.size());

            for (const auto& entry : mapping) {
                list.push_back(entry.value);
            }

            return list;
        }
    };

}  // namespace core

// Format helpers for string convertible enums
template <core::StringConvertibleEnum T>
struct std::formatter<T> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto pos = ctx.begin();
        while (pos != ctx.end() && *pos != '}') {
            if (*pos == 'd' || *pos == 'D') {
                displayString_ = true;
            }
            ++pos;
        }
        return pos;
    }

    auto format(const T value, std::format_context& context)
    {
        if (displayString_) {
            return std::format_to(context.out(), "{}", core::EnumHelper<T>::ToDisplayString(value));
        } else {
            return std::format_to(context.out(), "{}", core::EnumHelper<T>::ToString(value));
        }
    }

private:
    bool displayString_ = false;
};

template <core::StringConvertibleFlags T>
struct std::formatter<T> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto pos = ctx.begin();
        while (pos != ctx.end() && *pos != '}') {
            if (*pos == 'd' || *pos == 'D') {
                displayString_ = true;
            }
            ++pos;
        }
        return pos;
    }

    auto format(const T value, std::format_context& context)
    {
        if (displayString_) {
            const auto values = core::EnumHelper<T>::FlagsToDisplayString(value);
            return std::format_to(context.out(), "{}", core::Join(values.begin(), values.end(), ", "));
        } else {
            const auto values = core::EnumHelper<T>::FlagsToString(value);
            return std::format_to(context.out(), "{}", core::Join(values.begin(), values.end(), ","));
        }
    }

private:
    bool displayString_ = false;
};