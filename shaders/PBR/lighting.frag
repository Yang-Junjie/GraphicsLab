#version 460 core

in vec2 v_TexCoord;

layout(location = 0) out vec4 FragColor;

const float PI = 3.14159265359;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform sampler2D u_GPosition;
uniform sampler2D u_GNormal;
uniform sampler2D u_GAlbedo;
uniform sampler2D u_GMetallic;
uniform sampler2D u_GRoughness;
uniform sampler2D u_GAO;

uniform PointLight u_Light;
uniform vec3 u_ViewPos;
uniform vec3 u_BackgroundColor;
uniform float u_AmbientIntensity;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denom * denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    return NdotV / max(NdotV * (1.0 - k) + k, 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec4 position_sample = texture(u_GPosition, v_TexCoord);
    if (position_sample.a <= 0.001) {
        FragColor = vec4(u_BackgroundColor, 1.0);
        return;
    }

    vec3 frag_pos = position_sample.xyz;
    vec3 N = normalize(texture(u_GNormal, v_TexCoord).rgb * 2.0 - 1.0);
    vec3 V = normalize(u_ViewPos - frag_pos);
    vec3 albedo = texture(u_GAlbedo, v_TexCoord).rgb;
    float metallic = clamp(texture(u_GMetallic, v_TexCoord).r, 0.0, 1.0);
    float roughness = clamp(texture(u_GRoughness, v_TexCoord).r, 0.045, 1.0);
    float ao = clamp(texture(u_GAO, v_TexCoord).r, 0.0, 1.0);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 Lo = vec3(0.0);

    vec3 L = normalize(u_Light.position - frag_pos);
    vec3 H = normalize(V + L);
    float distance_to_light = length(u_Light.position - frag_pos);
    float attenuation = 1.0 / max(distance_to_light * distance_to_light, 0.01);
    vec3 radiance = u_Light.color * u_Light.intensity * attenuation;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 0.001);
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    float NdotL = max(dot(N, L), 0.0);

    Lo += (kD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient = u_AmbientIntensity * albedo * ao;
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
