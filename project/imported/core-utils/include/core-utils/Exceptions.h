#pragma once

#include <source_location>
#include <stdexcept>
#include <string>

namespace core {

    class CoreException : public std::runtime_error {
    public:
        CoreException(std::string message, std::source_location sourceLocation = std::source_location::current());

        std::string GetFullMessage() const;

        const std::source_location& GetSourceLocation() const;

    private:
        std::source_location sourceLocation_;
    };

    class BackendException : public CoreException {
    public:
        BackendException(std::string_view     message,
                         std::source_location sourceLocation = std::source_location::current());
    };

    class InvalidArgumentException : public CoreException {
    public:
        InvalidArgumentException(const std::string_view message,
                                 std::source_location   sourceLocation = std::source_location::current());
    };

    class IOException : public CoreException {
    public:
        IOException(const std::string_view message,
                    std::source_location   sourceLocation = std::source_location::current());
    };

    class NotImplementedException : public CoreException {
    public:
        NotImplementedException(const std::string_view message        = "",
                                std::source_location   sourceLocation = std::source_location::current());
    };

    class OutOfRangeException : public CoreException {
    public:
        OutOfRangeException(const std::string_view message,
                            std::source_location   sourceLocation = std::source_location::current());
    };

}  // namespace core