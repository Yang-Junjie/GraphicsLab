#version 460 core

in vec2 v_TexCoord;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

const int MAX_LIGHTS = 8;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform sampler2D u_GPosition;
uniform sampler2D u_GNormal;
uniform sampler2D u_GAlbedoSpec;
uniform sampler2D u_ShadowMap;

uniform mat4 u_LightSpaceMatrix;
uniform vec3 u_ViewPos;
uniform int u_ActiveLightCount;
uniform PointLight u_Lights[MAX_LIGHTS];
uniform float u_Shininess;
uniform float u_AmbientStrength;
uniform float u_SpecularStrength;
uniform int u_BlinnPhong;
uniform int u_UseGammaCorrection;
uniform int u_EnableShadows;
uniform int u_PCFregionSize;
uniform float u_BloomThreshold;

float ShadowCalculation(vec3 frag_pos, vec3 normal, vec3 light_pos)
{
    vec4 frag_pos_light_space = u_LightSpaceMatrix * vec4(frag_pos, 1.0);
    vec3 proj_coords = frag_pos_light_space.xyz / frag_pos_light_space.w;
    proj_coords = proj_coords * 0.5 + 0.5;

    if (proj_coords.z > 1.0) {
        return 0.0;
    }

    vec3 light_dir = normalize(light_pos - frag_pos);
    float bias = max(0.05 * (1.0 - dot(normalize(normal), light_dir)), 0.005);

    float shadow = 0.0;
    vec2 texel_size = 1.0 / textureSize(u_ShadowMap, 0);
    for (int x = -u_PCFregionSize / 2; x <= u_PCFregionSize / 2; ++x) {
        for (int y = -u_PCFregionSize / 2; y <= u_PCFregionSize / 2; ++y) {
            float pcf_depth = texture(u_ShadowMap, proj_coords.xy + vec2(x, y) * texel_size).r;
            shadow += proj_coords.z - bias > pcf_depth ? 1.0 : 0.0;
        }
    }
    shadow /= float(u_PCFregionSize * u_PCFregionSize);

    return shadow;
}

float LightAttenuation(float distance_to_light)
{
    return 1.0 / (1.0 + 0.09 * distance_to_light + 0.032 * distance_to_light * distance_to_light);
}

void main()
{
    vec3 frag_pos = texture(u_GPosition, v_TexCoord).xyz;
    vec4 albedo_spec = texture(u_GAlbedoSpec, v_TexCoord);
    if (albedo_spec.a <= 0.001) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 normal = normalize(texture(u_GNormal, v_TexCoord).xyz * 2.0 - 1.0);

    vec3 albedo = albedo_spec.rgb;
    float specular_mask = albedo_spec.a;
    vec3 view_dir = normalize(u_ViewPos - frag_pos);

    vec3 hdr_color = u_AmbientStrength * albedo;
    for (int i = 0; i < u_ActiveLightCount; ++i) {
        vec3 light_dir = normalize(u_Lights[i].position - frag_pos);
        float distance_to_light = length(u_Lights[i].position - frag_pos);
        float attenuation = LightAttenuation(distance_to_light);
        vec3 light_color = u_Lights[i].color * u_Lights[i].intensity * attenuation;

        float diff = max(dot(normal, light_dir), 0.0);
        vec3 diffuse = diff * light_color * albedo;

        float spec = 0.0;
        if (u_BlinnPhong == 1) {
            vec3 halfway_dir = normalize(light_dir + view_dir);
            spec = pow(max(dot(normal, halfway_dir), 0.0), u_Shininess);
        } else {
            vec3 reflect_dir = reflect(-light_dir, normal);
            spec = pow(max(dot(view_dir, reflect_dir), 0.0), u_Shininess);
        }
        vec3 specular = u_SpecularStrength * specular_mask * spec * light_color;

        float shadow = 0.0;
        if (u_EnableShadows == 1 && i == 0) {
            shadow = ShadowCalculation(frag_pos, normal, u_Lights[i].position);
        }

        hdr_color += (1.0 - shadow) * (diffuse + specular);
    }
    vec3 out_color = hdr_color;
    if (u_UseGammaCorrection == 1) {
        out_color = pow(out_color, vec3(1.0 / 2.2));
    }

    FragColor = vec4(out_color, 1.0);

    float brightness = dot(hdr_color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > u_BloomThreshold) {
        BrightColor = vec4(hdr_color, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
