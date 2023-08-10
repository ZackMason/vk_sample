

float saturate(float a) {
    return clamp(a, 0.0, 1.0);
}

vec3 saturate(vec3 a) {
    return clamp(a, vec3(0.0), vec3(1.0));
}

vec3 saturate_min(vec3 a, float mi) {
    return clamp(a, vec3(mi), vec3(1.0));
}