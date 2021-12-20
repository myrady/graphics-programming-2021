#version 330 core
in float lenColorScale;

out vec4 fragColor;

uniform bool renderLines;

void main() {
    float alpha = 1.0;
    if(!renderLines){
        // if rendering points, apply alpha falloff
        vec2 dist = (gl_PointCoord - .5) * 2.0;
        alpha -= dot(dist,dist);
    }

    fragColor = vec4(0.83, 0.8, 1.0, 0.95 * alpha * lenColorScale);
}
