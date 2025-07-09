#pragma once

#include "vec3.h"

namespace MongooseVK {

  class Ray {
    public:
      Ray() {}

      Ray(const point3& origin, const vec3& direction) : o(origin), dir(direction) {}

      const point3& origin() const  { return o; }
      const vec3& direction() const { return dir; }

      point3 at(double t) const {
          return o + t * dir;
      }

    private:
      point3 o;
      vec3 dir;
  };

}
