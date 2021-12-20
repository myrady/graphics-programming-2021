#version 330 core
out vec4 FragColor;
in  vec4 vtxColor;

void main()
{
   FragColor = vec4(vtxColor.xyz, 1.0);
}