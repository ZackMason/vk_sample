#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout( location = 0 ) in vec2 aVertex;
layout( location = 1 ) in vec2 aTexCoord;
layout( location = 2 ) in uint aTexID;
layout( location = 3 ) in uint aColor;

layout ( location = 0 ) out vec2 voUV;
layout ( location = 1 ) out uint voTexID;
layout ( location = 2 ) out vec4 voColor;

void
main() {
    voUV = aTexCoord.xy;
    voTexID = aTexID;
    voColor = vec4(
        float( aColor&0xff),
        float((aColor&0xff00)>>8),
        float((aColor&0xff0000)>>16),
        float((aColor&0xff000000)>>24))/255.0;
;
    vec2 pos = aVertex.xy * 2.0 - 1.0;
        
    gl_Position = vec4(pos, 0.00001, 1);
}