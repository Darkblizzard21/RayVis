/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once 
#include <chrono>

class Clock {
public:
    enum Unit { NanoSeconds, MicroSeconds, MiliSeconds, Seconds, Minutes, Hours, Days };

    Clock();

    double ElapsedTime(Unit unit = Seconds) const;
    double DeltaTime(Unit unit = Seconds) const;

    Clock& Reset();
    Clock& Advance(double minDeltaSeconds = 0.0);

private:
    double Convert(const double ms, Unit unit) const;

    std::chrono::system_clock::time_point start_;
    std::chrono::system_clock::time_point current_;

    double elapsedMs_;
    double deltaMs_;
};