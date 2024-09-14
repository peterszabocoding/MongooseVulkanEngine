#pragma once

#include <chrono>
#include <iostream>

namespace Raytracing {

    class Timer {

        public:

            Timer(const char* timerLabel)
            {
                label = timerLabel;
                startPoint = std::chrono::high_resolution_clock::now();
            }

            ~Timer()
            {
                Stop();
            }

            void Stop()
            {
                auto endTimepoint = std::chrono::high_resolution_clock::now();

                auto start = std::chrono::time_point_cast<std::chrono::microseconds>(startPoint).time_since_epoch().count();
                auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

                auto duration = end - start;
                double ms = duration * 0.001;

                std::clog << label << " | " << duration << " us (" << ms << " ms)" << std::endl;
            }

        private:
            const char* label;
            std::chrono::time_point<std::chrono::high_resolution_clock> startPoint;
    };

}