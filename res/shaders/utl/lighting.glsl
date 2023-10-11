struct Surface {
    vec3 point;
    vec3 albedo;
    vec3 normal;
    vec3 emissive;
    float occlusion;
    float roughness;
};

struct Lighting {
    vec3 diffuse;
    vec3 specular;
};

struct TotalLight {
    Lighting direct;
    Lighting indirect;
};

TotalLight InitLightSolution() {
    TotalLight light;
    light.direct.diffuse = vec3(0.0);
    light.direct.specular = vec3(0.0);
    light.indirect.specular = vec3(0.0);
    light.indirect.diffuse = vec3(0.0);
    return light;
}

struct DirectionalLight {
    vec4 direction;
    vec4 color;
};

struct PointLight {
    vec3 pos; 
    float range;
    vec3 col; 
    float power;
};

struct Environment {
    DirectionalLight sun;

    vec4 ambient_color;

    vec4 fog_color;

    // PointLight point_lights[32];

    float fog_density;

    // color correction
    float ambient_strength;
    float contrast;
    uint light_count;
    vec4 more_padding;
    
};

void apply_light(in Surface surface, in TotalLight light, inout vec3 color) {
    vec3 diffuse = light.direct.diffuse / 3.14159 + light.indirect.diffuse;
    vec3 specular = light.direct.specular + light.indirect.specular;
    color += surface.albedo * diffuse;
    color += specular;
    color += surface.emissive;
    color = max(vec3(0.0), color);
}

void directional_light(
    in DirectionalLight light, 
    in Surface surface, 
    inout TotalLight light_solution, 
    in float shadow
) {
    float nol = max(dot(surface.normal, light.direction.xyz), 0.0);

    light_solution.direct.diffuse += saturate(nol) * sqr(light.color.rgb) * shadow;
}

float point_light_attenuation(in float dist2, in float range2) {
    float dpr = dist2/max(0.0001, range2);
    dpr*=dpr;
    return saturate(1.0-dpr)/max(0.0001,dist2);
}

void point_light(
    in PointLight light, 
    in Surface surface, 
    inout TotalLight light_solution, 
    float shadow
) {
    vec3 L = light.pos.xyz - surface.point;
    vec3 l = normalize(L);
    float nol = max(0.0, dot(l, surface.normal));

    float d2 = dot(L,L);
    float r2 = light.range * light.range;
    float a = point_light_attenuation(d2, r2);
    light_solution.direct.diffuse += 300.0 * nol * shadow * (light.col.rgb) * light.power * a;
    // light_solution.direct.diffuse += shadow * light.col.rgb;
}