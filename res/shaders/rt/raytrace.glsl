#include "rng.glsl"

#ifndef RAY_MAX
#define RAY_MAX 1000.0
#endif

struct PrimativeID {
    uint primative;
    uint instance;
    uint flags;
};

struct BVHNode {
    vec3 bound_min;
    uint left;
    vec3 bound_max;
    uint right;
};

struct BVHPrimative {
    uint primative;
    uint instance;
    uint flags;
    float x0;

    float y0;
    float z0;
    float x1;
    float y1;

    float z1;
    float x2;
    float y2;
    float z2;
};

vec3 v0(BVHPrimative p) { return vec3(p.x0, p.y0, p.z0); }
vec3 v1(BVHPrimative p) { return vec3(p.x1, p.y1, p.z1); }
vec3 v2(BVHPrimative p) { return vec3(p.x2, p.y2, p.z2); }

struct Ray {
    vec3 origin;
    float tmin;
    vec3 direction;
    float tmax;
};

Ray 
CameraRay(
    mat4 inverse_view_projection, 
    vec3 camera_position, 
    vec2 clipspace
) {
	vec4 unprojected = inverse_view_projection * vec4(clipspace, 0, 1);
	unprojected.xyz /= unprojected.w;

	Ray ray;
	ray.origin = camera_position;
	ray.direction = normalize(unprojected.xyz - ray.origin);
	ray.tmin = 0.001;
	ray.tmax = RAY_MAX;

	return ray;
}

#ifndef RAY_STACK_SIZE
#define RAY_STACK_SIZE 32
#endif

#ifdef RAY_SHARED_STACK
shared uint stack[RAY_STACK_SIZE][RAY_BLOCK_SIZE*RAY_BLOCK_SIZE];
#endif

struct RayHit {
    vec2 bary;
    float dist;
    PrimativeID prim_id;
    bool backface;
};

RayHit 
CreateRayHit() {
	RayHit hit;
	hit.bary = vec2(0);
	hit.dist = RAY_MAX;
	hit.backface = false;
	return hit;
}

bool 
IntersectSphere(
    in Ray ray,
    vec4 sphere
) {
    vec3 p = sphere.xyz - ray.origin;
    float pd = dot(p,ray.direction);
    float r2 = sphere.w * sphere.w;
    if (dot(p,p) < r2) { return true; }
    if (pd < 0) { return false; }
    vec3 a = pd * ray.direction - p;
    return dot(a, a) < r2;
}

bool 
IntersectSphere2(
    in Ray ray,
    vec4 sphere
) {
    if (distance(ray.origin, sphere.xyz) < sphere.w) {
        return true;
    }
    vec3 p = sphere.xyz - ray.origin;
    float r2 = sphere.w * sphere.w;
    float pd = dot(p,ray.direction);
    if (pd > 0 || dot(p,p) < r2) {
        return false;
    }
    vec3 a = p - pd * ray.direction;
    float a2 = dot(a,a);
    if (a2>r2) return false;
    return true;
}

bool
IntersectTriangle(
    in Ray ray,
    inout RayHit hit,
    in BVHPrimative prim,
    inout uint rng
) {
    vec3 v0v1 = v1(prim) - v0(prim);
    vec3 v0v2 = v2(prim) - v0(prim);
    vec3 pvec = cross(ray.direction, v0v2);
    float det = dot(v0v1, pvec);

    if (det > 0.00001) {
        return false;
    }

    if (abs(det) < 0.0001) {
        return false;
    }
    float invDet = 1.0 / det;    
    vec3 tvec = ray.origin - v0(prim);
    float u = dot(tvec, pvec) * invDet;
    if (u<0||u>1) {
        return false;
    }

    vec3 qvec = cross(tvec, v0v1);
    float v = dot(ray.direction, qvec) * invDet;
    if (v < 0 || u+v> 1) {
        return false;
    }

    float t = dot(v0v2, qvec) * invDet;

    if (t >= ray.tmin && t <= ray.tmax && t <= hit.dist) {
        RayHit result = CreateRayHit();
        result.prim_id.primative = prim.primative;
        result.prim_id.instance = prim.instance;
        result.bary = vec2(u,v);
        result.backface = det > 0;
        result.dist = t;
        hit = result;
        return true;
    }
    return false;
}
