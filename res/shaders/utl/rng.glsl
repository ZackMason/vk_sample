
uint xorshift(inout uint state){
    uint x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return state;
}

vec3 hemisphere_random(vec3 v, float time) {
    uint rng = uint(time*1249252.75);
    vec3 n=vec3(0.0);
    while(true) {
        uint r0 = xorshift(rng);
        uint r1 = xorshift(rng);
        float rf0 = float(r0&0xffff)/float(0xffff);
        float rf1 = float((r0>>16)&0xffff)/float(0xffff);
        float rf2 = float(r1&0xffff)/float(0xffff);
        float rf3 = float((r1>>16)&0xffff)/float(0xffff);
        n = vec3(rf0, rf1, rf2) * 2.0 - 1.0;
        if (dot(n,n) < 1.0) {
            n = normalize(n);
            if (dot(n, v) < 0.0) {
                n = -n;
            }
            return n;
        }
    }
    return v;
}
