#pragma once

#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <span>
#include <thread>
#include <unordered_map>

namespace core {

    struct IFileEventHandler {
        virtual ~IFileEventHandler() = default;

        virtual void OnFileModifiedEvent(const std::filesystem::path& file) {}

        virtual void OnFilesModifiedEvent(std::span<const std::filesystem::path> files) {}
    };

    // Asynchronously monitors a list of files based on their modification timestamp.
    class FileWatchdog {
    public:
        FileWatchdog();
        ~FileWatchdog();

        // Terminates the monitor thread
        void Terminate();

        // Add file to be monitored
        void AddFile(const std::filesystem::path& file);
        // Remove file form being monitored
        void RemoveFile(const std::filesystem::path& file);

        void RegisterEventHandler(IFileEventHandler* eventHandler);
        void UnregisterEventHandler(const IFileEventHandler* eventHandler);

    private:
        void StartWorker();

        void EmitFileModifiedEvent(const std::filesystem::path& file) const;
        void EmitFilesModifiedEvent(std::span<const std::filesystem::path> files);

        std::unordered_map<std::filesystem::path, std::filesystem::file_time_type> files_;
        std::mutex                                                                 fileListMutex_;

        std::thread             workerThread_;
        std::atomic_bool        workerThreadSignal_;
        std::condition_variable workerThreadCv_;

        std::vector<IFileEventHandler*> eventHandlers_;
    };

}  // namespace core