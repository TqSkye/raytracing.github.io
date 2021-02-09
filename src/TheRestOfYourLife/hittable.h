#ifndef HITTABLE_H
#define HITTABLE_H
//==============================================================================================
// Originally written in 2016 by Peter Shirley <ptrshrl@gmail.com>
//
// To the extent possible under law, the author(s) have dedicated all copyright and related and
// neighboring rights to this software to the public domain worldwide. This software is
// distributed without any warranty.
//
// You should have received a copy (see file COPYING.txt) of the CC0 Public Domain Dedication
// along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//==============================================================================================

#include "rtweekend.h"

#include "aabb.h"


class material;


class hit_record {
  public:
    point3 p;
    vec3 normal;
    shared_ptr<material> mat;
    double t;
    double u;
    double v;
    bool front_face;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal :-outward_normal;
    }
};


class hittable {
  public:
    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    virtual bool bounding_box(aabb& output_box) const = 0;

    virtual double pdf_value(const vec3& o, const vec3& v) const {
        return 0.0;
    }

    virtual vec3 random(const vec3& o) const {
        return vec3(1,0,0);
    }
};


class flip_face : public hittable {
  public:
    flip_face(shared_ptr<hittable> p) : ptr(p) {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!ptr->hit(r, ray_t, rec))
            return false;

        rec.front_face = !rec.front_face;
        return true;
    }

    bool bounding_box(aabb& output_box) const override {
        return ptr->bounding_box(output_box);
    }

  public:
    shared_ptr<hittable> ptr;
};


class translate : public hittable {
  public:
    translate(shared_ptr<hittable> p, const vec3& displacement)
        : ptr(p), offset(displacement) {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        ray moved_r(r.origin() - offset, r.direction(), r.time());
        if (!ptr->hit(moved_r, ray_t, rec))
            return false;

        rec.p += offset;
        rec.set_face_normal(moved_r, rec.normal);

        return true;
    }

    bool bounding_box(aabb& output_box) const override {
        if (!ptr->bounding_box(output_box))
            return false;

        output_box += offset;
        return true;
    }

  public:
    shared_ptr<hittable> ptr;
    vec3 offset;
};


class rotate_y : public hittable {
  public:
    rotate_y(shared_ptr<hittable> p, double angle) : ptr(p) {
        auto radians = degrees_to_radians(angle);
        sin_theta = sin(radians);
        cos_theta = cos(radians);
        hasbox = ptr->bounding_box(bbox);

        point3 min( infinity,  infinity,  infinity);
        point3 max(-infinity, -infinity, -infinity);

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                for (int k = 0; k < 2; k++) {
                    auto x = i*bbox.x.max + (1-i)*bbox.x.min;
                    auto y = j*bbox.y.max + (1-j)*bbox.y.min;
                    auto z = k*bbox.z.max + (1-k)*bbox.z.min;

                    auto newx =  cos_theta*x + sin_theta*z;
                    auto newz = -sin_theta*x + cos_theta*z;

                    vec3 tester(newx, y, newz);

                    for (int c = 0; c < 3; c++) {
                        min[c] = fmin(min[c], tester[c]);
                        max[c] = fmax(max[c], tester[c]);
                    }
                }
            }
        }

        bbox = aabb(min, max);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        auto origin = r.origin();
        auto direction = r.direction();

        origin[0] = cos_theta*r.origin()[0] - sin_theta*r.origin()[2];
        origin[2] = sin_theta*r.origin()[0] + cos_theta*r.origin()[2];

        direction[0] = cos_theta*r.direction()[0] - sin_theta*r.direction()[2];
        direction[2] = sin_theta*r.direction()[0] + cos_theta*r.direction()[2];

        ray rotated_r(origin, direction, r.time());

        if (!ptr->hit(rotated_r, ray_t, rec))
            return false;

        auto p = rec.p;
        auto normal = rec.normal;

        p[0] =  cos_theta*rec.p[0] + sin_theta*rec.p[2];
        p[2] = -sin_theta*rec.p[0] + cos_theta*rec.p[2];

        normal[0] =  cos_theta*rec.normal[0] + sin_theta*rec.normal[2];
        normal[2] = -sin_theta*rec.normal[0] + cos_theta*rec.normal[2];

        rec.p = p;
        rec.set_face_normal(rotated_r, normal);

        return true;
    }

    bool bounding_box(aabb& output_box) const override {
        output_box = bbox;
        return hasbox;
    }

  public:
    shared_ptr<hittable> ptr;
    double sin_theta;
    double cos_theta;
    bool hasbox;
    aabb bbox;
};


#endif
