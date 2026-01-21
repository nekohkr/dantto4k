#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <string>

class ProgressReporter {
public:
    ProgressReporter(uint64_t totalSize, bool enabled)
        : totalSize_(totalSize), enabled_(enabled), 
          startTime_(std::chrono::steady_clock::now()), 
          lastUpdateTime_(startTime_), processedSize_(0) {}

    void update(uint64_t processed) {
        if (!enabled_) {
            return;
        }

        processedSize_ += processed;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime_);

        if (elapsed.count() >= 500 || (totalSize_ > 0 && processedSize_ == totalSize_)) {
            lastUpdateTime_ = now;
            printProgress();
        }
    }

    void finish() {
        if (enabled_) {
            if (totalSize_ > 0) {
                processedSize_ = totalSize_;
            }
            printProgress();
            std::cerr << std::endl;
        }
    }

private:
    void printProgress() {
        auto now = std::chrono::steady_clock::now();
        auto totalElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_);
        double speed = 0;
        if (totalElapsed.count() > 0) {
            speed = static_cast<double>(processedSize_) / (1024 * 1024) / totalElapsed.count();
        }

        std::cerr << "\r[";
        if (totalSize_ > 0) {
            double percentage = static_cast<double>(processedSize_) / totalSize_ * 100;
            std::cerr << std::fixed << std::setprecision(1) << percentage << "%] ";
            std::cerr << processedSize_ / (1024 * 1024) << "/" << totalSize_ / (1024 * 1024) << " MiB, ";
        } else {
            std::cerr << processedSize_ / (1024 * 1024) << " MiB, ";
        }

        std::cerr << std::fixed << std::setprecision(2) << speed << " MiB/s, ";

        auto formatTime = [](long long seconds) {
            long long h = seconds / 3600;
            long long m = (seconds % 3600) / 60;
            long long s = seconds % 60;
            char buf[32];
            snprintf(buf, sizeof(buf), "%lld:%02lld:%02lld", h, m, s);
            return std::string(buf);
        };

        std::cerr << formatTime(totalElapsed.count());

        if (totalSize_ > 0 && speed > 0.001) {
            long eta = static_cast<long>((totalSize_ - processedSize_) / (speed * 1024 * 1024));
            std::cerr << " eta " << formatTime(eta);
        }
        std::cerr << "   "; // padding
        std::cerr.flush();
    }

    uint64_t totalSize_;
    bool enabled_;
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point lastUpdateTime_;
    uint64_t processedSize_;
};
