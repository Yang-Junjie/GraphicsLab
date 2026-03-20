#version 460 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in mat3 v_TBN;

layout(location = 0) out vec4 g_Position;
layout(location = 1) out vec4 g_Normal;
layout(location = 2) out vec4 g_Albedo;
layout(location = 3) out float g_Metallic;
layout(location = 4) out float g_Roughness;
layout(location = 5) out float g_AO;
layout(location = 6) out vec4 g_Emissive;

uniform vec4 u_BaseColorFactor;
uniform sampler2D u_BaseColorTexture;
uniform int u_UseBaseColorTexture;

uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;
uniform sampler2D u_MetallicRoughnessTexture;
uniform int u_UseMetallicRoughnessTexture;

uniform float u_NormalScale;
uniform sampler2D u_NormalTexture;
uniform int u_UseNormalTexture;

uniform float u_OcclusionStrength;
uniform sampler2D u_OcclusionTexture;
uniform int u_UseOcclusionTexture;

uniform vec3 u_EmissiveFactor;
uniform sampler2D u_EmissiveTexture;
uniform int u_UseEmissiveTexture;

void main()
{
    vec4 base_color = u_BaseColorFactor;
    if (u_UseBaseColorTexture == 1) {
        base_color *= texture(u_BaseColorTexture, v_TexCoord);
    }

    if (base_color.a <= 0.001) {
        discard;
    }

    vec3 normal = normalize(v_Normal);
    if (u_UseNormalTexture == 1) {
        vec3 tangent_normal = texture(u_NormalTexture, v_TexCoord).xyz * 2.0 - 1.0;
        tangent_normal.xy *= u_NormalScale;
        normal = normalize(v_TBN * tangent_normal);
    }

    float metallic = clamp(u_MetallicFactor, 0.0, 1.0);
    float roughness = clamp(u_RoughnessFactor, 0.045, 1.0);
    if (u_UseMetallicRoughnessTexture == 1) {
        vec3 metallic_roughness = texture(u_MetallicRoughnessTexture, v_TexCoord).rgb;
        roughness = clamp(metallic_roughness.g * u_RoughnessFactor, 0.045, 1.0);
        metallic = clamp(metallic_roughness.b * u_MetallicFactor, 0.0, 1.0);
    }

    float ao = 1.0;
    if (u_UseOcclusionTexture == 1) {
        float occlusion = texture(u_OcclusionTexture, v_TexCoord).r;
        ao = clamp(mix(1.0, occlusion, u_OcclusionStrength), 0.0, 1.0);
    }

    vec3 emissive = max(u_EmissiveFactor, vec3(0.0));
    if (u_UseEmissiveTexture == 1) {
        emissive *= texture(u_EmissiveTexture, v_TexCoord).rgb;
    }

    g_Position = vec4(v_FragPos, 1.0);
    g_Normal = vec4(normal * 0.5 + 0.5, 1.0);
    g_Albedo = vec4(clamp(base_color.rgb, 0.0, 1.0), 1.0);
    g_Metallic = metallic;
    g_Roughness = roughness;
    g_AO = ao;
    g_Emissive = vec4(emissive, 1.0);
}
