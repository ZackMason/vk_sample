
uvec3 index_1d(uvec3 dim, uint idx) {
    const uint z = idx / (dim.x * dim.y);
	idx -= (z * dim.x * dim.y);
	const uint y = idx / dim.x;
	const uint x = idx % dim.x;
	return  uvec3(x, y, z);
}

uint index_3d(uvec3 dim, uvec3 i) {
    return i.x + i.y * dim.x + i.z * dim.x * dim.y;
}

float saturate(float a) {
    return clamp(a, 0.0, 1.0);
}

vec3 saturate(vec3 a) {
    return clamp(a, vec3(0.0), vec3(1.0));
}

vec3 saturate_min(vec3 a, float mi) {
    return clamp(a, vec3(mi), vec3(1.0));
}

float sqr(float x) {
    return x*x;
}
vec3 sqr(vec3 x) {
    return x*x;
}

const uint PROBE_RAY_MAX = 256;
const uint PROBE_IRRADIANCE_DIM = 8;
const uint PROBE_VISIBILITY_DIM = 16;
const uint PROBE_PADDING = 1;
const uint PROBE_IRRADIANCE_TOTAL = PROBE_IRRADIANCE_DIM + 2 * PROBE_PADDING;
const uint PROBE_VISIBILITY_TOTAL = PROBE_VISIBILITY_DIM + 2 * PROBE_PADDING;

uint probe_ray_start(uint index, uint odd_frame) {
	return index * PROBE_RAY_MAX + odd_frame * PROBE_RAY_MAX * 10000;
}

vec2 sign_not_zero(vec2 v)
{
	return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}

vec2 DDGIGetNormalizedOctahedralCoordinates(ivec2 texCoords, int numTexels)
{
    vec2 octahedralTexelCoord = vec2(texCoords.x % numTexels, texCoords.y % numTexels);

    octahedralTexelCoord.xy += 0.5f;

    // Normalize
    octahedralTexelCoord.xy /= float(numTexels);

    // Shift to [-1, 1);
    octahedralTexelCoord *= 2.f;
    octahedralTexelCoord -= vec2(1.f, 1.f);

    return octahedralTexelCoord;
}


vec2 encode_oct(in vec3 v)
{
	// Project the sphere onto the octahedron, and then onto the xy plane
	vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
	// Reflect the folds of the lower hemisphere over the diagonals
	return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * sign_not_zero(p)) : p;
}

vec3 decode_oct(vec2 e)
{
	vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * sign_not_zero(v.xy);
	return normalize(v);
}

vec2 normalized_oct_coord(ivec2 frag_coord, int probe_side_length)
{
    int probe_with_border_side = probe_side_length + 2;

    vec2 oct_frag_coord = ivec2((frag_coord.x - 2) % probe_with_border_side, (frag_coord.y - 2) % probe_with_border_side);
    // Add back the half pixel to get pixel center normalized coordinates
    return (vec2(oct_frag_coord) + vec2(0.5f)) * (2.0f / float(probe_side_length)) - vec2(1.0f, 1.0f);
}

vec3 probe_position(uvec3 probe_coord, vec3 grid_size, vec3 min_pos) {
	return min_pos + grid_size * vec3(probe_coord);
}

uvec2 probe_color_pixel(uvec3 probeCoord, uvec3 dim)
{
	return probeCoord.xz * PROBE_IRRADIANCE_TOTAL + uvec2(probeCoord.y * dim.x * PROBE_IRRADIANCE_TOTAL, 0) + 1;
}


ivec2 probe_color_texel(uvec3 probeCoord, vec3 direction, uvec3 dim)
{
	vec2 pixel = probe_color_pixel(probeCoord, dim);
	pixel += (encode_oct(normalize(direction)) * 0.5 + 0.5) * PROBE_IRRADIANCE_DIM;
	return ivec2(pixel);
}

vec2 probe_color_uv(uvec3 probeCoord, vec3 direction, uvec3 dim, vec2 texture_resolution_rcp)
{
	vec2 pixel = probe_color_pixel(probeCoord, dim);
	pixel += (encode_oct(normalize(direction)) * 0.5 + 0.5) * PROBE_IRRADIANCE_DIM;
	return pixel * texture_resolution_rcp;
}

uvec2 probe_depth_pixel(uvec3 probeCoord, uvec3 dim)
{
	return probeCoord.xz * PROBE_VISIBILITY_TOTAL + uvec2(probeCoord.y * dim.x * PROBE_VISIBILITY_TOTAL, 0) + 1;
}

vec2 probe_depth_uv(uvec3 probeCoord, vec3 direction, uvec3 dim, vec2 texture_resolution_rcp)
{
	vec2 pixel = probe_depth_pixel(probeCoord, dim);
	pixel += (encode_oct(normalize(direction)) * 0.5 + 0.5) * PROBE_VISIBILITY_DIM;
	return pixel * texture_resolution_rcp;
}

ivec2 probe_depth_texel(uvec3 probeCoord, vec3 direction, uvec3 dim)
{
	vec2 pixel = probe_depth_pixel(probeCoord, dim);
	pixel += (encode_oct(normalize(direction)) * 0.5 + 0.5) * PROBE_VISIBILITY_DIM;
	return ivec2(pixel);
}
