const float PI = 3.14159;


vec3 filament_schlick(const vec3 f0, float VoH) {
    float f = pow(1.0 - VoH, 5.0);
    return f + f0 * (1.0 - f);
}

vec3 filament_schlick(const vec3 f0, float f90, float VoH) {
    return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0);
}

float filament_schlick(float f0, float f90, float VoH) {
    return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0);
}

float filament_DGGX(float NoH, float roughness) {
    float a2 = roughness * roughness;
    float f = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / (PI * f * f);
}

float filament_DGGX_fast(float NoH, float roughness, const vec3 n, const vec3 h) {
    vec3 NxH = cross(n,h);
    float a = NoH * roughness;
    float k = roughness / (dot(NxH, NxH) + a * a);
    float d = k * k * (1.0/PI);
    return min(d, 65504.0);
}

float filament_SmithGGX(float NoV, float NoL, float roughness) {
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    float GGXL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    return min(0.5 / (GGXV + GGXL), 65504.0);
}

float filament_DLambert() {
    return 1.0 / PI;
}

float filament_Burley(float roughness, float NoV, float NoL, float LoH) {
    // Burley 2012, "Physically-Based Shading at Disney"
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = filament_schlick(1.0, f90, NoL);
    float viewScatter  = filament_schlick(1.0, f90, NoV);
    return lightScatter * viewScatter * (1.0 / PI);
}

