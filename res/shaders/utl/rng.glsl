
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

vec3 hemisphere_random(vec3 v, inout uint rng) {
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

vec3 sphere_random(inout uint rng) {
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
            return n;
        }
    }
    return n;
}

uint tea(uint val0, uint val1)
{
  uint v0 = val0;
  uint v1 = val1;
  uint s0 = 0;

  for(uint n = 0; n < 16; n++)
  {
    s0 += 0x9e3779b9;
    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
  }

  return v0;
}

uint lcg(inout uint prev)
{
  uint LCG_A = 1664525u;
  uint LCG_C = 1013904223u;
  prev       = (LCG_A * prev + LCG_C);
  return prev & 0x00FFFFFF;
}

float rnd(inout uint prev)
{
  return (float(lcg(prev)) / float(0x01000000));
}