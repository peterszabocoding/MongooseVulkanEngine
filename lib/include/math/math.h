#pragma once

#define _USE_MATH_DEFINES

#include <limits>
#include <algorithm>
#include <math.h>
#include <cmath>

namespace Raytracing {

    static bool isEqual(float a, float b) {
        const float diff = std::abs(a - b);
        const float epsilon = std::numeric_limits<float>::epsilon();

        return diff < epsilon;
    }

    static float lerp(float a, float b, float f)
    {
        return a + f * (b - a);
    }

}