
vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

vec3 to_srgb(vec3 color)
{
    return sqrt(color);
    // return pow(color,vec3(1./2.2));
}

float rgb_to_luminance(vec3 color)
{
    // return dot(color, vec3(0.01599, 0.01082, 0.114));
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

float karis_average(vec3 color) {
    float l = rgb_to_luminance(to_srgb(color)) * 0.25;
    return l / (1. + l);
}

vec3 white_balance(vec3 color, vec3 w0, vec3 w1, vec3 c0, vec3 c1, float tempurature) {
    float gs = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    if (tempurature > 0.0) {
        return mix(color, mix(w0, w1, gs), tempurature);
    } else {
        return mix(color, mix(c0, c1, gs), -tempurature);
    }
}


