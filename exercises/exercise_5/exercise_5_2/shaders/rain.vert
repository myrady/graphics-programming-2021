#version 330 core

layout (location = 0) in vec3 initPosition;

out float lenColorScale;
uniform vec3 offsets, cameraPosition, forwardOffset, inverseVelocity;
uniform mat4 viewProj;
uniform mat4 prevViewProj;
uniform float boxSize;
uniform bool renderLines;

vec4 pointRender(vec3 position){
    // position in clipping space
    vec4 bottomPosition = viewProj * vec4(position, 1.0);
    gl_PointSize = mix(18.0, 1.0, bottomPosition.z/boxSize);
    lenColorScale = 1.0;
    return bottomPosition;
}


vec4 lineRender(vec3 position){
    // positions in clipping space
    vec4 bottomPosition = viewProj * vec4(position, 1.0);

    vec4 topPositionPrev =  prevViewProj * vec4(position + inverseVelocity, 1.0);
    vec4 topPosition =  viewProj * vec4(position + inverseVelocity, 1.0);

    // attenuation
    vec2 dir = topPosition.xy/topPosition.w - bottomPosition.xy/bottomPosition.w;
    vec2 dirPrev = topPositionPrev.xy/topPositionPrev.w - bottomPosition.xy/bottomPosition.w;

    float len = length(dir);
    float lenPrev = length(dirPrev);

    lenColorScale = clamp(len/lenPrev, 0.0, 1.0);

    // final position
    return mix(bottomPosition, topPositionPrev, mod(gl_VertexID, 2));
}



void main()
{
    // position in world space
    vec3 position = mod(initPosition + offsets, boxSize);
    position += cameraPosition + forwardOffset - boxSize/2.0;

    if(renderLines){
        gl_Position = lineRender(position);
    } else {
        gl_Position = pointRender(position);
    }

}