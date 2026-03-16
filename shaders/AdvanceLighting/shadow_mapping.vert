#version 460 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_model;
uniform mat4 u_LightSpaceMatrix;


void main()
{
    gl_Position = u_LightSpaceMatrix * u_model * vec4(a_Position, 1.0);
}
