#version 460 core
layout(location = 0) in vec2 a_Pos;
layout(location = 1) in vec2 a_TexCoord;

uniform mat4 u_Proj;
uniform mat4 u_Model;

out vec2 v_TexCoord;

void main() {
    gl_Position = u_Proj * u_Model * vec4(a_Pos, 0.0, 1.0);
    v_TexCoord = a_TexCoord;
}
