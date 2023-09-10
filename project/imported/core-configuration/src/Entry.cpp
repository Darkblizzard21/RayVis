#include "Entry.h"

#include <filesystem>
#include <unordered_set>

namespace core {

    ConfigurationEntry::ConfigurationEntry(ConfigurationValue value) : value_(std::move(value)) {}

    ConfigurationEntry::ConfigurationEntry(ConfigurationValue value, std::span<const Validator> validators)
        : value_(std::move(value)), validators_(validators.begin(), validators.end())
    {
        for (const auto& validator : validators_) {
            if (!validator.callback) {
                throw InvalidArgumentException("validator callback cannot be invalid.");
            }

            try {
                validator.callback(value_);
            } catch (const ConfigurationValidationExeption& e) {
                throw InvalidArgumentException(
                    std::format("Cannot construct configuration entry with value \"{}\".\n"
                                "\tValidator: {}\n"
                                "\tMessage:   {}",
                                value_,
                                validator.description,
                                e.what()));
            }
        }
    }

    ConfigurationEntry::ConfigurationEntry(ConfigurationValue         value,
                                           Type                       type,
                                           bool                       readOnly,
                                           std::string_view           name,
                                           std::string_view           description,
                                           Parameters                 parameters,
                                           std::span<const Validator> validators)
        : value_(std::move(value)),
          type_(type),
          readOnly_(readOnly),
          name_(name),
          description_(description),
          parameters_(std::move(parameters)),
          validators_(validators.begin(), validators.end())
    {
        const auto valueTypeName = std::visit([](const auto& arg) { return typeid(arg).name(); }, value_);

        switch (type_) {
        case Type::Boolean:
            if (!std::holds_alternative<bool>(value_)) {
                throw InvalidArgumentException(
                    std::format("cannot create configuration entry of type \"Boolean\" that holds value of type \"{}\"",
                                valueTypeName));
            }
            if (!std::holds_alternative<BooleanParameters>(parameters_)) {
                throw InvalidArgumentException(
                    "configuration entry of type \"Boolean\" must have parameters of type BooleanParameters");
            }
            if (!validators.empty()) {
                throw InvalidArgumentException("configuration entry of type \"Boolean\" cannot have validators");
            }
            break;
        case Type::Integer:
            if (!std::holds_alternative<std::int32_t>(value_)) {
                throw InvalidArgumentException(
                    std::format("cannot create configuration entry of type \"Integer\" that holds value of type \"{}\"",
                                valueTypeName));
            }
            if (!std::holds_alternative<IntParameters>(parameters_)) {
                throw InvalidArgumentException(
                    "configuration entry of type \"Integer\" must have parameters of type IntParameters");
            }
            if (validators_.empty()) {
                const auto intParameters = std::get<IntParameters>(parameters_);
                validators_.emplace_back(
                    [intParameters](const auto& value) {
                        const auto val = std::get<int>(value);
                        if (val < intParameters.min || val > intParameters.max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                    },
                    std::format("int values in [{},{}]", intParameters.min, intParameters.max));
            }
            break;
        case Type::Float:
            if (!std::holds_alternative<float>(value_)) {
                throw InvalidArgumentException(
                    std::format("cannot create configuration entry of type \"Float\" that holds value of type \"{}\"",
                                valueTypeName));
            }
            if (!std::holds_alternative<FloatParameters>(parameters_)) {
                throw InvalidArgumentException(
                    "configuration entry of type \"Float\" must have parameters of type FloatParameters");
            }
            if (validators_.empty()) {
                const auto floatParameters = std::get<FloatParameters>(parameters_);
                validators_.emplace_back(
                    [floatParameters](const auto& value) {
                        const auto val = std::get<float>(value);
                        if (val < floatParameters.min || val > floatParameters.max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                    },
                    std::format("float values in [{},{}]", floatParameters.min, floatParameters.max));
            }
            break;
        case Type::String:
            if (!std::holds_alternative<std::string>(value_)) {
                throw InvalidArgumentException(
                    std::format("cannot create configuration entry of type \"String\" that holds value of type \"{}\"",
                                valueTypeName));
            }
            if (!std::holds_alternative<std::monostate>(parameters_)) {
                throw InvalidArgumentException(
                    "configuration entry of type \"String\" must have parameters of type std::monostate");
            }
            break;
        case Type::File:
            if (!std::holds_alternative<std::string>(value_)) {
                throw InvalidArgumentException(
                    std::format("cannot create configuration entry of type \"File\" that holds value of type \"{}\"",
                                valueTypeName));
            }
            if (!std::holds_alternative<FileParameters>(parameters_)) {
                throw InvalidArgumentException(
                    "configuration entry of type \"File\" must have parameters of type FileParameters");
            }
            if (validators_.empty()) {
                validators_.push_back(Validator::ExistingFile);
            }
            break;
        case Type::Directory:
            if (!std::holds_alternative<std::string>(value_)) {
                throw InvalidArgumentException(std::format(
                    "cannot create configuration entry of type \"Directory\" that holds value of type \"{}\"",
                    valueTypeName));
            }
            if (!std::holds_alternative<std::monostate>(parameters_)) {
                throw InvalidArgumentException(
                    "configuration entry of type \"Directory\" must have parameters of type std::monostate");
            }
            if (validators_.empty()) {
                validators_.push_back(Validator::ExistingDirectory);
            }
            break;
        case Type::Vec2:
            if (!std::holds_alternative<glm::vec2>(value_)) {
                throw InvalidArgumentException(
                    std::format("cannot create configuration entry of type \"Vec2\" that holds value of type \"{}\"",
                                valueTypeName));
            }
            if (!std::holds_alternative<FloatParameters>(parameters_)) {
                throw InvalidArgumentException(
                    "configuration entry of type \"Vec2\" must have parameters of type FloatParameters");
            }
            if (validators_.empty()) {
                const auto floatParameters = std::get<FloatParameters>(parameters_);
                validators_.emplace_back(
                    [floatParameters](const auto& value) {
                        const auto& val = std::get<glm::vec2>(value);
                        if (val.x < floatParameters.min || val.x > floatParameters.max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                        if (val.y < floatParameters.min || val.y > floatParameters.max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                    },
                    std::format("glm::vec2 values in [{},{}]", floatParameters.min, floatParameters.max));
            }
            break;
        case Type::Vec3:
            if (!std::holds_alternative<glm::vec3>(value_)) {
                throw InvalidArgumentException(
                    std::format("cannot create configuration entry of type \"Vec3\" that holds value of type \"{}\"",
                                valueTypeName));
            }
            if (!std::holds_alternative<FloatParameters>(parameters_)) {
                throw InvalidArgumentException(
                    "configuration entry of type \"Vec3\" must have parameters of type FloatParameters");
            }
            if (validators_.empty()) {
                const auto floatParameters = std::get<FloatParameters>(parameters_);
                validators_.emplace_back(
                    [floatParameters](const auto& value) {
                        const auto& val = std::get<glm::vec3>(value);
                        if (val.x < floatParameters.min || val.x > floatParameters.max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                        if (val.y < floatParameters.min || val.y > floatParameters.max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                        if (val.z < floatParameters.min || val.z > floatParameters.max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                    },
                    std::format("glm::vec3 values in [{},{}]", floatParameters.min, floatParameters.max));
            }
            break;
        case Type::Vec4:
            if (!std::holds_alternative<glm::vec4>(value_)) {
                throw InvalidArgumentException(
                    std::format("cannot create configuration entry of type \"Vec4\" that holds value of type \"{}\"",
                                valueTypeName));
            }
            if (!std::holds_alternative<FloatParameters>(parameters_)) {
                throw InvalidArgumentException(
                    "configuration entry of type \"Vec4\" must have parameters of type FloatParameters");
            }
            if (validators_.empty()) {
                const auto floatParameters = std::get<FloatParameters>(parameters_);
                validators_.emplace_back(
                    [floatParameters](const auto& value) {
                        const auto& val = std::get<glm::vec4>(value);
                        if (val.x < floatParameters.min || val.x > floatParameters.max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                        if (val.y < floatParameters.min || val.y > floatParameters.max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                        if (val.z < floatParameters.min || val.z > floatParameters.max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                        if (val.w < floatParameters.min || val.w > floatParameters.max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                    },
                    std::format("glm::vec4 values in [{},{}]", floatParameters.min, floatParameters.max));
            }
            break;
        case Type::Color:
            if (!std::holds_alternative<glm::vec4>(value_)) {
                throw InvalidArgumentException(
                    std::format("cannot create configuration entry of type \"Color\" that holds value of type \"{}\"",
                                valueTypeName));
            }
            if (!std::holds_alternative<std::monostate>(parameters_)) {
                throw InvalidArgumentException(
                    "configuration entry of type \"Color\" must have parameters of type std::monostate");
            }
            if (validators_.empty()) {
                const auto min = 0.f;
                const auto max = 1.f;
                validators_.emplace_back(
                    [min, max](const auto& value) {
                        const auto& val = std::get<glm::vec4>(value);
                        if (val.x < min || val.x > max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                        if (val.y < min || val.y > max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                        if (val.z < min || val.z > max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                        if (val.w < min || val.w > max) {
                            throw ConfigurationValidationExeption("value out of range.");
                        }
                    },
                    std::format("glm::vec4 values in [{},{}]", min, max));
            }
            break;
        case Type::Enum:
            if (!std::holds_alternative<std::string>(value_)) {
                throw InvalidArgumentException(
                    std::format("cannot create configuration entry of type \"Enum\" that holds value of type \"{}\"",
                                valueTypeName));
            }
            if (!std::holds_alternative<EnumParameters>(parameters_)) {
                throw InvalidArgumentException(
                    "configuration entry of type \"Enum\" must have parameters of type EnumParameters");
            }
            if (validators.empty()) {
                const auto                      enumParameters = std::get<EnumParameters>(parameters_);
                std::unordered_set<std::string> lookup(enumParameters.values.begin(), enumParameters.values.end());
                validators_.emplace_back(
                    [lookup](const auto& value) {
                        const auto& val = std::get<std::string>(value);
                        if (lookup.find(val) == lookup.end()) {
                            throw ConfigurationValidationExeption("invalid string value.");
                        }
                    },
                    std::format("enum values in [{:, }]", enumParameters.values));
            }
            break;
        case Type::Flags:
            if (!std::holds_alternative<std::unordered_set<std::string>>(value_)) {
                throw InvalidArgumentException(
                    std::format("cannot create configuration entry of type \"Flags\" that holds value of type \"{}\"",
                                valueTypeName));
            }
            if (!std::holds_alternative<EnumParameters>(parameters_)) {
                throw InvalidArgumentException(
                    "configuration entry of type \"Enum\" must have parameters of type EnumParameters");
            }
            if (validators.empty()) {
                const auto                      enumParameters = std::get<EnumParameters>(parameters_);
                std::unordered_set<std::string> lookup(enumParameters.values.begin(), enumParameters.values.end());
                validators_.emplace_back(
                    [lookup](const auto& value) {
                        const auto& values = std::get<std::unordered_set<std::string>>(value);
                        for (const auto& val : values) {
                            if (lookup.find(val) == lookup.end()) {
                                throw ConfigurationValidationExeption("invalid string value.");
                            }
                        }
                    },
                    std::format("enum flag values in [{:, }]", enumParameters.values));
            }
            break;
        default:
            throw InvalidArgumentException("unknown type.");
        }

        for (const auto& validator : validators_) {
            if (!validator.callback) {
                throw InvalidArgumentException("validator callback cannot be invalid.");
            }

            try {
                validator.callback(value_);
            } catch (const ConfigurationValidationExeption& e) {
                throw InvalidArgumentException(
                    std::format("Cannot construct configuration entry with value \"{}\".\n"
                                "\tValidator: {}\n"
                                "\tMessage:   {}",
                                value_,
                                validator.description,
                                e.what()));
            }
        }
    }

    const ConfigurationValue& ConfigurationEntry::GetValue() const
    {
        return value_;
    }

    void ConfigurationEntry::SetValue(ConfigurationValue value, const std::source_location sourceLocation)
    {
        if (readOnly_) {
            throw CoreException("Cannot set configuration entry value. Entry is read-only.", sourceLocation);
        }

        ValidateAndSet(std::move(value), sourceLocation);
    }

    void ConfigurationEntry::ValidateAndSet(ConfigurationValue value, std::source_location sourceLocation)
    {
        if (value.index() != value_.index()) {
            const auto GetTypeName      = [](const auto& arg) { return typeid(arg).name(); };
            const auto valueTypeName    = std::visit(GetTypeName, value_);
            const auto newValueTypeName = std::visit(GetTypeName, value);

            throw InvalidArgumentException(
                std::format("Cannot set configuration entry as type \"{}\". Internal variant holds type \"{}\".",
                            newValueTypeName,
                            valueTypeName),
                sourceLocation);
        }

        for (const auto& validator : validators_) {
            try {
                validator.callback(value);
            } catch (const ConfigurationValidationExeption& e) {
                throw InvalidArgumentException(std::format("Cannot set configuration entry to value \"{}\".\n"
                                                           "\tValidator: {}\n"
                                                           "\tMessage:   {}",
                                                           value,
                                                           validator.description,
                                                           e.what()),
                                               sourceLocation);
            }
        }

        if ((value_ != value) && !previousValue_.has_value()) {
            previousValue_.emplace(std::move(value_));
        }

        value_ = std::move(value);
    }

    ConfigurationEntry::Type ConfigurationEntry::GetType() const
    {
        return type_;
    }

    bool ConfigurationEntry::IsModified() const
    {
        return previousValue_.has_value();
    }

    void ConfigurationEntry::ResetModified()
    {
        previousValue_.reset();
    }

    void ConfigurationEntry::RevertModification()
    {
        if (!previousValue_.has_value()) {
            throw CoreException("cannot revert unmodified entry.");
        }

        value_ = std::move(*previousValue_);
    }

    bool ConfigurationEntry::IsReadOnly() const
    {
        return readOnly_;
    }

    std::string_view ConfigurationEntry::GetName() const
    {
        return name_;
    }

    std::string_view ConfigurationEntry::GetDescription() const
    {
        return description_;
    }

    const ConfigurationEntry::Parameters& ConfigurationEntry::GetParameters() const
    {
        return parameters_;
    }

    ConfigurationEntry ConfigurationEntry::Create(bool              value,
                                                  std::string_view  name,
                                                  std::string_view  description,
                                                  BooleanParameters parameters,
                                                  bool              readOnly)
    {
        return ConfigurationEntry(value, Type::Boolean, readOnly, name, description, parameters, {});
    }

    ConfigurationEntry ConfigurationEntry::Create(std::int32_t               value,
                                                  std::string_view           name,
                                                  std::string_view           description,
                                                  IntParameters              parameters,
                                                  std::span<const Validator> validators,
                                                  bool                       readOnly)
    {
        return ConfigurationEntry(value, Type::Integer, readOnly, name, description, parameters, validators);
    }

    ConfigurationEntry ConfigurationEntry::Create(float                      value,
                                                  std::string_view           name,
                                                  std::string_view           description,
                                                  FloatParameters            parameters,
                                                  std::span<const Validator> validators,
                                                  bool                       readOnly)
    {
        return ConfigurationEntry(value, Type::Float, readOnly, name, description, parameters, validators);
    }

    ConfigurationEntry ConfigurationEntry::Create(std::string_view           value,
                                                  std::string_view           name,
                                                  std::string_view           description,
                                                  std::span<const Validator> validators,
                                                  bool                       readOnly)
    {
        return ConfigurationEntry(
            std::string(value), Type::String, readOnly, name, description, std::monostate{}, validators);
    }

    ConfigurationEntry ConfigurationEntry::CreateFile(std::filesystem::path      value,
                                                      std::string_view           name,
                                                      std::string_view           description,
                                                      FileParameters             parameters,
                                                      std::span<const Validator> validators,
                                                      bool                       readOnly)
    {
        return ConfigurationEntry(
            value.generic_string(), Type::File, readOnly, name, description, std::move(parameters), validators);
    }

    ConfigurationEntry ConfigurationEntry::CreateDirectory(std::filesystem::path      value,
                                                           std::string_view           name,
                                                           std::string_view           description,
                                                           std::span<const Validator> validators,
                                                           bool                       readOnly)
    {
        return ConfigurationEntry(
            value.generic_string(), Type::Directory, readOnly, name, description, std::monostate{}, validators);
    }

    ConfigurationEntry ConfigurationEntry::Create(const glm::vec2&           value,
                                                  std::string_view           name,
                                                  std::string_view           description,
                                                  FloatParameters            parameters,
                                                  std::span<const Validator> validators,
                                                  bool                       readOnly)
    {
        return ConfigurationEntry(value, Type::Vec2, readOnly, name, description, parameters, validators);
    }

    ConfigurationEntry ConfigurationEntry::Create(const glm::vec3&           value,
                                                  std::string_view           name,
                                                  std::string_view           description,
                                                  FloatParameters            parameters,
                                                  std::span<const Validator> validators,
                                                  bool                       readOnly)
    {
        return ConfigurationEntry(value, Type::Vec3, readOnly, name, description, parameters, validators);
    }

    ConfigurationEntry ConfigurationEntry::Create(const glm::vec4&           value,
                                                  std::string_view           name,
                                                  std::string_view           description,
                                                  FloatParameters            parameters,
                                                  std::span<const Validator> validators,
                                                  bool                       readOnly)
    {
        return ConfigurationEntry(value, Type::Vec4, readOnly, name, description, parameters, validators);
    }

    ConfigurationEntry ConfigurationEntry::CreateColor(const glm::vec4&           value,
                                                       std::string_view           name,
                                                       std::string_view           description,
                                                       std::span<const Validator> validators,
                                                       bool                       readOnly)
    {
        return ConfigurationEntry(value, Type::Color, readOnly, name, description, std::monostate{}, validators);
    }

    ConfigurationEntry ConfigurationEntry::CreateEnum(std::string_view           value,
                                                      std::string_view           name,
                                                      std::string_view           description,
                                                      EnumParameters             parameters,
                                                      std::span<const Validator> validators,
                                                      bool                       readOnly)
    {
        return ConfigurationEntry(std::string(value), Type::Enum, readOnly, name, description, parameters, validators);
    }

    ConfigurationValidationExeption::ConfigurationValidationExeption(const std::string& message)
        : std::runtime_error(message)
    {
    }

    const ConfigurationEntry::Validator ConfigurationEntry::Validator::Empty = {[](const auto&) {}, "All values."};
    const ConfigurationEntry::Validator ConfigurationEntry::Validator::PositiveFloat = {
        [](const auto& value) {
            const auto val = std::get<float>(value);
            if (val < 0) {
                throw ConfigurationValidationExeption("values is less than zero.");
            }
        },
        "Positive float values."};
    const ConfigurationEntry::Validator ConfigurationEntry::Validator::PositiveInt = {
        [](const auto& value) {
            const auto val = std::get<std::int32_t>(value);
            if (val < 0) {
                throw ConfigurationValidationExeption("values is less than zero.");
            }
        },
        "Positive int values."};

    const ConfigurationEntry::Validator ConfigurationEntry::Validator::ExistingPath = {
        [](const auto& value) {
            const auto val = std::get<std::string>(value);
            if (!std::filesystem::exists(val)) {
                throw ConfigurationValidationExeption("path does not exist");
            }
        },
        "Existing paths (files & directories allowed)"};

    const ConfigurationEntry::Validator ConfigurationEntry::Validator::ExistingFile = {
        [](const auto& value) {
            const auto val = std::get<std::string>(value);
            if (!std::filesystem::exists(val)) {
                throw ConfigurationValidationExeption("file does not exist");
            }
            if (std::filesystem::is_directory(val)) {
                throw ConfigurationValidationExeption("path is not a file");
            }
        },
        "Existing files (no directories allowed)"};

    const ConfigurationEntry::Validator ConfigurationEntry::Validator::ExistingDirectory = {
        [](const auto& value) {
            const auto val = std::get<std::string>(value);
            if (!std::filesystem::exists(val)) {
                throw ConfigurationValidationExeption("path does not exist");
            }
            if (!std::filesystem::is_directory(val)) {
                throw ConfigurationValidationExeption("path is not a directory");
            }
        },
        "Existing directories (no files allowed)"};

    std::span<const ConfigurationEntry::Validator> ConfigurationEntry::Validator::NoValidation{&Empty, 1};

    ConfigurationEntry::EnumParameters ConfigurationEntry::EnumParameters::FromPairs(
        const std::span<const std::pair<std::string, std::string>> pairs,
        const DisplayMode                                          displayMode)
    {
        EnumParameters result;
        result.displayMode = displayMode;

        result.values.reserve(pairs.size());
        result.displayNames.reserve(pairs.size());

        for (const auto& [value, displayName] : pairs) {
            result.values.push_back(value);
            result.displayNames.emplace(value, displayName);
        }

        return result;
    }

}  // namespace core