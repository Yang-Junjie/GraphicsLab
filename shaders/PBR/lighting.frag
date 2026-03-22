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
uniform sampler2D u_GEmissive;

uniform samplerCube u_EnvironmentMap;
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilterMap;
uniform sampler2D u_BrdfLut;

uniform PointLight u_Light;
uniform vec3 u_ViewPos;
uniform vec3 u_BackgroundColor;
uniform mat4 u_InverseView;
uniform mat4 u_InverseProjection;
uniform float u_AmbientIntensity;
uniform float u_IblIntensity;
uniform float u_EnvironmentIntensity;
uniform float u_BackgroundLod;
uniform float u_PrefilterMaxLod;
uniform float u_Exposure;
uniform int u_ShowEnvironment;
uniform int u_HasIBL;
uniform int u_DebugOutputMode;

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

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) *
                    pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ComputeWorldDirection(vec2 texCoord)
{
    vec2 ndc = texCoord * 2.0 - 1.0;
    vec4 clip = vec4(ndc, 1.0, 1.0);
    vec4 view = u_InverseProjection * clip;
    view = vec4(view.xy, -1.0, 0.0);
    return normalize((u_InverseView * view).xyz);
}

vec3 ToneMap(vec3 color)
{
    vec3 mapped = vec3(1.0) - exp(-max(color, vec3(0.0)) * u_Exposure);
    return pow(mapped, vec3(1.0 / 2.2));
}

vec3 EnvBRDFApprox(vec3 specularColor, float roughness, float NoV)
{
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    vec2 ab = vec2(-1.04, 1.04) * a004 + r.zw;
    return specularColor * ab.x + ab.y;
}

void main()
{
    vec4 position_sample = texture(u_GPosition, v_TexCoord);
    if (position_sample.a <= 0.001) {
        if (u_HasIBL == 1 && u_ShowEnvironment == 1) {
            vec3 direction = ComputeWorldDirection(v_TexCoord);
            vec3 environment = textureLod(u_EnvironmentMap, direction, u_BackgroundLod).rgb *
                               u_EnvironmentIntensity;
            FragColor = vec4(ToneMap(environment), 1.0);
        } else {
            FragColor = vec4(ToneMap(u_BackgroundColor), 1.0);
        }
        return;
    }

    vec3 frag_pos = position_sample.xyz;
    vec3 N = normalize(texture(u_GNormal, v_TexCoord).rgb * 2.0 - 1.0);
    vec3 V = normalize(u_ViewPos - frag_pos);
    vec3 R = reflect(-V, N);
    vec3 albedo = texture(u_GAlbedo, v_TexCoord).rgb;
    float metallic = clamp(texture(u_GMetallic, v_TexCoord).r, 0.0, 1.0);
    float roughness = clamp(texture(u_GRoughness, v_TexCoord).r, 0.045, 1.0);
    float ao = clamp(texture(u_GAO, v_TexCoord).r, 0.0, 1.0);
    vec3 emissive = texture(u_GEmissive, v_TexCoord).rgb;

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
    vec3 iblDiffuse = vec3(0.0);
    vec3 prefilteredColor = vec3(0.0);
    vec2 envBRDF = vec2(0.0);
    vec3 specularIbl = vec3(0.0);
    vec3 iblAmbient = vec3(0.0);

    if (u_HasIBL == 1) {
        float NoV = max(dot(N, V), 0.0);
        vec3 F_ibl = FresnelSchlickRoughness(NoV, F0, roughness);
        vec3 kS_ibl = F_ibl;
        vec3 kD_ibl = (vec3(1.0) - kS_ibl) * (1.0 - metallic);

        vec3 irradiance = texture(u_IrradianceMap, N).rgb * u_EnvironmentIntensity;
        iblDiffuse = irradiance * albedo;

        prefilteredColor =
            textureLod(u_PrefilterMap, R, roughness * u_PrefilterMaxLod).rgb *
            u_EnvironmentIntensity;
        envBRDF = texture(u_BrdfLut, vec2(NoV, roughness)).rg;

        vec3 lutSpecular = prefilteredColor * (F_ibl * envBRDF.x + envBRDF.y);
        vec3 approxSpecular = prefilteredColor * EnvBRDFApprox(F0, roughness, NoV);
        vec3 environmentFallback =
            textureLod(u_EnvironmentMap, R, roughness * u_PrefilterMaxLod).rgb *
            u_EnvironmentIntensity * F_ibl * (1.0 - 0.5 * roughness);
        specularIbl = max(max(lutSpecular, approxSpecular), environmentFallback);

        iblAmbient = (kD_ibl * iblDiffuse + specularIbl) * ao * u_IblIntensity;
        ambient = iblAmbient;
    }

    if (u_DebugOutputMode == 1) {
        FragColor = vec4(ToneMap(iblDiffuse * u_IblIntensity), 1.0);
        return;
    }
    if (u_DebugOutputMode == 2) {
        FragColor = vec4(ToneMap(prefilteredColor * u_IblIntensity), 1.0);
        return;
    }
    if (u_DebugOutputMode == 3) {
        vec3 brdfDebug = max(vec3(envBRDF, 0.0), EnvBRDFApprox(F0, roughness, max(dot(N, V), 0.0)));
        FragColor = vec4(brdfDebug, 1.0);
        return;
    }
    if (u_DebugOutputMode == 4) {
        FragColor = vec4(ToneMap(specularIbl * u_IblIntensity), 1.0);
        return;
    }
    if (u_DebugOutputMode == 5) {
        FragColor = vec4(ToneMap(iblAmbient), 1.0);
        return;
    }

    vec3 color = emissive + ambient + Lo;
    FragColor = vec4(ToneMap(color), 1.0);
}