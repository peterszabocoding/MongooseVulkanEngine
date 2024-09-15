#pragma once

#include "math/vec3.h"
#include <vector>
#include "math/hitable.h"

std::vector<Raytracing::Hitable*> test_scene = {
    new Raytracing::Sphere(Raytracing::vec3(0.0, 0.0, -1.0), 0.5),
};