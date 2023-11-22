
float asfloat(uint x) {
    return uintBitsToFloat(x);
}

uint asuint(float x) {
    return floatBitsToUint(x);
}

uvec3 asuint(vec3 x) {
    return floatBitsToUint(x);
}

uint pack_normal(vec3 n) {
    uint result = 0;
    result |= uint((n.x*0.5+0.5)*255.0)<<0;
    result |= uint((n.y*0.5+0.5)*255.0)<<8;
    result |= uint((n.z*0.5+0.5)*255.0)<<16;
    return result;
}

vec3 unpack_normal(uint p) {
    vec3 n;
    n.x = float((p>>0)&0xff)/255.0*2.-1.;
    n.y = float((p>>8)&0xff)/255.0*2.-1.;
    n.z = float((p>>16)&0xff)/255.0*2.-1.;
    return n;
}

uint pack_rgbe(vec3 rgb) {
    const float max_val = asfloat(0x477f8000);
    const float min_val = asfloat(0x37800000);
    rgb = clamp(rgb, 0, max_val);

    float max_channel = max(max(min_val, rgb.r), max(rgb.g, rgb.b));

    float bias = asfloat((asuint(max_channel) + 0x07804000) & 0x7f800000);

    uvec3 RGB = asuint(rgb+bias);
    uint e = (asuint(bias) << 4) + 0x10000000;

    return e | RGB.b << 18 | RGB.g << 9 | (RGB.r & 0x1ff);
}

vec3 unpack_rgbe(uint p) {
    vec3 rgb = uvec3(p, p>>9,p>>18)&0x1ff;
    return ldexp(rgb, ivec3(p>>27)-24);
}

uint pack_half2(vec2 hf) {
    return packHalf2x16(hf);
}

vec2 unpack_half2(uint p) {
    return unpackHalf2x16(p);
}

uvec2 pack_half3(vec3 hf) {
    return uvec2(pack_half2(hf.xy), pack_half2(hf.zz));
}

vec3 unpack_half3(uvec2 p) {
    return vec3(unpack_half2(p.x), unpack_half2(p.y).x);
}

uvec2 pack_half4(vec4 hf) {
    return uvec2(packHalf2x16(hf.xy), packHalf2x16(hf.zw));
}

vec4 unpack_half4(uvec2 p) {
    return vec4(unpackHalf2x16(p.x), unpackHalf2x16(p.y));
}

uint pack_u16(uvec2 x) {
    return x.x&0xffff|((x.y&0xffff)<<16);
}

uvec2 unpack_u16(uint x) {
    return uvec2(x&0xff,(x>>16)&0xff);
}

uvec2 split_u64(uint64_t x) {
    return uvec2(x,(x>>32)) & 0xffffffff;
}

uvec2 split_u24(uint x) {
    return uvec2(x,(x>>12)) & 0x00ffffff;
}

vec4 rgba_to_color(uint rgba) {
    return vec4(
        float( rgba&0xff),
        float((rgba&0xff00)>>8),
        float((rgba&0xff0000)>>16),
        float((rgba&0xff000000)>>24))/255.0;
}