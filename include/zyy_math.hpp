#pragma once 

namespace axis {
    constexpr inline static v3f up          {0.0f, 1.0f, 0.0f};
    constexpr inline static v3f down        {0.0f,-1.0f, 0.0f};
    constexpr inline static v3f right       {1.0f, 0.0f, 0.0f};
    constexpr inline static v3f left        {-1.0f,0.0f, 0.0f};
    constexpr inline static v3f forward     {0.0f, 0.0f,-1.0f};
    constexpr inline static v3f backward    {0.0f, 0.0f, 1.0f};
    
};

namespace planes {
    constexpr inline static v3f xy {axis::right + axis::up};
    constexpr inline static v3f yz {axis::backward + axis::up};
    constexpr inline static v3f xz {axis::right + axis::backward};

    constexpr inline static v3f yx {axis::right + axis::up};
    constexpr inline static v3f zy {axis::backward + axis::up};
    constexpr inline static v3f zx {axis::right + axis::backward};
};

namespace swizzle {
    constexpr v2f xx(v2f v) { return v2f{v.x,v.x}; }
    constexpr v2f yx(v2f v) { return v2f{v.y,v.x}; }
    constexpr v2f yy(v2f v) { return v2f{v.y,v.y}; }
    
    constexpr v2f xy(v3f v) { return v2f{v.x,v.y}; }
    constexpr v2f xx(v3f v) { return v2f{v.x,v.x}; }
    constexpr v2f yx(v3f v) { return v2f{v.y,v.x}; }
    constexpr v2f yy(v3f v) { return v2f{v.y,v.y}; }
    constexpr v2f zy(v3f v) { return v2f{v.z,v.y}; }
    constexpr v2f yz(v3f v) { return v2f{v.y,v.z}; }
    constexpr v2f xz(v3f v) { return v2f{v.x,v.z}; }
    constexpr v2f zx(v3f v) { return v2f{v.z,v.x}; }
};

namespace math {
    constexpr inline static v2f zero2{0.0f};
    constexpr inline static v2f width2{1.0f, 0.0f};
    constexpr inline static v2f height2{0.0f, 1.0f};
    constexpr inline static v2f half2{0.5f};
    constexpr inline static v2f half_width2{0.5f, 0.0f};
    constexpr inline static v2f half_height2{0.5f, 0.5f};
    
    m33 normalize(const m33& m) {
        return m33 {
            glm::normalize(m[0]),
            glm::normalize(m[1]),
            glm::normalize(m[2])
        };
    }

    constexpr u64 pretty_bytes(u64 size) {
        return size > gigabytes(1) ? size / gigabytes(1) :
                size > megabytes(1) ? size / megabytes(1) :
                size > kilobytes(1) ? size / kilobytes(1) :
                size;
    }

    constexpr std::string_view pretty_bytes_postfix(u64 size) {
        return size > gigabytes(1) ? "Gb"sv :
                size > megabytes(1) ? "Mb"sv :
                size > kilobytes(1) ? "Kb"sv :
                "B";
    }

    constexpr auto clamp_length(auto v, f32 min = 0.0f, f32 max = 1.0f) {
        const auto length = glm::length(v); 
        return length > 0.0f ? glm::normalize(v)*glm::clamp(length, min, max) : v;
    }

    constexpr quat quat_identity() {
        return quat{1,0,0,0};
    }

    template <typename SamplerType>
    concept CSampler = requires(SamplerType sampler, f32 t) {
        { sampler.sample(t) } -> std::same_as<typename SamplerType::type>;
    };

    constexpr v2f from_polar(f32 angle, f32 radius = 1.0f) noexcept {
        return v2f{glm::cos(angle), glm::sin(angle)} * radius;
    }

    template <typename T>
    constexpr T remap01(T a, T b, T x) {
        return (x-a)/(b-a);
    }

    template <typename T>
    constexpr T sqr(T x) {
        return x * x;
    }

    template <typename T>
    constexpr T cube(T x) {
        return x * x * x;
    }

    constexpr v2f octahedral_mapping(v3f co) {
        using namespace swizzle;
        // projection onto octahedron
        co /= glm::dot( v3f(1.0f), glm::abs(co) );

        // out-folding of the downward faces
        if ( co.y < 0.0f ) {
            co = v3f(1.0f - glm::abs(zx(co)) * glm::sign(xz(co)), co.z);
        }

        // mapping to [0;1]ˆ2 texture space
        return xy(co) * 0.5f + 0.5f;
    }

    constexpr v3f octahedral_unmapping(v2f co) {
        using namespace swizzle;
        co = co * 2.0f - 1.0f;

        v2f abs_co = glm::abs(co);
        v3f v = v3f(co, 1.0f - (abs_co.x + abs_co.y));

        if ( abs_co.x + abs_co.y > 1.0f ) {
            v = v3f{(glm::abs(yx(co)) - 1.0f) * -glm::sign(co), v.z};
        }

        return v;
    }

    constexpr v3f get_spherical(f32 yaw, f32 pitch) {
        return v3f{
            glm::cos((yaw)) * glm::cos((pitch)),
            glm::sin((pitch)),
            glm::sin((yaw)) * glm::cos((pitch))
        };
    }

    v3f world_to_screen(const m44& proj, const m44& view, const v4f& viewport, const v3f& p) noexcept {
        return glm::project(p, view, proj, viewport);
    }
    
    v3f world_to_screen(const m44& vp, const v4f& p) noexcept {
        v4f sp = vp * p;
        sp /= sp.w;
        sp.x = sp.x * 0.5f + 0.5f;
        sp.y = sp.y * 0.5f + 0.5f;
        return sp;
    }

    v3f world_to_screen(const m44& vp, const v3f& p) noexcept {
        return world_to_screen(vp, v4f{p, 1.0f});
    }

    v3f world_normal_to_screen(const m44& vp, const v3f& p) noexcept {
        return world_to_screen(vp, v4f{p, 0.0f});
    }

    bool fcmp(f32 a, f32 b, f32 eps = 1e-6) {
        return std::fabsf(a-b) < eps;
    }

    namespace constants {
        inline static constexpr auto pi32  = glm::pi<f32>();
        inline static constexpr auto tau32 = glm::pi<f32>()*2.0f;
        inline static constexpr auto e32   = 2.71828182845904523536f;

    }
    using perc_tau_t = f32;

    constexpr f32 to_radians(perc_tau_t p) noexcept {
        return p*constants::tau32;
    }
    constexpr f32 to_degrees(perc_tau_t p) noexcept {
        return p * 360.0f;
    }

    struct pid_controller_t {
        enum struct mode {
            velocity, error_rate_of_change,
        };

        f32 proportional_gain{0.8f};
        f32 derivative_gain{0.3f};
        f32 integral_gain{0.7f};
        f32 integral_saturation{0.0f};

        f32 output_min{-1.0f};
        f32 output_max{1.0f};
        mode derivative_mode{mode::error_rate_of_change};

        f32 _integral{0.0f};
        f32 _error_last{0.0f};
        f32 _value_last{0.0f};
        b32 _initialized{false};

        void reset() {
            _initialized = false;
            _integral = 0.0f;
        }

        f32 compute(f32 dt, f32 current_value, f32 target_value) {
            const f32 error = target_value - current_value;
            const f32 P = proportional_gain * error;

            _integral = glm::clamp(_integral + (error * dt), -integral_saturation, integral_saturation);
            f32 I = integral_gain * _integral;

            f32 error_derivative = (error-_error_last)/dt;
            _error_last = error;

            f32 value_derivative = (current_value-_value_last)/dt;
            _value_last = current_value;

            float D = 0.0f;
            if (_initialized) {
                D = derivative_gain * (derivative_mode == mode::velocity ?
                    -value_derivative : error_derivative);
            } else {
                _initialized = 1;
            }
            return glm::clamp(P+I+D, output_min, output_max);
        }
    };

    struct position_pid_t {
        pid_controller_t x{};
        pid_controller_t y{};
        pid_controller_t z{};

        explicit position_pid_t(const pid_controller_t& o = pid_controller_t{}) 
            : x{o}, y{o}, z{o}
        {
        }
        position_pid_t& operator=(const pid_controller_t& o) {
            x = o;
            y = o;
            z = o;
            return *this;
        }

        v3f compute(f32 dt, v3f current_value, v3f target_value) {
            return v3f {
                x.compute(dt, current_value.x, target_value.x),
                y.compute(dt, current_value.y, target_value.y),
                z.compute(dt, current_value.z, target_value.z)
            };
        }
    };

    // TODO(Zack) add bspline for camera
    
    template<typename T>
    T hermite(float t, const T& p1, const T& s1, const T& p2, const T& s2) noexcept {
        return 
            p1 * ((1.0f + 2.0f * t) * ((1.0f - t) * (1.0f - t))) +
            s1 * (t * ((1.0f - t) * (1.0f - t))) +
            p2 * ((t * t) * (3.0f - 2.0f * t)) +
            s2 * ((t * t) * (t - 1.0f));
    }

    struct curve_t {
        struct point_t {
            v2f position{};
            f32 left_tangent{0.0f};
            f32 right_tangent{0.0f};
        };

        umm count{0};
        std::array<point_t, 16> points{};
    };

    struct cat_curve_t {
        using type = v2f;
        std::array<type,4> p{};

        f32 get_t(f32 t, f32 alpha, auto p0, auto p1) {
            auto d = p1-p0;
            float a = glm::dot(d,d);
            float b = glm::pow(a, 0.5f*alpha);
            return b+t;
        }

        type sample(f32 t, f32 alpha = 0.5f) {
            float t0=0.0f;
            float t1=get_t(t0,alpha,p[0],p[1]);
            float t2=get_t(t1,alpha,p[1],p[2]);
            float t3=get_t(t2,alpha,p[2],p[3]);
            t=glm::mix(t1,t2,t);
            auto A1 = (t1-t)/(t1-t0)*p[0]+(t-t0)/(t1-t0)*p[1];
            auto A2 = (t2-t)/(t2-t1)*p[1]+(t-t1)/(t2-t1)*p[2];
            auto A3 = (t3-t)/(t3-t2)*p[2]+(t-t2)/(t3-t2)*p[3];
            auto B1 = (t2-t)/(t2-t0)*A1+(t-t0)/(t2-t0)*A2;
            auto B2 = (t3-t)/(t3-t1)*A2+(t-t1)/(t3-t1)*A3;
            auto C  = (t2-t)/(t2-t1)*B1+(t-t1)/(t2-t1)*B2;
            return C;
        }

    };

    struct triangle_t {
        std::array<v3f,3> p;

        v3f normal() const {
            return glm::normalize(glm::cross(e1(), e2()));
        }

        v3f e1() const {
            return  p[1]-p[0];
        }
        v3f e2() const {
            return  p[2]-p[0];
        }

        v3f bary(v2f uv) {
            return p[0] + e1() * uv.x + e2() * uv.y;
        }
    };

    struct plane_t {
        v3f n{};
        f32 d{};

        static plane_t 
        from_triangle(const triangle_t& tri) {
            plane_t p;
            p.n = tri.normal();
            p.d = glm::dot(tri.p[0], p.n);
        }

        f32 distance(v3f p) const {
            return glm::dot(n, p) - d;
        }
    };

    struct sphere_t {
        v3f origin{0.0f};
        f32 radius{0.0f};
    };

    struct circle_t {
        v2f origin{0.0f};
        f32 radius{0.0f};

        bool contains(v2f p) {
            return glm::distance2(origin, p) < radius * radius;
        }
    };

    
    struct polygon_t {
        v3f* points{};
        size_t count{0}, capacity{0};

        explicit polygon_t(v3f* memory, size_t capacity_)
            : points{memory}, capacity{capacity_}
        {}

        v3f sample(f32 t) {
            assert(count>2);
            size_t index = size_t(t * f32(count));
            f32 p1 = f32(index/(count));
            f32 p2 = f32((index+1)/(count));
            f32 d = p2-p1;
            t = (t-p1)/d;
            return points[index] + t * points[index+1];
        }

        polygon_t& add(v3f p) {
            assert(count < capacity);
            points[count++] = p;

            return *this;
        }

        polygon_t& insert(size_t i, v3f p) {
            assert(i<=count);
            assert(i<capacity);
            size_t r{count}, w{count+1};
            while(i&&w!=i) { 
                points[w--] = points[r--]; 
            }
            points[i] = p;
            count++;
            return *this;
        }

        polygon_t& remove(size_t i) {
            assert(i<count);
            assert(i<capacity);
            size_t r{i+1}, w{i};
            while(i&&i!=capacity&&w!=count) points[w++] = points[r++];
            count--;
            return *this;
        }
    };

    bool intersect(circle_t c, v2f p) {
        return glm::distance2(c.origin, p) < c.radius * c.radius;
    }

    bool intersect(sphere_t a, v3f b) {
        return glm::distance2(a.origin, b) < a.radius * a.radius;
    }

    f32 distance(circle_t c, v2f p) {
        return glm::distance(c.origin, p) - c.radius;
    }

    f32 distance(sphere_t a, v3f p) {
        return glm::distance(a.origin, p) - a.radius;
    }

    f32 distance(plane_t pl, v3f p) {
        return pl.distance(p);
    }

    struct frustum_t {
        v3f points[8];
    };

    // template <typename T>
    struct hit_result_t {
        bool intersect{false};
        f32 distance{0.0f};
        v3f position{0.f, 0.f, 0.f};
        // T& object;

        operator bool() const {
            return intersect;
        }
    };

    struct ray_t {
        v3f origin;
        v3f direction;

        v3f at(f32 t) const {
            return origin + direction * t;
        }
    };
    
template <typename T = v3f>
struct aabb_t {
    // aabb_t(){}
    // aabb_t(const T& o) {
    //     expand(o);
    // }

    // aabb_t(const T& min_, const T& max_) noexcept
    //     : min{min_}, max{max_}
    // {
    // }

    T min{std::numeric_limits<float>::max()};
    T max{-std::numeric_limits<float>::max()};

    T size() const {
        return max - min;
    }

    T center() const {
        return (max+min)/2.0f;
    }
    
    T sample(T a) const {
        return size() * a + min;
    }

    // from center
    void set_size(v2f s) {
        auto c = center();
        aabb_t<T> t;
        t.expand(c - s * 0.5f);
        t.expand(c + s * 0.5f);
        *this = t;
    }

    void pull(T amount) {
        aabb_t<T> t;
        t.expand(min - amount);
        t.expand(max + amount);
        *this = t;
    }

    void pad(f32 amount) {
        pull(v2f{-amount});
    }

    void add(T amount) {
        aabb_t<T> t;
        t.expand(min + amount);
        t.expand(max + amount);
        *this = t;
    }

    void scale(T amount) {
        aabb_t<T> t;
        t.expand(center() + size() * 0.5f * amount);
        t.expand(center() - size() * 0.5f * amount);
        *this = t;
    }

    void expand(const aabb_t<T>& o) {
        expand(o.min);
        expand(o.max);
    }

    void expand(const T& p) {
        min = glm::min(p, min);
        max = glm::max(p, max);
    }

    bool intersect(const aabb_t<v2f>& o) const {
        return  this->min.x <= o.max.x &&
                this->max.x >= o.min.x &&
                this->min.y <= o.max.y &&
                this->max.y >= o.min.y;
    }

    bool intersect(const aabb_t<v3f>& o) const {
        return  this->min.x <= o.max.x &&
                this->max.x >= o.min.x &&
                this->min.y <= o.max.y &&
                this->max.y >= o.min.y &&
                this->min.z <= o.max.z &&
                this->max.z >= o.min.z;
    }

    bool contains(const aabb_t<T>& o) const {
        return contains(o.min) && contains(o.max);
    }
    bool contains(const f32& p) const {
        return 
            min <= p &&
            p <= max;            
    }
    bool contains(const v2f& p) const {
        return 
            min.x <= p.x &&
            min.y <= p.y &&
            p.x <= max.x &&
            p.y <= max.y;
    }
    bool contains(const v3f& p) const {
        //static_assert(std::is_same<T, v3f>::value);
        // return min <= p && max <= p;
        return 
            min.x <= p.x &&
            min.y <= p.y &&
            min.z <= p.z &&
            p.x <= max.x &&
            p.y <= max.y &&
            p.z <= max.z;
    }

    aabb_t<T> operator+(const T& v) const {
        return aabb_t<T>{min+v,max+v};
    }
    
    void set_min_size(T min_size) {
        T s = glm::max(min_size, size());
        auto c = center();
        expand(c + s/2.0f);
        expand(c - s/2.0f);
    }
    
    // this is clipper
    // o is rect being clipped
    aabb_t<v2f> clip(const aabb_t<v2f>& o) const {
        aabb_t<T> result;

        result.min.x = glm::max(min.x, o.min.x);
        result.max.x = glm::min(max.x, o.max.x);
        
        result.min.y = glm::max(min.y, o.min.y);
        result.max.y = glm::min(max.y, o.max.y);
        
        return result;
    }
    
    aabb_t<v2f> cut_left(f32 amount) {
        aabb_t<T> left = *this;

        this->min.x = left.max.x = left.min.x + amount;
        return left;
    }

    aabb_t<v2f> cut_right(f32 amount) {
        aabb_t<T> right = *this;

        this->max.x = right.min.x = right.max.x - amount;
        return right;
    }

    aabb_t<v2f> cut_top(f32 amount) {
        aabb_t<T> top = *this;

        this->min.y = top.max.y = top.min.y + amount;
        return top;
    }

    aabb_t<v2f> cut_bottom(f32 amount) {
        aabb_t<T> bottom = *this;

        this->max.y = bottom.min.y = bottom.max.y - amount;
        return bottom;
    }
};

using range_t = aabb_t<f32>;
using rect2d_t = aabb_t<v2f>;
using rect3d_t = aabb_t<v3f>;

constexpr inline static rect2d_t r2zero{v2f{0.0f}, v2f{0.0f}};
constexpr inline static rect3d_t r3zero{v3f{0.0f}, v3f{0.0f}};

rect2d_t zero_to(v2f range) {
    return rect2d_t {
        .min = v2f{0.0f},
        .max = range
    };
}

std::pair<rect2d_t, rect2d_t> cut_left(rect2d_t rect, f32 amount) {
    auto left = rect.cut_left(amount);
    return {left, rect};
}
std::pair<rect2d_t, rect2d_t> cut_right(rect2d_t rect, f32 amount) {
    auto right = rect.cut_right(amount);
    return {right, rect};
}
std::pair<rect2d_t, rect2d_t> cut_top(rect2d_t rect, f32 amount) {
    auto top = rect.cut_top(amount);
    return {top, rect};
}
std::pair<rect2d_t, rect2d_t> cut_bottom(rect2d_t rect, f32 amount) {
    auto bottom = rect.cut_bottom(amount);
    return {bottom, rect};
}

// template<size_t N>
// std::span<rect2d_t> partition(const rect2d_t& rect, stack_buffer<rect2d_t, N>&& buffer, u32 count, f32 padding) {
//     buffer.clear();

//     auto p_rect = rect;
//     p_rect.pull(v2f{-padding});

//     f32 size = p_rect.size().y;

//     for (u32 i = 0; i < count; i++) {

//     }

//     return buffer.view();
// }

v2f bottom_middle(rect2d_t r) {
    return r.min + r.size() * v2f{0.5f, 1.0f};
}

v2f top_middle(rect2d_t r) {
    return r.min + r.size() * v2f{0.5f, 0.0f};
}
v2f top_right(rect2d_t r) {
    return r.min + r.size() * v2f{1.0f, 0.0f};
}
v2f bottom_left(rect2d_t r) {
    return r.min + r.size() * v2f{0.0f, 1.0f};
}

static bool
intersect(aabb_t<v2f> rect, v2f p) {
    return rect.contains(p);
}

inline static bool
intersect(const ray_t& ray, const triangle_t& tri, f32& t) noexcept {
    const auto& [v0, v1, v2] = tri.p;
    const v3f v0v1 = v1 - v0;
    const v3f v0v2 = v2 - v0;
    const v3f n = glm::cross(v0v1, v0v2);
    const f32 area = glm::length(n);
    const f32 NoR = glm::dot(ray.direction, n);
    if (glm::abs(NoR) < 0.00001f) {
        return false;
    }
    const f32 d = -glm::dot(n, v0);

    t = -(glm::dot(n,ray.origin) + d) / NoR;
    if (t < 0.0f) {
        return false;
    }
    
    const v3f p = ray.origin + ray.direction * t;
    v3f c;

    // edge 0
    v3f edge0 = v1 - v0; 
    v3f vp0 = p - v0;
    c = glm::cross(edge0, vp0);
    if (glm::dot(n, c) < 0.0f) {return false;} // P is on the right side
 
    // edge 1
    v3f edge1 = v2 - v1; 
    v3f vp1 = p - v1;
    c = glm::cross(edge1, vp1);
    if (glm::dot(n, c) < 0.0f) {return false;} // P is on the right side
  
    // edge 2
    v3f edge2 = v0 - v2; 
    v3f vp2 = p - v2;
    c = glm::cross(edge2, vp2);
    if (glm::dot(n, c) < 0.0f) {return false;} // P is on the right side

    return true; 
}


bool intersect_aabb_slab( const ray_t& ray, float t, const v3f bmin, const v3f bmax )
{
    float tx1 = (bmin.x - ray.origin.x) / ray.direction.x, tx2 = (bmax.x - ray.origin.x) / ray.direction.x;
    float tmin = glm::min( tx1, tx2 ), tmax = glm::max( tx1, tx2 );
    float ty1 = (bmin.y - ray.origin.y) / ray.direction.y, ty2 = (bmax.y - ray.origin.y) / ray.direction.y;
    tmin = glm::max( tmin, glm::min( ty1, ty2 ) ), tmax = glm::min( tmax, glm::max( ty1, ty2 ) );
    float tz1 = (bmin.z - ray.origin.z) / ray.direction.z, tz2 = (bmax.z - ray.origin.z) / ray.direction.z;
    tmin = glm::max( tmin, glm::min( tz1, tz2 ) ), tmax = glm::min( tmax, glm::max( tz1, tz2 ) );
    return tmax >= tmin && tmin < t && tmax > 0;
}

inline hit_result_t          //<u64> 
intersect(const ray_t& ray, const aabb_t<v3f>& aabb) {
    const v3f inv_dir = 1.0f / ray.direction;

    const v3f min_v = (aabb.min - ray.origin) * inv_dir;
    const v3f max_v = (aabb.max - ray.origin) * inv_dir;

    const f32 tmin = std::max(std::max(std::min(min_v.x, max_v.x), std::min(min_v.y, max_v.y)), std::min(min_v.z, max_v.z));
    const f32 tmax = std::min(std::min(std::max(min_v.x, max_v.x), std::max(min_v.y, max_v.y)), std::max(min_v.z, max_v.z));

    hit_result_t hit;
    hit.intersect = false;

    if (tmax < 0.0f || tmin > tmax) {
        return hit;
    }

    if (tmin < 0.0f) {
        hit.distance = tmax;
    } else {
        hit.distance = tmin;
    }
    hit.intersect = true;
    hit.position = ray.origin + ray.direction * hit.distance;
    return hit;
}

struct transform_t {
	
	transform_t(const v3f& position, const quat& rotation) : origin{position} {
        set_rotation(rotation);
    }
	constexpr transform_t(const m44& mat = m44{1.0f}) : basis(mat), origin(mat[3]) {};
	constexpr transform_t(const m33& _basis, const v3f& _origin) : basis(_basis), origin(_origin) {};
	constexpr transform_t(const v3f& position, const v3f& scale = {1,1,1}, const v3f& rotation = {0,0,0})
	    : basis(m33(1.0f)
    ) {
		origin = (position);
		set_rotation(rotation);
		set_scale(scale);
	}
    
	m44 to_matrix() const {
        m44 res = basis;
        res[3] = v4f(origin,1.0f);
        return res;
    }

    math::transform_t operator*(const math::transform_t& o) const {
        return math::transform_t{to_matrix() * o.to_matrix()};
    }
    
	operator m44() const {
		return to_matrix();
	}
    f32 get_basis_magnitude(u32 i) {
        assert(i < 3);
        return glm::length(basis[i]);
    }
	transform_t& translate(const v3f& delta) {
        origin += delta;
        return *this;
    }
	transform_t& scale(const v3f& delta) {
        basis = glm::scale(m44{basis}, delta);
        return *this;
    }
	transform_t& rotate(const v3f& axis, f32 rads) {
        set_rotation((get_orientation() * glm::angleAxis(rads, axis)));

        // m44 rot = m44(1.0f);
        // rot = glm::rotate(rot, delta.z, { 0,0,1 });
        // rot = glm::rotate(rot, delta.y, { 0,1,0 });
        // rot = glm::rotate(rot, delta.x, { 1,0,0 });
        // basis = (basis) * m33(rot);
        return *this;
    }
	transform_t& rotate_quat(const glm::quat& delta) {
        set_rotation(get_orientation() * delta);
        return *this;
    }
    
    glm::quat get_orientation() const {
        return glm::normalize(glm::quat_cast(basis));
    }

	transform_t inverse() const {
        transform_t transform;
        transform.basis = glm::transpose(basis);
        transform.origin = transform.basis * -origin;
        return transform;
    }

	void look_at(const v3f& target, const v3f& up = axis::up) {
        set_rotation(glm::quatLookAt(target-origin, up));
    }

    constexpr transform_t& set_scale(const v3f& scale) {
        for (int i = 0; i < 3; i++) {
            basis[i] = glm::normalize(basis[i]) * scale[i];
        }
        return *this;
    }
	constexpr void set_rotation(const v3f& rotation) {
        f32 scales[3];
        range_u32(i, 0, 3) {
            scales[i] = glm::length(basis[i]);
        }
        basis = glm::eulerAngleYXZ(rotation.y, rotation.x, rotation.z);
        range_u32(i, 0, 3) {
            basis[i] *= scales[i];
        }
    }
    // in radians
	void set_rotation(v3f axis, f32 rads) {
        set_rotation(glm::angleAxis(rads, axis));
    }
	void set_rotation(const glm::quat& quat) {
        f32 scales[3];
        range_u32(i, 0, 3) {
            scales[i] = glm::length(basis[i]);
        }
        basis = glm::toMat3(glm::normalize(quat));
        range_u32(i, 0, 3) {
            basis[i] *= scales[i];
        }
    }

    void normalize() {
        for (decltype(basis)::length_type i = 0; i < 3; i++) {
            basis[i] = glm::normalize(basis[i]);
        }
    }

	// in radians
	v3f get_euler_rotation() const {
        return glm::eulerAngles(get_orientation());
    }

	transform_t affine_invert() const {
        auto t = *this;
		t.basis = glm::inverse(t.basis);
		t.origin = t.basis * -t.origin;
        return t;
	}

	v3f inv_xform(const v3f& vector) const
	{
		const v3f v = vector - origin;
		return glm::transpose(basis) * v;
	}

	v3f xform(const v3f& vector) const {
		return v3f(basis * vector) + origin;
	}
    
	ray_t xform(const ray_t& ray) const { 
		return ray_t{
			xform(ray.origin),
			glm::normalize(basis * ray.direction)
		};
	}

	aabb_t<v3f> xform_aabb(const aabb_t<v3f>& box) const {
		aabb_t<v3f> t_box;
		t_box.min = t_box.max = origin;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
                const auto a = basis[j][i] * box.min[j];
                const auto b = basis[j][i] * box.max[j];
                t_box.min[i] += a < b ? a : b;
                t_box.max[i] += a < b ? b : a;
            }
        }

		return t_box;
	}
    
	m33 basis = m33(1.0f);
	v3f origin = v3f(0, 0, 0);
};

struct statistic_t {
    f64         average{};
    aabb_t<f64> range{};
    u32         count{};
};

inline void
begin_statistic(statistic_t& stat) {
    stat.average = 0.0;
    stat.count = 0;
    stat.range.max = std::numeric_limits<f64>::min();
    stat.range.min = std::numeric_limits<f64>::max();
}

inline void
update_statistic(statistic_t& stat, f64 value) {
    stat.range.expand(value);
    stat.average += value;
    stat.count += 1;
}

inline void
end_statistic(statistic_t& stat) {
    if (stat.count) {
        stat.average /= (f64)stat.count;
    } else {
        stat.average = stat.range.min = 0.0;
        stat.range.max = 1.0; // to avoid div by 0
    }
}

}; // namespace math
