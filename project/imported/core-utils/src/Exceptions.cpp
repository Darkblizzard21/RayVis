#include "Exceptions.h"

#include <spdlog/spdlog.h>

#include <format>

namespace core {
    CoreException::CoreException(const std::string message, const std::source_location sourceLocation)
        : std::runtime_error(message), sourceLocation_(sourceLocation)
    {
        spdlog::trace("Exception thrown: {}", GetFullMessage());
    }

    std::string CoreException::GetFullMessage() const
    {
        return std::format(
            "{}\n\n"
            "Function: {}\n"
            "Location: {}:{}\n"
            "File:     {}",
            what(),
            sourceLocation_.function_name(),
            sourceLocation_.line(),
            sourceLocation_.column(),
            sourceLocation_.file_name());
    }

    const std::source_location& CoreException::GetSourceLocation() const
    {
        return sourceLocation_;
    }

    BackendException::BackendException(const std::string_view message, const std::source_location sourceLocation)
        : CoreException(std::format("BackendException: {}", message), sourceLocation)
    {
    }

    InvalidArgumentException::InvalidArgumentException(const std::string_view     message,
                                                       const std::source_location sourceLocation)
        : CoreException(std::format("InvalidArgumentException: {}", message), sourceLocation)
    {
    }

    IOException::IOException(const std::string_view message, const std::source_location sourceLocation)
        : CoreException(std::format("IOException: {}", message), sourceLocation)
    {
    }

    NotImplementedException::NotImplementedException(const std::string_view     message,
                                                     const std::source_location sourceLocation)
        : CoreException(std::format("NotImplementedException: Called function is not (yet) implemented! {}", message),
                        sourceLocation)
    {
    }

    OutOfRangeException::OutOfRangeException(const std::string_view message, std::source_location sourceLocation)
        : CoreException(std::format("OutOfRangeException: {}", message), sourceLocation)
    {
    }
}  // namespace core
