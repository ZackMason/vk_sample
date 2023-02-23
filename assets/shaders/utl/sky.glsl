const float Br = 0.0025;
const float Bm = 0.0003;
const float g =  0.9800;
const vec3 nitrogen = vec3(0.650, 0.570, 0.475);
const vec3 Kr = Br / pow(nitrogen, vec3(4.0));
const vec3 Km = Bm / pow(nitrogen, vec3(0.84));

vec3 get_extinction(vec3 normal, vec3 sun) {
    float mu = dot(normalize(normal), sun);
    float rayleigh = 3.0 / (8.0 * 3.1415) * (1.0 + mu * mu);
    vec3 mie = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5)) / (Br + Bm);
    
    vec3 day_extinction = exp(-exp(-((normal.y + sun.y * 4.0) * (exp(-normal.y * 16.0) + 0.1) / 80.0) / Br) * (exp(-normal.y * 16.0) + 0.1) * Kr / Br) * exp(-normal.y * exp(-normal.y * 8.0 ) * 4.0) * exp(-normal.y * 2.0) * 4.0;
    vec3 night_extinction = vec3(1.0 - exp(sun.y)) * 0.2;
    return mix(day_extinction, night_extinction, -sun.y * 0.2 + 0.5);
}

vec3 rayleigh_scatter(vec3 normal, vec3 sun) {
    float mu = dot(normalize(normal), sun);
    float rayleigh = 3.0 / (8.0 * 3.1415) * (1.0 + mu * mu);
    vec3 mie = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5)) / (Br + Bm);
    
    vec3 day_extinction = exp(-exp(-((normal.y + sun.y * 4.0) * (exp(-normal.y * 16.0) + 0.1) / 80.0) / Br) * (exp(-normal.y * 16.0) + 0.1) * Kr / Br) * exp(-normal.y * exp(-normal.y * 8.0 ) * 4.0) * exp(-normal.y * 2.0) * 4.0;
    vec3 night_extinction = vec3(1.0 - exp(sun.y)) * 0.2;
    vec3 extinction = mix(day_extinction, night_extinction, -sun.y * 0.2 + 0.5);
    
    return rayleigh * mie * extinction;
}