const float PI = 3.14159;


vec3 filament_Schlick(const vec3 f0, float VoH) {
    float f = pow(1.0 - VoH, 5.0);
    return f + f0 * (1.0 - f);
}

vec3 filament_Schlick(float u, vec3 f0) {
    return f0 + (vec3(1.0) - f0) * pow(1.0 - u, 5.0);
}

float filament_Schlick(float u, float f0, float f90) {
    return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}

float filament_DGGX(float NoH, float roughness) {
    float a = NoH * roughness;
    float k = roughness / (1.0 - NoH * NoH + a * a);
    return k * k * (1.0 / PI);
}

float filament_DGGX2(float NoH, float a) {
    float a2 = a * a;
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
    // float a2 = roughness * roughness;
    // float GGXV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    // float GGXL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    // return min(0.5 / (GGXV + GGXL), 65504.0);

    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

float filament_SmithGGXCorrelated(float NoV, float NoL, float a) {
    float a2 = a * a;
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return 0.5 / max(GGXV + GGXL, 1e-5);
}

float filament_DLambert() {
    return 1.0 / PI;
}

float filament_Burley(float roughness, float NoV, float NoL, float LoH) {
    // Burley 2012, "Physically-Based Shading at Disney"
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = filament_Schlick(1.0, f90, NoL);
    float viewScatter  = filament_Schlick(1.0, f90, NoV);
    return lightScatter * viewScatter * (1.0 / PI);
}

