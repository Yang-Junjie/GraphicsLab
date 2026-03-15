#version 460 core

in vec3 v_WorldPos;
in vec3 v_Normal;

uniform vec3 u_ViewPos;
uniform vec3 u_LightPos;
uniform vec3 u_LightColor;
uniform vec3 u_SurfaceColor;
uniform float u_Shininess;

out vec4 FragColor;

void main()
{
    vec3 normal = normalize(v_Normal);
    // Make sure surface is visible from both sides
    if (!gl_FrontFacing)
        normal = -normal;

    // Ambient
    float ambient_strength = 0.15;
    vec3 ambient = ambient_strength * u_LightColor;

    // Diffuse
    vec3 light_dir = normalize(u_LightPos - v_WorldPos);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diff * u_LightColor;

    // Specular (Blinn-Phong)
    vec3 view_dir = normalize(u_ViewPos - v_WorldPos);
    vec3 halfway = normalize(light_dir + view_dir);
    float spec = pow(max(dot(normal, halfway), 0.0), u_Shininess);
    vec3 specular = 0.5 * spec * u_LightColor;

    vec3 result = (ambient + diffuse + specular) * u_SurfaceColor;
    FragColor = vec4(result, 1.0);
}
