#include "Configuration.h"

#include <spdlog/spdlog.h>

#include <fstream>
#undef CreateFile
#undef CreateDirectory

namespace core {

    void IConfiguration::Register(std::string_view key, ConfigurationEntry&& entry, OverwritePolicy overwritePolicy)
    {
        if (key.empty()) {
            throw InvalidArgumentException("key cannot be empty");
        }
        RegisterImpl(key, std::move(entry), overwritePolicy);
    }

    void IConfiguration::Register(const std::string_view                key,
                                  const bool                            value,
                                  const std::string_view                name,
                                  const std::string_view                description,
                                  ConfigurationEntry::BooleanParameters parameters,
                                  const bool                            readOnly,
                                  const OverwritePolicy                 overwritePolicy)
    {
        Register(key,
                 ConfigurationEntry::Create(value, name, description, std::move(parameters), readOnly),
                 overwritePolicy);
    }

    void IConfiguration::Register(const std::string_view                               key,
                                  const std::int32_t                                   value,
                                  const std::string_view                               name,
                                  const std::string_view                               description,
                                  ConfigurationEntry::IntParameters                    parameters,
                                  const std::span<const ConfigurationEntry::Validator> validators,
                                  const bool                                           readOnly,
                                  const OverwritePolicy                                overwritePolicy)
    {
        Register(key,
                 ConfigurationEntry::Create(value, name, description, std::move(parameters), validators, readOnly),
                 overwritePolicy);
    }

    void IConfiguration::Register(const std::string_view                               key,
                                  const float                                          value,
                                  const std::string_view                               name,
                                  const std::string_view                               description,
                                  ConfigurationEntry::FloatParameters                  parameters,
                                  const std::span<const ConfigurationEntry::Validator> validators,
                                  const bool                                           readOnly,
                                  const OverwritePolicy                                overwritePolicy)
    {
        Register(key,
                 ConfigurationEntry::Create(value, name, description, std::move(parameters), validators, readOnly),
                 overwritePolicy);
    }

    void IConfiguration::Register(const std::string_view                               key,
                                  const std::string_view                               value,
                                  const std::string_view                               name,
                                  const std::string_view                               description,
                                  const std::span<const ConfigurationEntry::Validator> validators,
                                  const bool                                           readOnly,
                                  const OverwritePolicy                                overwritePolicy)
    {
        Register(key, ConfigurationEntry::Create(value, name, description, validators, readOnly), overwritePolicy);
    }

    void IConfiguration::RegisterFile(const std::string_view                               key,
                                      std::filesystem::path                                value,
                                      const std::string_view                               name,
                                      const std::string_view                               description,
                                      ConfigurationEntry::FileParameters                   parameters,
                                      const std::span<const ConfigurationEntry::Validator> validators,
                                      const bool                                           readOnly,
                                      const OverwritePolicy                                overwritePolicy)
    {
        Register(key,
                 ConfigurationEntry::CreateFile(
                     std::move(value), name, description, std::move(parameters), validators, readOnly),
                 overwritePolicy);
    }

    void IConfiguration::RegisterDirectory(const std::string_view                               key,
                                           std::filesystem::path                                value,
                                           const std::string_view                               name,
                                           const std::string_view                               description,
                                           const std::span<const ConfigurationEntry::Validator> validators,
                                           const bool                                           readOnly,
                                           const OverwritePolicy                                overwritePolicy)
    {
        Register(key,
                 ConfigurationEntry::CreateDirectory(std::move(value), name, description, validators, readOnly),
                 overwritePolicy);
    }

    void IConfiguration::Register(const std::string_view                               key,
                                  const glm::vec2&                                     value,
                                  const std::string_view                               name,
                                  const std::string_view                               description,
                                  ConfigurationEntry::FloatParameters                  parameters,
                                  const std::span<const ConfigurationEntry::Validator> validators,
                                  const bool                                           readOnly,
                                  const OverwritePolicy                                overwritePolicy)
    {
        Register(key,
                 ConfigurationEntry::Create(value, name, description, std::move(parameters), validators, readOnly),
                 overwritePolicy);
    }

    void IConfiguration::Register(const std::string_view                               key,
                                  const glm::vec3&                                     value,
                                  const std::string_view                               name,
                                  const std::string_view                               description,
                                  ConfigurationEntry::FloatParameters                  parameters,
                                  const std::span<const ConfigurationEntry::Validator> validators,
                                  const bool                                           readOnly,
                                  const OverwritePolicy                                overwritePolicy)
    {
        Register(key,
                 ConfigurationEntry::Create(value, name, description, std::move(parameters), validators, readOnly),
                 overwritePolicy);
    }

    void IConfiguration::Register(const std::string_view                               key,
                                  const glm::vec4&                                     value,
                                  const std::string_view                               name,
                                  const std::string_view                               description,
                                  ConfigurationEntry::FloatParameters                  parameters,
                                  const std::span<const ConfigurationEntry::Validator> validators,
                                  const bool                                           readOnly,
                                  const OverwritePolicy                                overwritePolicy)
    {
        Register(key,
                 ConfigurationEntry::Create(value, name, description, std::move(parameters), validators, readOnly),
                 overwritePolicy);
    }

    void IConfiguration::RegisterColor(std::string_view                               key,
                                       const glm::vec4&                               value,
                                       std::string_view                               name,
                                       std::string_view                               description,
                                       std::span<const ConfigurationEntry::Validator> validators,
                                       bool                                           readOnly,
                                       OverwritePolicy                                overwritePolicy)
    {
        Register(key, ConfigurationEntry::CreateColor(value, name, description, validators, readOnly), overwritePolicy);
    }

    void IConfiguration::RegisterEnum(std::string_view                               key,
                                      std::string_view                               value,
                                      std::string_view                               name,
                                      std::string_view                               description,
                                      ConfigurationEntry::EnumParameters             parameters,
                                      std::span<const ConfigurationEntry::Validator> validators,
                                      bool                                           readOnly,
                                      OverwritePolicy                                overwritePolicy)
    {
        Register(key,
                 ConfigurationEntry::CreateEnum(value, name, description, parameters, validators, readOnly),
                 overwritePolicy);
    }

    void IConfiguration::Delete(const std::string_view key)
    {
        DeleteImpl(key);
    }

    bool IConfiguration::HasEntry(const std::string_view key) const
    {
        return HasEntryImpl(key);
    }

    bool IConfiguration::IsEntryModified(const std::string_view key) const
    {
        return GetEntry(key).IsModified();
    }

    bool IConfiguration::IsAnyEntryModified(const std::span<const std::string> keys) const
    {
        return std::any_of(keys.begin(), keys.end(), [&](const auto& key) { return IsEntryModified(key); });
    }

    bool IConfiguration::IsAnyEntryModified(const std::initializer_list<const std::string_view> keys) const
    {
        return std::any_of(keys.begin(), keys.end(), [&](const auto& key) { return IsEntryModified(key); });
    }

    std::string IConfiguration::ResolveKey(const std::string_view key) const
    {
        return ResolveKeyImpl(key);
    }

    std::unique_ptr<IConfiguration> IConfiguration::CreateView(const std::string_view prefix, const AliasMap& aliases)
    {
        return std::make_unique<ConfigurationView>(this, prefix, aliases);
    }

    ConfigurationEntry& IConfiguration::GetEntry(const std::string_view key, const std::source_location sourceLocation)
    {
        return GetEntryImpl(key, sourceLocation);
    }

    const ConfigurationEntry& IConfiguration::GetEntry(const std::string_view     key,
                                                       const std::source_location sourceLocation) const
    {
        return GetEntryImpl(key, sourceLocation);
    }

    const ConfigurationValue& IConfiguration::GetValue(const std::string_view     key,
                                                       const std::source_location sourceLocation) const
    {
        return GetEntry(key, sourceLocation).GetValue();
    }

    void IConfiguration::Set(const std::string_view key, const std::string_view value)
    {
        Set(key, std::string(value));
    }

    void IConfiguration::SetValue(const std::string_view     key,
                                  ConfigurationValue         value,
                                  const std::source_location sourceLocation)
    {
        SetValueImpl(key, std::move(value), sourceLocation);
    }

    void Configuration::ResetModified()
    {
        for (auto& [key, entry] : values_) {
            entry.ResetModified();
        }
    }

    void Configuration::ResetModified(std::string_view key)
    {
        GetEntry(key).ResetModified();
    }

    void Configuration::LoadJson(const std::filesystem::path& jsonFilePath)
    {
        std::ifstream stream(jsonFilePath);

        LoadJson(nlohmann::json::parse(stream));
    }

    void Configuration::LoadJson(const nlohmann::json& json)
    {
        if (!json.is_object()) {
            throw InvalidArgumentException("json must be of type object.");
        }

        LoadJson("", json);
    }

    nlohmann::json Configuration::ToJson() const
    {
        nlohmann::json result = nlohmann::json::object();

        for (const auto& [key, value] : values_) {
            nlohmann::json* currentObj = &result;

            {
                // split key at '.' and find matching json object
                auto objKeys = Split(key, '.');
                for (const auto& objKey : objKeys) {
                    currentObj = &((*currentObj)[std::string(objKey)]);
                }
            }

            if (!currentObj->is_null()) {
                throw InvalidArgumentException(std::format("json object with key \"{}\" already exists.", key));
            }

            std::visit(VisitorOverload{[currentObj](const bool value) { *currentObj = value; },
                                       [currentObj](const std::int32_t value) { *currentObj = value; },
                                       [currentObj](const float value) { *currentObj = value; },
                                       [currentObj](const std::string& value) { *currentObj = value; },
                                       [currentObj](const glm::vec2& value) {
                                           *currentObj = nlohmann::json::array({value.x, value.y});
                                       },
                                       [currentObj](const glm::vec3& value) {
                                           *currentObj = nlohmann::json::array({value.x, value.y, value.z});
                                       },
                                       [currentObj](const glm::vec4& value) {
                                           *currentObj = nlohmann::json::array({value.x, value.y, value.z, value.w});
                                       },
                                       [currentObj](const std::unordered_set<std::string>& value) {
                                           nlohmann::json::array_t arr;
                                           for (const auto& val : value) {
                                               arr.push_back(val);
                                           }
                                           *currentObj = arr;
                                       }},
                       value.GetValue());
        }  // namespace core

        return result;
    }

    void Configuration::LoadJson(std::string_view prefix, const nlohmann::json& json)
    {
        if (json.is_object()) {
            for (auto it = json.begin(); it != json.end(); ++it) {
                if (prefix.empty()) {
                    LoadJson(it.key(), *it);
                } else {
                    LoadJson(std::format("{}.{}", prefix, it.key()), *it);
                }
            }
        } else if (json.is_array()) {
            if (std::all_of(json.begin(), json.end(), [](const auto& elem) { return elem.is_string(); })) {
                std::unordered_set<std::string> value;
                for (const auto& elem : json) {
                    value.insert(elem.get<std::string>());
                }
                SetValue(prefix, value);
            } else if (std::all_of(json.begin(), json.end(), [](const auto& elem) { return elem.is_number(); })) {
                glm::vec4 value;
                if (json.size() >= 2) {
                    json.at(0).get_to(value.x);
                    json.at(1).get_to(value.y);
                    if (json.size() == 2) {
                        Set<glm::vec2>(prefix, value);
                        return;
                    }
                }
                if (json.size() >= 3) {
                    json.at(2).get_to(value.z);
                    if (json.size() == 3) {
                        Set<glm::vec3>(prefix, value);
                        return;
                    }
                }
                if (json.size() >= 4) {
                    json.at(3).get_to(value.w);
                    if (json.size() == 4) {
                        Set<glm::vec4>(prefix, value);
                        return;
                    }
                }
                throw InvalidArgumentException(std::format("json array \"{}\" has invalid size.", prefix));
            } else {
                throw InvalidArgumentException(std::format("json array \"{}\" contains invalid elements.", prefix));
            }
        } else if (json.is_boolean()) {
            Set(prefix, json.get<bool>());
        } else if (json.is_number_float()) {
            Set(prefix, json.get<float>());
        } else if (json.is_number_integer()) {
            Set(prefix, json.get<std::int32_t>());
        } else if (json.is_string()) {
            try {
                Set(prefix, json.get<std::string>());
            } catch (CoreException& _) {
                spdlog::warn("JSON-LOAD: Value was not set because of (validation) error ({}, {})", prefix, json.get<std::string>());
            }
        } else {
            throw InvalidArgumentException(std::format("json object \"{}\" has invalid type.", prefix));
        }
    }

    void Configuration::RegisterImpl(const std::string_view key,
                                     ConfigurationEntry&&   entry,
                                     const OverwritePolicy  overwritePolicy)
    {
        const auto it = values_.find(key);

        if ((it == values_.end()) || overwritePolicy == OverwritePolicy::Always) {
            Delete(key);

            values_.emplace(key, std::move(entry));
            return;
        }

        if (overwritePolicy == OverwritePolicy::Never) {
            throw InvalidArgumentException(
                std::format("configuration entry with key \"{}\" already exists and overwrite is disabled.", key));
        }

        const auto previousValue = it->second.GetValue();

        try {
            entry.ValidateAndSet(previousValue);
        } catch (const InvalidArgumentException& e) {
            if (overwritePolicy == OverwritePolicy::KeepValue) {
                throw InvalidArgumentException(
                    std::format("configuration entry with key \"{}\" already exists and previous values is not "
                                "compatible with new validation:\n{}",
                                key,
                                e.what()));
            } else {
                spdlog::warn(
                    "configuration entry with key \"{}\" already exists and previous values is not "
                    "compatible with new validation:\n{}\nignoring old value.",
                    key,
                    e.what());
            }
        }

        it->second = std::move(entry);
    }

    void Configuration::DeleteImpl(const std::string_view key)
    {
        if (const auto it = values_.find(key); it != values_.end()) {
            values_.erase(it);
        }
    }

    bool Configuration::HasEntryImpl(const std::string_view key) const
    {
        return values_.find(key) != values_.end();
    }

    const ConfigurationEntry& Configuration::GetEntryImpl(const std::string_view     key,
                                                          const std::source_location sourceLocation) const
    {
        if (const auto it = values_.find(key); it != values_.end()) {
            return it->second;
        }
        throw InvalidArgumentException(std::format("configuration does not contain an entry with key \"{}\"", key),
                                       sourceLocation);
    }

    ConfigurationEntry& Configuration::GetEntryImpl(const std::string_view key, std::source_location sourceLocation)
    {
        if (const auto it = values_.find(key); it != values_.end()) {
            return it->second;
        }
        throw InvalidArgumentException(std::format("configuration does not contain an entry with key \"{}\"", key),
                                       sourceLocation);
    }

    void Configuration::SetValueImpl(const std::string_view     key,
                                     ConfigurationValue         value,
                                     const std::source_location sourceLocation)
    {
        if (HasEntry(key)) {
            GetEntry(key, sourceLocation).SetValue(value, sourceLocation);
        } else {
            values_.emplace(key, std::move(value));
        }
    }

    std::string Configuration::ResolveKeyImpl(std::string_view key) const
    {
        return std::string(key);
    }

    ConfigurationView::ConfigurationView(IConfiguration*  configuration,
                                         std::string_view prefix,
                                         const AliasMap&  aliases)
        : config_(configuration), prefix_(prefix), aliases_(aliases)
    {
        if (configuration == nullptr) {
            throw InvalidArgumentException("configuration cannot be nullptr.");
        }

        for (const auto& [alias, key] : aliases_) {
            if (configuration->HasEntry(prefix_ + alias)) {
                throw InvalidArgumentException(std::format("alias \"{}\" -> \"{}\" hides configuration entry \"{}\"",
                                                           alias,
                                                           key,
                                                           configuration->ResolveKey(prefix_ + alias)));
            }
        }
    }

    std::string_view ConfigurationView::GetPrefix() const
    {
        return prefix_;
    }

    void ConfigurationView::RegisterImpl(std::string_view     key,
                                         ConfigurationEntry&& entry,
                                         OverwritePolicy      overwritePolicy)
    {
        if (const auto it = aliases_.find(key); it != aliases_.end()) {
            config_->Register(it->second, std::move(entry), overwritePolicy);
        } else {
            config_->Register(prefix_ + std::string(key), std::move(entry), overwritePolicy);
        }
    }

    void ConfigurationView::DeleteImpl(std::string_view key)
    {
        if (const auto it = aliases_.find(key); it != aliases_.end()) {
            config_->Delete(it->second);
        } else {
            config_->Delete(prefix_ + std::string(key));
        }
    }

    bool ConfigurationView::HasEntryImpl(std::string_view key) const
    {
        if (const auto it = aliases_.find(key); it != aliases_.end()) {
            return config_->HasEntry(it->second);
        } else {
            return config_->HasEntry(prefix_ + std::string(key));
        }
    }

    ConfigurationEntry& ConfigurationView::GetEntryImpl(std::string_view key, std::source_location sourceLocation)
    {
        if (const auto it = aliases_.find(key); it != aliases_.end()) {
            return config_->GetEntry(it->second, sourceLocation);
        } else {
            return config_->GetEntry(prefix_ + std::string(key), sourceLocation);
        }
    }

    const ConfigurationEntry& ConfigurationView::GetEntryImpl(std::string_view     key,
                                                              std::source_location sourceLocation) const
    {
        if (const auto it = aliases_.find(key); it != aliases_.end()) {
            return config_->GetEntry(it->second, sourceLocation);
        } else {
            return config_->GetEntry(prefix_ + std::string(key), sourceLocation);
        }
    }

    void ConfigurationView::SetValueImpl(std::string_view     key,
                                         ConfigurationValue   value,
                                         std::source_location sourceLocation)
    {
        if (const auto it = aliases_.find(key); it != aliases_.end()) {
            config_->SetValue(it->second, std::move(value), sourceLocation);
        } else {
            config_->SetValue(prefix_ + std::string(key), std::move(value), sourceLocation);
        }
    }

    std::string ConfigurationView::ResolveKeyImpl(std::string_view key) const
    {
        if (const auto it = aliases_.find(key); it != aliases_.end()) {
            return config_->ResolveKey(it->second);
        } else {
            return config_->ResolveKey(prefix_ + std::string(key));
        }
    }

    void IConfigurationComponent::SetConfiguration(std::unique_ptr<IConfiguration>&& configuration)
    {
        if (!configuration) {
            throw InvalidArgumentException("configuration cannot be nullptr.");
        }
        SetConfigurationImpl(std::move(configuration));
    }

    IConfiguration& IConfigurationComponent::GetConfiguration()
    {
        return GetConfigurationImpl();
    }

    std::vector<std::string> IConfigurationComponent::GetAvailableConfigurationKeys() const
    {
        return std::vector<std::string>();
    }

}  // namespace core
