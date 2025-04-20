#pragma once

#include <limits>
#include <algorithm>

namespace Raytracing {

    static bool isEqual(float a, float b) {
        const float diff = std::abs(a - b);
        const float epsilon = std::numeric_limits<float>::epsilon();

        return diff < epsilon;
    }

}