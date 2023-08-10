float random (in vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(16.9898,98.233)))
                 * 43758.5453123);
}

vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

// 2D Noise based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise (in vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    // Smooth Interpolation

    // Cubic Hermine Curve.  Same as SmoothStep()
    vec2 u = f*f*(3.0-2.0*f);
    // u = smoothstep(0.,1.,f);

    // Mix 4 coorners percentages
    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}

float o_noise(in vec2 st, in int octave, in float s) {
    float r = 0.0;
    while (octave > 0) {
        r += noise(st) * s;
        s *= 0.5;
        octave -= 1;
    }
    return r;
}
float so_noise(in vec2 st, in int octave, in float s) {
    float r = 0.0;
    while (octave > 0) {
        r += noise(st*s) * s;
        s *= 0.5;
        octave -= 1;
    }
    return r;
}

float cell(in vec2 p, in float time) {
    float min_dist = 1000.0;
    vec2 i_st = floor(p);
    vec2 f_st = fract(p);

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 n = vec2(float(x), float(y));

            vec2 point = random2(i_st + n);

            point = 0.5 + 0.5 * sin(time + 6.285*point);

            float dist = length(n+point-f_st);

            min_dist = min(min_dist, dist);
        }
    }

    return min_dist;
}

vec2 voronoi( in vec2 x , in float time) {
    ivec2 p = ivec2(floor( x ));
    vec2  f = fract( x );

    vec2 res = vec2( 8.0 );
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        ivec2 b = ivec2(i, j);
        vec2 point = random2(vec2(p + b));
        point = 0.5 + 0.5 * sin(time + 6.285*point);
        vec2  r = vec2(b) - f + point;
        //r += 0.5 + 0.5 * sin(time+r*6.3);
        float d = dot(r, r);

        if( d < res.x )
        {
            res.y = res.x;
            res.x = d;
        }
        else if( d < res.y )
        {
            res.y = d;
        }
    }

    return sqrt( res );
}


vec2 voronoi( in vec2 x, in float time, out vec2 oA, out vec2 oB )
{
    ivec2 p = ivec2(floor( x ));
    vec2  f = fract( x );

    vec2 res = vec2( 8.0 );
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        ivec2 b = ivec2(i, j);
        vec2 point = random2(vec2(p + b));
        point = 0.5 + 0.5 * sin(time + 6.285*point);
        vec2  r = vec2(b) - f + point;
        float d = dot( r, r );

        if( d < res.x )
        {
            res.y = res.x;
            res.x = d;
            oA = r;
        }
        else if( d < res.y )
        {
            res.y = d;
            oB = r;
        }
    }

    return sqrt( res );
}


float voronoi2_edge( in vec2 p, in float time, in float edge)
{
    vec2 a, b;
    vec2 c = voronoi( p, time, a, b );

    float d = dot(0.5*(a+b),normalize(b-a));

    return 1.0 - smoothstep(0.0,edge,d);
}

float voronoi_edge( in vec2 p, in float time, in float edge) {
    vec2 c = voronoi( p , time);

    float dis = c.y - c.x;

    return 1.0 - smoothstep(0.0,edge,dis);
}

float fbm( in vec2 x, in float H )
{    
    float t = 0.0;
    for( int i=0; i<8; i++ )
    {
        float f = pow( 2.0, float(i) );
        float a = pow( f, -H );
        t += a*noise(f*x);
    }
    return t;
}
