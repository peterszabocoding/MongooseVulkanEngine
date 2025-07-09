#pragma once

#include "vec3.h"
#include "ray.h"

namespace MongooseVK {

    struct HitRecord 
    {
        vec3 point;
        vec3 normal;
        double t;
    };

    class Hitable
    {
        public:
            virtual ~Hitable() = default;
            virtual bool Hit(const Ray& r, double ray_tmin, double ray_tmax, HitRecord& rec) const = 0;
    };

    class Sphere: public Hitable
    {
        public:
            Sphere(vec3 o, double r): center(o), radius(r) {}
            virtual ~Sphere() = default;

            virtual bool Hit(const Ray& r, double ray_tmin, double ray_tmax, HitRecord& rec) const override 
            {
                vec3 oc = center - r.origin();
                auto a = r.direction().length_squared();
                auto h = dot(r.direction(), oc);
                auto c = oc.length_squared() - radius * radius;

                auto discriminant = h*h - a*c;
                if (discriminant < 0) {
                    return false;
                }

                auto sqrtd = std::sqrt(discriminant);
                
                // Find the nearest root that lies in the acceptable range.
                auto root = (h - sqrtd) / a;
                if (root <= ray_tmin || ray_tmax <= root) {
                    root = (h + sqrtd) / a;
                    if (root <= ray_tmin || ray_tmax <= root)
                        return false;
                }

                rec.t = root;
                rec.point = r.at(rec.t);
                rec.normal = (rec.point - center) / radius;

                return true;
            }

        private:
            vec3 center;
            double radius;
    };


}