#version 460 core

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

uniform vec3 u_CameraPos;
uniform vec3 u_LightDirection;
uniform vec3 u_LightColor;
uniform vec3 u_AmbientColor;

uniform vec4 u_BaseColorFactor;
uniform sampler2D u_BaseColorTexture;
uniform int u_UseBaseColorTexture;

out vec4 FragColor;

void main()
{
    vec4 albedo = u_BaseColorFactor;
    if (u_UseBaseColorTexture == 1) {
        albedo *= texture(u_BaseColorTexture, v_TexCoord);
    }

    vec3 N = normalize(v_Normal);
    vec3 L = normalize(-u_LightDirection);
    vec3 V = normalize(u_CameraPos - v_WorldPos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 64.0);

    vec3 ambient = albedo.rgb * u_AmbientColor;
    vec3 diffuse = albedo.rgb * u_LightColor * NdotL;
    vec3 specular = u_LightColor * spec * 0.12;

    FragColor = vec4(ambient + diffuse + specular, albedo.a);
}
