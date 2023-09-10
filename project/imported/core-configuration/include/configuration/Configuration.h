#pragma once

#include <core-utils/Exceptions.h>
#include <core-utils/StringUtils.h>

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <variant>

#include "Entry.h"
#include "Types.h"

namespace core {

    class IConfiguration {
    public:
        enum class OverwritePolicy {
            // Never overwrite old entries when registering new ones.
            // Will throw if old entry exists.
            Never,
            // Overwrites registered value if old values exists.
            // Throws if old values is not valid.
            KeepValue,
            // Overwrites registered value if old values exists and is valid.
            KeepValidValue,
            // Always overwrites old entries with new values.
            Always
        };

        using AliasMap = std::unordered_map<std::string, std::string, StringHash, StringEqual>;

        virtual ~IConfiguration() = default;

        // Registers new configuration entry.
        void Register(std::string_view     key,
                      ConfigurationEntry&& entry,
                      OverwritePolicy      overwritePolicy = OverwritePolicy::KeepValue);

        // Register an entry of type Boolean
        void Register(std::string_view                      key,
                      bool                                  value,
                      std::string_view                      name,
                      std::string_view                      description     = "",
                      ConfigurationEntry::BooleanParameters parameters      = {},
                      bool                                  readOnly        = false,
                      OverwritePolicy                       overwritePolicy = OverwritePolicy::KeepValue);
        // Register an entry of type Integer
        void Register(std::string_view                               key,
                      std::int32_t                                   value,
                      std::string_view                               name,
                      std::string_view                               description,
                      ConfigurationEntry::IntParameters              parameters,
                      std::span<const ConfigurationEntry::Validator> validators      = {},
                      bool                                           readOnly        = false,
                      OverwritePolicy                                overwritePolicy = OverwritePolicy::KeepValue);
        // Register an entry of type Float
        void Register(std::string_view                               key,
                      float                                          value,
                      std::string_view                               name,
                      std::string_view                               description,
                      ConfigurationEntry::FloatParameters            parameters,
                      std::span<const ConfigurationEntry::Validator> validators      = {},
                      bool                                           readOnly        = false,
                      OverwritePolicy                                overwritePolicy = OverwritePolicy::KeepValue);
        // Register an entry of type String
        void Register(std::string_view                               key,
                      std::string_view                               value,
                      std::string_view                               name,
                      std::string_view                               description,
                      std::span<const ConfigurationEntry::Validator> validators      = {},
                      bool                                           readOnly        = false,
                      OverwritePolicy                                overwritePolicy = OverwritePolicy::KeepValue);
        void RegisterFile(
            std::string_view                               key,
            std::filesystem::path                          value,
            std::string_view                               name,
            std::string_view                               description,
            ConfigurationEntry::FileParameters             parameters,
            std::span<const ConfigurationEntry::Validator> validators = ConfigurationEntry::Validator::NoValidation,
            bool                                           readOnly   = false,
            OverwritePolicy                                overwritePolicy = OverwritePolicy::KeepValue);
        void RegisterDirectory(std::string_view                               key,
                               std::filesystem::path                          value,
                               std::string_view                               name,
                               std::string_view                               description,
                               std::span<const ConfigurationEntry::Validator> validators = {},
                               bool                                           readOnly   = false,
                               OverwritePolicy overwritePolicy                           = OverwritePolicy::KeepValue);
        // Register an entry of type Vec2
        void Register(std::string_view                               key,
                      const glm::vec2&                               value,
                      std::string_view                               name,
                      std::string_view                               description,
                      ConfigurationEntry::FloatParameters            parameters,
                      std::span<const ConfigurationEntry::Validator> validators      = {},
                      bool                                           readOnly        = false,
                      OverwritePolicy                                overwritePolicy = OverwritePolicy::KeepValue);
        // Register an entry of type Vec3
        void Register(std::string_view                               key,
                      const glm::vec3&                               value,
                      std::string_view                               name,
                      std::string_view                               description,
                      ConfigurationEntry::FloatParameters            parameters,
                      std::span<const ConfigurationEntry::Validator> validators      = {},
                      bool                                           readOnly        = false,
                      OverwritePolicy                                overwritePolicy = OverwritePolicy::KeepValue);
        // Register an entry of type Vec4
        void Register(std::string_view                               key,
                      const glm::vec4&                               value,
                      std::string_view                               name,
                      std::string_view                               description,
                      ConfigurationEntry::FloatParameters            parameters,
                      std::span<const ConfigurationEntry::Validator> validators      = {},
                      bool                                           readOnly        = false,
                      OverwritePolicy                                overwritePolicy = OverwritePolicy::KeepValue);
        // Register an entry of type Color
        void RegisterColor(std::string_view                               key,
                           const glm::vec4&                               value,
                           std::string_view                               name,
                           std::string_view                               description,
                           std::span<const ConfigurationEntry::Validator> validators      = {},
                           bool                                           readOnly        = false,
                           OverwritePolicy                                overwritePolicy = OverwritePolicy::KeepValue);
        // Register an entry of type Enum
        void RegisterEnum(std::string_view                               key,
                          std::string_view                               value,
                          std::string_view                               name,
                          std::string_view                               description,
                          ConfigurationEntry::EnumParameters             parameters,
                          std::span<const ConfigurationEntry::Validator> validators      = {},
                          bool                                           readOnly        = false,
                          OverwritePolicy                                overwritePolicy = OverwritePolicy::KeepValue);

        // Register an entry of type Enum
        template <StringConvertibleEnum T>
        void Register(std::string_view                   key,
                      const T                            value,
                      std::string_view                   name,
                      std::string_view                   description,
                      ConfigurationEntry::EnumParameters parameters = ConfigurationEntry::EnumParameters::Create<T>(),
                      std::span<const ConfigurationEntry::Validator> validators      = {},
                      bool                                           readOnly        = false,
                      OverwritePolicy                                overwritePolicy = OverwritePolicy::KeepValue)
        {
            Register(key,
                     ConfigurationEntry(EnumHelper<T>::ToString(value),
                                        ConfigurationEntry::Type::Enum,
                                        readOnly,
                                        name,
                                        description,
                                        std::move(parameters),
                                        validators),
                     overwritePolicy);
        }

        // Register an entry of type Flags
        template <StringConvertibleFlags T>
        void Register(std::string_view                   key,
                      const T                            value,
                      std::string_view                   name,
                      std::string_view                   description,
                      ConfigurationEntry::EnumParameters parameters = ConfigurationEntry::EnumParameters::Create<T>(),
                      std::span<const ConfigurationEntry::Validator> validators      = {},
                      bool                                           readOnly        = false,
                      OverwritePolicy                                overwritePolicy = OverwritePolicy::KeepValue)
        {
            Register(key,
                     ConfigurationEntry(EnumHelper<T>::FlagsToStringSet(value),
                                        ConfigurationEntry::Type::Flags,
                                        readOnly,
                                        name,
                                        description,
                                        std::move(parameters),
                                        validators),
                     overwritePolicy);
        }

        // Delete values and additional infomation from configuration
        void Delete(std::string_view key);

        // Returns whether configuration entry with key exists
        bool HasEntry(std::string_view key) const;

        bool IsEntryModified(std::string_view key) const;
        bool IsAnyEntryModified(std::span<const std::string> keys) const;
        bool IsAnyEntryModified(std::initializer_list<const std::string_view> keys) const;

        // Resolved absolute key in parent configuration
        std::string                     ResolveKey(std::string_view key) const;
        // Create sub-view of configuration for prefix.
        // Returned configuration will reference this configuration for values.
        // Returned configuration can only access key beginning with prefix.
        // Entries registered in returned configuration will contain prefix.
        std::unique_ptr<IConfiguration> CreateView(std::string_view prefix, const AliasMap& aliases = {});

        ConfigurationEntry&       GetEntry(std::string_view     key,
                                           std::source_location sourceLocation = std::source_location::current());
        const ConfigurationEntry& GetEntry(std::string_view     key,
                                           std::source_location sourceLocation = std::source_location::current()) const;

        template <ConfigurationVariantContains T>
        const T& Get(std::string_view key, std::source_location sourceLocation = std::source_location::current()) const;

        template <StringConvertibleEnumType T>
        T Get(std::string_view key, std::source_location sourceLocation = std::source_location::current()) const;

        const ConfigurationValue& GetValue(std::string_view     key,
                                           std::source_location sourceLocation = std::source_location::current()) const;

        template <ConfigurationVariantContains T>
        void Set(std::string_view     key,
                 const T&             value,
                 std::source_location sourceLocation = std::source_location::current());

        template <StringConvertibleEnumType T>
        void Set(std::string_view key, T value, std::source_location sourceLocation = std::source_location::current());

        void Set(std::string_view key, std::string_view value);

        void SetValue(std::string_view     key,
                      ConfigurationValue   value,
                      std::source_location sourceLocation = std::source_location::current());

    private:
        virtual void                      RegisterImpl(std::string_view     key,
                                                       ConfigurationEntry&& entry,
                                                       OverwritePolicy      overwritePolicy)                           = 0;
        virtual void                      DeleteImpl(std::string_view key)                                        = 0;
        virtual bool                      HasEntryImpl(std::string_view key) const                                = 0;
        virtual ConfigurationEntry&       GetEntryImpl(std::string_view key, std::source_location sourceLocation) = 0;
        virtual const ConfigurationEntry& GetEntryImpl(std::string_view     key,
                                                       std::source_location sourceLocation) const                 = 0;
        virtual void                      SetValueImpl(std::string_view     key,
                                                       ConfigurationValue   value,
                                                       std::source_location sourceLocation)                       = 0;
        virtual std::string               ResolveKeyImpl(std::string_view key) const                              = 0;
    };

    class Configuration : public IConfiguration {
    public:
        // Reset modified state for all entries
        void ResetModified();
        // Reset modified state for single entry
        void ResetModified(std::string_view key);

        void           LoadJson(const std::filesystem::path& jsonFilePath);
        void           LoadJson(const nlohmann::json& json);
        nlohmann::json ToJson() const;

    private:
        void LoadJson(std::string_view prefix, const nlohmann::json& json);

        void RegisterImpl(std::string_view key, ConfigurationEntry&& entry, OverwritePolicy overwritePolicy) override;
        void DeleteImpl(std::string_view key) override;
        bool HasEntryImpl(std::string_view key) const override;
        ConfigurationEntry&       GetEntryImpl(std::string_view key, std::source_location sourceLocation) override;
        const ConfigurationEntry& GetEntryImpl(std::string_view     key,
                                               std::source_location sourceLocation) const override;
        void SetValueImpl(std::string_view key, ConfigurationValue value, std::source_location sourceLocation) override;
        std::string ResolveKeyImpl(std::string_view key) const override;

        std::unordered_map<std::string, ConfigurationEntry, StringHash, StringEqual> values_;
    };

    class ConfigurationView : public IConfiguration {
    public:
        // aliases map local key (without prefix) to key in parent configuration
        ConfigurationView(IConfiguration* configuration, std::string_view prefix, const AliasMap& aliases = {});

        std::string_view GetPrefix() const;

    private:
        void RegisterImpl(std::string_view key, ConfigurationEntry&& entry, OverwritePolicy overwritePolicy) override;
        void DeleteImpl(std::string_view key) override;
        bool HasEntryImpl(std::string_view key) const override;
        ConfigurationEntry&       GetEntryImpl(std::string_view key, std::source_location sourceLocation) override;
        const ConfigurationEntry& GetEntryImpl(std::string_view     key,
                                               std::source_location sourceLocation) const override;
        void SetValueImpl(std::string_view key, ConfigurationValue value, std::source_location sourceLocation) override;
        std::string ResolveKeyImpl(std::string_view key) const override;

        IConfiguration* config_;
        std::string     prefix_;
        AliasMap        aliases_;
    };

    class IConfigurationComponent {
    public:
        virtual ~IConfigurationComponent() = default;

        void            SetConfiguration(std::unique_ptr<IConfiguration>&& configuration);
        IConfiguration& GetConfiguration();

        // Returns list of configuation keys, that can be modified or rendered in the user interface.
        // Returned list may depend on current state and may not contain all configuration keys of the component.
        virtual std::vector<std::string> GetAvailableConfigurationKeys() const;

    private:
        virtual void            SetConfigurationImpl(std::unique_ptr<IConfiguration>&& configuration) = 0;
        virtual IConfiguration& GetConfigurationImpl()                                                = 0;
    };

    // ===============================================================================
    // Implementation IConfiguration

    template <ConfigurationVariantContains T>
    inline const T& core::IConfiguration::Get(const std::string_view     key,
                                              const std::source_location sourceLocation) const
    {
        return GetEntry(key, sourceLocation).Get<T>(sourceLocation);
    }

    template <StringConvertibleEnumType T>
    inline T core::IConfiguration::Get(const std::string_view key, const std::source_location sourceLocation) const
    {
        return GetEntry(key, sourceLocation).Get<T>(sourceLocation);
    }

    template <ConfigurationVariantContains T>
    inline void IConfiguration::Set(const std::string_view     key,
                                    const T&                   value,
                                    const std::source_location sourceLocation)
    {
        SetValue(key, value, sourceLocation);
    }

    template <StringConvertibleEnumType T>
    inline void IConfiguration::Set(const std::string_view key, T value, const std::source_location sourceLocation)
    {
        if constexpr (StringConvertibleEnum<T>) {
            SetValue(key, EnumHelper<T>::ToString(value), sourceLocation);
        } else if constexpr (StringConvertibleFlags<T>) {
            SetValue(key, EnumHelper<T>::FlagsToStringSet(value), sourceLocation);
        } else {
            throw InvalidArgumentException(std::format("type \"{}\" is not supported.", typeid(T).name()),
                                           sourceLocation);
        }
    }

}  // namespace core