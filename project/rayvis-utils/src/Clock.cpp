/* Copyright (c) 2023 Pirmin Pfeifer */
#include "Clock.h"

#include <spdlog/spdlog.h>
#include <windows.h>

using namespace std::chrono;

Clock::Clock()
{
    Reset();
}

double Clock::ElapsedTime(Unit unit) const
{
    return Convert(elapsedMs_, unit);
}

double Clock::DeltaTime(Unit unit) const
{
    return Convert(deltaMs_, unit);
}

Clock& Clock::Reset()
{
    start_     = system_clock::now();
    current_   = system_clock::now();
    elapsedMs_ = 0;
    deltaMs_   = 0;
    return *this;
};

Clock& Clock::Advance(double minDeltaSeconds)
{
    const auto minDeltaMiro = minDeltaSeconds * 1000 * 1000;
    auto       now        = system_clock::now();
    
    while (duration_cast<microseconds>(now - current_).count() < minDeltaMiro) {
        now = system_clock::now(); 
    }

    deltaMs_   = duration_cast<nanoseconds>(now - current_).count() / 1000.0 / 1000.0;
    elapsedMs_ = duration_cast<nanoseconds>(now - start_).count() / 1000.0 / 1000.0;
    current_   = now;
    return *this;
}

double Clock::Convert(const double ms, Unit unit) const
{
    if (unit == MiliSeconds) {
        return ms;
    }
    double multiplier = 1.0;
    switch (unit) {
    case Clock::NanoSeconds:
        multiplier = 1000 * 1000;
        break;
    case Clock::MicroSeconds:
        multiplier = 1000;
        break;
    case Clock::Seconds:
        multiplier = 1.0 / 1000;
        break;
    case Clock::Minutes:
        multiplier = 1.0 / 1000 / 60;
        break;
    case Clock::Hours:
        multiplier = 1.0 / 1000 / 60 / 60;
        break;
    case Clock::Days:
        multiplier = 1.0 / 1000 / 60 / 60 / 24;
        break;
    default:
        spdlog::warn("Unknown time unit. Defaulting to seconds");
    }
    return ms * multiplier;
}