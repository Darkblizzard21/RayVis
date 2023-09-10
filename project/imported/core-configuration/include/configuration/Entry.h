#pragma once

#include <core-utils/Exceptions.h>

#include <filesystem>
#include <functional>
#include <optional>

#include "Types.h"

namespace core {

    class ConfigurationEntry {
    public:
        // Type of configuration entry. Used for rendering UI elements
        enum class Type {
            Unknown,
            Boolean,
            Integer,
            Float,
            String,
            File,
            Directory,
            Vec2,
            Vec3,
            Vec4,
            Color,
            Enum,
            Flags
        };

        struct BooleanParameters {
            enum class DisplayMode { Checkbox, Button };

            DisplayMode displayMode = DisplayMode::Checkbox;
        };

        // Defines range of UI slider element.
        struct IntParameters {
            std::int32_t min;
            std::int32_t max;
        };

        // Defines range of UI slider element.
        struct FloatParameters {
            float       min;
            float       max;
            // Enable non-linear slider
            bool        logarithmic         = false;
            // Format determines display value and slider resolution.
            std::string format              = "%0.3f";
            // Enable option to normalize vector types
            bool        vectorNormalizeable = false;
        };

        // Defines file dialog settings
        struct FileParameters {
            // filterList syntax:
            //   - ";" begin a new filter
            //   - "," separator type to filter
            // e.g. "png,jpg;dds"
            //
            // A wildcard filter is automatically added to every dialog
            std::string filterList;
        };

        // Defines possible values for user interface
        struct EnumParameters {
            enum class DisplayMode { Dropdown, List };

            std::vector<std::string>                     values;
            std::unordered_map<std::string, std::string> displayNames;
            DisplayMode                                  displayMode = DisplayMode::Dropdown;

            template <StringConvertibleEnumType T>
            static EnumParameters Create(DisplayMode displayMode = DisplayMode::Dropdown);

            static EnumParameters FromPairs(std::span<const std::pair<std::string, std::string>> pairs,
                                            DisplayMode displayMode = DisplayMode::Dropdown);
        };

        using Parameters = std::
            variant<std::monostate, BooleanParameters, IntParameters, FloatParameters, FileParameters, EnumParameters>;

        struct Validator {
            // Callback for validation. Throws ConfigurationValidationExeption if value is invalid.
            std::function<void(const ConfigurationValue&)> callback;
            // Description of validator. Should describe valid values.
            std::string                                    description;

            // Empty validator, which accepts all values
            static const Validator Empty;
            // Validator for positive float values
            static const Validator PositiveFloat;
            // Validator for positive int values
            static const Validator PositiveInt;

            // Validator for existing paths (files & directories allowed)
            static const Validator ExistingPath;
            // Validator for existing files (no directories allowed)
            static const Validator ExistingFile;
            // Validator for existing directories (no files allowed)
            static const Validator ExistingDirectory;

            // Returns span containing Empty validator
            static std::span<const Validator> NoValidation;
        };

        ConfigurationEntry(ConfigurationValue value);
        ConfigurationEntry(ConfigurationValue value, std::span<const Validator> validators);
        ConfigurationEntry(ConfigurationValue         value,
                           Type                       type,
                           bool                       readOnly,
                           std::string_view           name,
                           std::string_view           description,
                           Parameters                 parameters,
                           std::span<const Validator> validators);

        template <ConfigurationVariantContains T>
        const T& Get(std::source_location sourceLocation = std::source_location::current()) const;

        template <StringConvertibleEnumType T>
        T Get(std::source_location sourceLocation = std::source_location::current()) const;

        const ConfigurationValue& GetValue() const;

        template <ConfigurationVariantContains T>
        void Set(const T& value, std::source_location sourceLocation = std::source_location::current());

        template <StringConvertibleEnumType T>
        void Set(T value, std::source_location sourceLocation = std::source_location::current());

        void SetValue(ConfigurationValue value, std::source_location sourceLocation = std::source_location::current());

        Type              GetType() const;
        bool              IsModified() const;
        void              ResetModified();
        void              RevertModification();
        bool              IsReadOnly() const;
        std::string_view  GetName() const;
        std::string_view  GetDescription() const;
        const Parameters& GetParameters() const;

        // Create an entry of type Boolean
        static ConfigurationEntry Create(bool              value,
                                         std::string_view  name,
                                         std::string_view  description = "",
                                         BooleanParameters parameters  = {},
                                         bool              readOnly    = false);
        // Create an entry of type Integer
        static ConfigurationEntry Create(std::int32_t               value,
                                         std::string_view           name,
                                         std::string_view           description,
                                         IntParameters              parameters,
                                         std::span<const Validator> validators = {},
                                         bool                       readOnly   = false);
        // Create an entry of type Float
        static ConfigurationEntry Create(float                      value,
                                         std::string_view           name,
                                         std::string_view           description,
                                         FloatParameters            parameters,
                                         std::span<const Validator> validators = {},
                                         bool                       readOnly   = false);
        // Create an entry of type String
        static ConfigurationEntry Create(std::string_view           value,
                                         std::string_view           name,
                                         std::string_view           description,
                                         std::span<const Validator> validators = {},
                                         bool                       readOnly   = false);
        // Create an entry of type File
        static ConfigurationEntry CreateFile(std::filesystem::path      value,
                                             std::string_view           name,
                                             std::string_view           description,
                                             FileParameters             parameters,
                                             std::span<const Validator> validators = Validator::NoValidation,
                                             bool                       readOnly   = false);
        // Create an entry of type File
        static ConfigurationEntry CreateDirectory(std::filesystem::path      value,
                                                  std::string_view           name,
                                                  std::string_view           description,
                                                  std::span<const Validator> validators = {},
                                                  bool                       readOnly   = false);
        // Create an entry of type Vec2
        static ConfigurationEntry Create(const glm::vec2&           value,
                                         std::string_view           name,
                                         std::string_view           description,
                                         FloatParameters            parameters,
                                         std::span<const Validator> validators = {},
                                         bool                       readOnly   = false);
        // Create an entry of type Vec3
        static ConfigurationEntry Create(const glm::vec3&           value,
                                         std::string_view           name,
                                         std::string_view           description,
                                         FloatParameters            parameters,
                                         std::span<const Validator> validators = {},
                                         bool                       readOnly   = false);
        // Create an entry of type Vec4
        static ConfigurationEntry Create(const glm::vec4&           value,
                                         std::string_view           name,
                                         std::string_view           description,
                                         FloatParameters            parameters,
                                         std::span<const Validator> validators = {},
                                         bool                       readOnly   = false);
        // Create an entry of type Color
        static ConfigurationEntry CreateColor(const glm::vec4&           value,
                                              std::string_view           name,
                                              std::string_view           description,
                                              std::span<const Validator> validators = {},
                                              bool                       readOnly   = false);
        // Create an entry of type Enum
        static ConfigurationEntry CreateEnum(std::string_view           value,
                                             std::string_view           name,
                                             std::string_view           description,
                                             EnumParameters             parameters,
                                             std::span<const Validator> validators = {},
                                             bool                       readOnly   = false);

        // Create an entry of type Enum
        template <StringConvertibleEnum T>
        static ConfigurationEntry Create(const T                    value,
                                         std::string_view           name,
                                         std::string_view           description,
                                         EnumParameters             parameters = EnumParameters::Create<T>(),
                                         std::span<const Validator> validators = {},
                                         bool                       readOnly   = false)
        {
            return ConfigurationEntry(EnumHelper<T>::ToString(value),
                                      Type::Enum,
                                      readOnly,
                                      name,
                                      description,
                                      std::move(parameters),
                                      validators);
        }

        // Create an entry of type Flags
        template <StringConvertibleFlags T>
        static ConfigurationEntry Create(const T                    value,
                                         std::string_view           name,
                                         std::string_view           description,
                                         EnumParameters             parameters = EnumParameters::Create<T>(),
                                         std::span<const Validator> validators = {},
                                         bool                       readOnly   = false)
        {
            return ConfigurationEntry(EnumHelper<T>::FlagsToStringSet(value),
                                      Type::Flags,
                                      readOnly,
                                      name,
                                      description,
                                      std::move(parameters),
                                      validators);
        }

    private:
        friend class Configuration;

        void ValidateAndSet(ConfigurationValue   value,
                            std::source_location sourceLocation = std::source_location::current());

        ConfigurationValue                value_;
        std::optional<ConfigurationValue> previousValue_;
        Type                              type_     = Type::Unknown;
        bool                              readOnly_ = false;
        std::string                       name_;
        std::string                       description_;
        Parameters                        parameters_;
        std::vector<Validator>            validators_;
    };

    // ===============================================================================
    // Implementation ConfigurationEntry

    template <ConfigurationVariantContains T>
    inline const T& ConfigurationEntry::Get(const std::source_location sourceLocation) const
    {
        if (std::holds_alternative<T>(value_)) {
            return std::get<T>(value_);
        }

        throw InvalidArgumentException(
            std::format("Cannot get configuration entry as type \"{}\". Entry has type \"{}\".",
                        typeid(T).name(),
                        std::visit([](const auto& arg) { return typeid(arg).name(); }, value_)),
            sourceLocation);
    }

    template <StringConvertibleEnumType T>
    inline T ConfigurationEntry::Get(const std::source_location sourceLocation) const
    {
        if constexpr (StringConvertibleEnum<T>) {
            if (std::holds_alternative<std::string>(value_)) {
                try {
                    return EnumHelper<T>::FromString(std::get<std::string>(value_));
                } catch (const InvalidArgumentException& e) {
                    throw InvalidArgumentException(e.what(), sourceLocation);
                }
            }

            throw InvalidArgumentException(
                std::format("Cannot get configuration entry as type \"{}\". Entry has type \"{}\".",
                            typeid(T).name(),
                            std::visit([](const auto& arg) { return typeid(arg).name(); }, value_)),
                sourceLocation);
        } else if constexpr (StringConvertibleFlags<T>) {
            if (std::holds_alternative<std::unordered_set<std::string>>(value_)) {
                try {
                    return EnumHelper<T>::FlagsFromString(std::get<std::unordered_set<std::string>>(value_));
                } catch (const InvalidArgumentException& e) {
                    throw InvalidArgumentException(e.what(), sourceLocation);
                }
            }

            throw InvalidArgumentException(
                std::format("Cannot get configuration entry as type \"{}\". Entry has type \"{}\".",
                            typeid(T).name(),
                            std::visit([](const auto& arg) { return typeid(arg).name(); }, value_)),
                sourceLocation);
        } else {
            throw InvalidArgumentException(std::format("type \"{}\" is not supported.", typeid(T).name()),
                                           sourceLocation);
        }
    }

    template <ConfigurationVariantContains T>
    inline void ConfigurationEntry::Set(const T& value, const std::source_location sourceLocation)
    {
        SetValue(value, sourceLocation);
    }

    template <StringConvertibleEnumType T>
    inline void ConfigurationEntry::Set(T value, const std::source_location sourceLocation)
    {
        if constexpr (StringConvertibleEnum<T>) {
            SetValue(EnumHelper<T>::ToString(value), sourceLocation);
        } else if constexpr (StringConvertibleFlags<T>) {
            SetValue(EnumHelper<T>::FlagsToStringSet(value), sourceLocation);
        } else {
            throw InvalidArgumentException(std::format("type \"{}\" is not supported.", typeid(T).name()),
                                           sourceLocation);
        }
    }

    template <StringConvertibleEnumType T>
    inline ConfigurationEntry::EnumParameters ConfigurationEntry::EnumParameters::Create(const DisplayMode displayMode)
    {
        EnumParameters result;
        result.displayMode = displayMode;

        const auto enumValues = EnumHelper<T>::Enumerate();

        result.values.reserve(enumValues.size());
        result.displayNames.reserve(enumValues.size());

        for (const auto value : enumValues) {
            result.values.push_back(EnumHelper<T>::ToString(value));
            result.displayNames.emplace(EnumHelper<T>::ToString(value), EnumHelper<T>::ToDisplayString(value));
        }

        return result;
    }

}  // namespace core