#version 430 core

layout(location = 0) in vec2 a_Pos;
layout(location = 1) in vec4 a_Color;

uniform mat4 u_Proj;

out vec4 v_Color;

void main()
{
    gl_Position = u_Proj * vec4(a_Pos, 0.0, 1.0);
    v_Color = a_Color;
}
