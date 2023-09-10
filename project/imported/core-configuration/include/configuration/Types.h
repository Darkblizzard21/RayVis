#pragma once

// Types, concepts & helpers for configuration

#include <core-utils/EnumUtils.h>
#include <core-utils/GlmInclude.h>
#include <core-utils/TypeUtils.h>

#include <type_traits>
#include <unordered_set>
#include <variant>
#include <vector>

namespace core {

    using ConfigurationValue = std::variant<bool,
                                            std::int32_t,
                                            float,
                                            std::string,
                                            glm::vec2,
                                            glm::vec3,
                                            glm::vec4,
                                            std::unordered_set<std::string>>;

    template <typename T>
    concept ConfigurationVariantContains = VariantContains<T, ConfigurationValue>;

    template <typename T>
    concept ConfigurationSupported =
        ConfigurationVariantContains<T> || StringConvertibleEnum<T> || StringConvertibleFlags<T>;

    class ConfigurationValidationExeption : public std::runtime_error {
    public:
        ConfigurationValidationExeption(const std::string& message);
    };

}  // namespace core

template <>
struct std::formatter<core::ConfigurationValue> : std::formatter<std::string_view> {
    auto format(const core::ConfigurationValue& value, std::format_context& ctx)
    {
        return std::visit(
            [&](const auto& val) { return std::formatter<std::string_view>::format(std::format("{}", val), ctx); },
            value);
    }
};