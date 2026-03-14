#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_LightSpaceMatrix;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec4 v_FragPosLightSpace;

void main()
{
    vec4 world_pos = u_model * vec4(a_Position, 1.0);
    v_FragPos = world_pos.xyz;
    v_Normal = mat3(transpose(inverse(u_model))) * a_Normal;
    v_TexCoord = a_TexCoord;
    v_FragPosLightSpace = u_LightSpaceMatrix * world_pos;

    gl_Position = u_projection * u_view * world_pos;
}
