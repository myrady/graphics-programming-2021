#version 330 core
// VERTEX SHADER


layout (location = 1) in vec3 color;


// TODO voronoi 1.3
// Receives position in 'vec3', the position variable has attribute 'location = 0'
// CODE HERE
layout (location = 0) in vec3 pos;

// You have to declare an 'out float' to send the z-coordinate of the position
// to the fragment shader (voronoi 1.4 and 1.5)
// CODE HERE
out float depth;

// You have to set an 'uniform vec2' to receive the position offset of the object
// CODE HERE
 in vec2 offset;    //uniform

out vec3 outColor;


void main()
{
    // TODO voronoi 1.3
    // Set the vertex->fragment shader 'out' variable
    // CODE HERE
    // Set the 'gl_Position' built-in variable using a 'vec4(vec3 position you compute, 1.0)',
    // Remeber to use the 'uniform vec2' to move the vertex before you set 'gl_Position'.
    // CODE HERE

     //offset = vec2(pos.x, pos.y);
     gl_Position = vec4(pos.x + offset.x, pos.y + offset.y, pos.z, 1.0);
     depth = pos.z;
     outColor = color;
}