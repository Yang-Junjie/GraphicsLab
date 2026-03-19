#version 460 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

layout(location = 0) out vec4 g_Position;
layout(location = 1) out vec4 g_Normal;
layout(location = 2) out vec4 g_AlbedoSpec;

uniform sampler2D u_AlbedoTexture;

void main()
{
    g_Position = vec4(v_FragPos, 1.0);
    g_Normal = vec4(normalize(v_Normal) * 0.5 + 0.5, 1.0);
    g_AlbedoSpec.rgb = texture(u_AlbedoTexture, v_TexCoord).rgb;
    g_AlbedoSpec.a = 1.0;
}
