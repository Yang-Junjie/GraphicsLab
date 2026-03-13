#version 460 core

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight {
    int enabled;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    int enabled;
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    int enabled;
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
};

#define NR_POINT_LIGHTS 4

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
out vec4 FragColor;

uniform vec3 u_ViewPos;
uniform Material u_Material;
uniform DirLight u_DirLight;
uniform PointLight u_PointLights[NR_POINT_LIGHTS];
uniform SpotLight u_SpotLight;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 view_dir)
{
    if (light.enabled == 0) {
        return vec3(0.0);
    }

    vec3 light_dir = normalize(-light.direction);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), u_Material.shininess);

    vec3 albedo = vec3(texture(u_Material.diffuse, v_TexCoord));
    vec3 spec_map = vec3(texture(u_Material.specular, v_TexCoord));

    vec3 ambient = light.ambient * albedo;
    vec3 diffuse = light.diffuse * diff * albedo;
    vec3 specular = light.specular * spec * spec_map;
    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_dir)
{
    if (light.enabled == 0) {
        return vec3(0.0);
    }

    vec3 light_dir = normalize(light.position - frag_pos);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), u_Material.shininess);

    float distance = length(light.position - frag_pos);
    float attenuation =
        1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    vec3 albedo = vec3(texture(u_Material.diffuse, v_TexCoord));
    vec3 spec_map = vec3(texture(u_Material.specular, v_TexCoord));

    vec3 ambient = light.ambient * albedo;
    vec3 diffuse = light.diffuse * diff * albedo;
    vec3 specular = light.specular * spec * spec_map;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return ambient + diffuse + specular;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 frag_pos, vec3 view_dir)
{
    if (light.enabled == 0) {
        return vec3(0.0);
    }

    vec3 light_dir = normalize(light.position - frag_pos);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), u_Material.shininess);

    float distance = length(light.position - frag_pos);
    float attenuation =
        1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    float theta = dot(light_dir, normalize(-light.direction));
    float epsilon = max(light.cutOff - light.outerCutOff, 0.0001);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 albedo = vec3(texture(u_Material.diffuse, v_TexCoord));
    vec3 spec_map = vec3(texture(u_Material.specular, v_TexCoord));

    vec3 ambient = light.ambient * albedo;
    vec3 diffuse = light.diffuse * diff * albedo;
    vec3 specular = light.specular * spec * spec_map;

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return ambient + diffuse + specular;
}

void main()
{
    vec3 norm = normalize(v_Normal);
    vec3 view_dir = normalize(u_ViewPos - v_FragPos);

    vec3 result = CalcDirLight(u_DirLight, norm, view_dir);
    for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
        result += CalcPointLight(u_PointLights[i], norm, v_FragPos, view_dir);
    }
    result += CalcSpotLight(u_SpotLight, norm, v_FragPos, view_dir);

    FragColor = vec4(result, 1.0);
}
