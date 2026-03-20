#version 460 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

layout(location = 0) out vec4 g_Position;
layout(location = 1) out vec4 g_Normal;
layout(location = 2) out vec4 g_Albedo;
layout(location = 3) out float g_Metallic;
layout(location = 4) out float g_Roughness;
layout(location = 5) out float g_AO;

uniform vec3 u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AO;

void main()
{
    vec3 normal = normalize(v_Normal);

    g_Position = vec4(v_FragPos, 1.0);
    g_Normal = vec4(normal * 0.5 + 0.5, 1.0);
    g_Albedo = vec4(clamp(u_Albedo, 0.0, 1.0), 1.0);
    g_Metallic = clamp(u_Metallic, 0.0, 1.0);
    g_Roughness = clamp(u_Roughness, 0.045, 1.0);
    g_AO = clamp(u_AO, 0.0, 1.0);
}
