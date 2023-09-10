#include "FileUtils.h"

#include <core-utils/Exceptions.h>

#include <chrono>

namespace core {

    FileWatchdog::FileWatchdog()
    {
        StartWorker();
    }

    FileWatchdog::~FileWatchdog()
    {
        Terminate();
    }

    void FileWatchdog::Terminate()
    {
        if (workerThread_.joinable()) {
            workerThreadSignal_ = false;
            workerThreadCv_.notify_one();
            workerThread_.join();
        }
    }

    void FileWatchdog::AddFile(const std::filesystem::path& file)
    {
        if (!std::filesystem::exists(file)) {
            throw InvalidArgumentException(std::format("file \"{}\" does not exist.", file.generic_string()));
        }

        const auto absolutePath = std::filesystem::absolute(file);

        std::unique_lock<std::mutex> lock(fileListMutex_);

        if (files_.find(absolutePath) != files_.end()) {
            throw InvalidArgumentException(
                std::format("file \"{}\" is already monitored.", absolutePath.generic_string()));
        }

        files_.emplace(absolutePath, std::filesystem::last_write_time(absolutePath));
    }

    void FileWatchdog::RemoveFile(const std::filesystem::path& file)
    {
        const auto absolutePath = std::filesystem::absolute(file);

        std::unique_lock<std::mutex> lock(fileListMutex_);

        if (files_.find(absolutePath) == files_.end()) {
            throw InvalidArgumentException(std::format("file \"{}\" is not monitored.", absolutePath.generic_string()));
        }

        files_.erase(absolutePath);
    }

    void FileWatchdog::RegisterEventHandler(IFileEventHandler* eventHandler)
    {
        if (eventHandler == nullptr) {
            throw InvalidArgumentException("eventHandler cannot be nullptr.");
        }

        eventHandlers_.push_back(eventHandler);
    }

    void FileWatchdog::UnregisterEventHandler(const IFileEventHandler* eventHandler)
    {
        if (eventHandler == nullptr) {
            throw InvalidArgumentException("eventHandler cannot be nullptr.");
        }

        if (const auto it = std::find(eventHandlers_.begin(), eventHandlers_.end(), eventHandler);
            it != eventHandlers_.end())
        {
            eventHandlers_.erase(it);
        }
    }

    void FileWatchdog::StartWorker()
    {
        if (workerThread_.joinable()) {
            throw CoreException("workerThread is already running.");
        }

        workerThreadSignal_ = true;

        workerThread_ = std::thread{[&]() {
            using namespace std::chrono_literals;

            while (workerThreadSignal_) {
                std::unique_lock<std::mutex> lock(fileListMutex_);

                std::vector<std::filesystem::path> modifiedFiles;

                for (auto& [path, modificationTimestamp] : files_) {
                    const auto timestamp = std::filesystem::last_write_time(path);

                    if (modificationTimestamp != timestamp) {
                        modificationTimestamp = timestamp;

                        EmitFileModifiedEvent(path);

                        modifiedFiles.push_back(path);
                    }
                }

                if (!modifiedFiles.empty()) {
                    EmitFilesModifiedEvent(modifiedFiles);
                }

                workerThreadCv_.wait_for(lock, 1s);
            }
        }};
    }

    void FileWatchdog::EmitFileModifiedEvent(const std::filesystem::path& file) const
    {
        for (const auto eventHandler : eventHandlers_) {
            eventHandler->OnFileModifiedEvent(file);
        }
    }

    void FileWatchdog::EmitFilesModifiedEvent(std::span<const std::filesystem::path> files)
    {
        for (const auto eventHandler : eventHandlers_) {
            eventHandler->OnFilesModifiedEvent(files);
        }
    }

}  // namespace core
