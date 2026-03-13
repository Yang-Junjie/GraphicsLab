#version 460 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

out vec4 FragColor;

uniform sampler2D u_FloorTexture;
uniform vec3 u_LightPos;
uniform vec3 u_ViewPos;
uniform vec3 u_LightColor;
uniform float u_Shininess;
uniform float u_AmbientStrength;
uniform float u_SpecularStrength;
uniform int u_BlinnPhong; // 0 = Phong, 1 = Blinn-Phong

void main()
{
    vec3 color = texture(u_FloorTexture, v_TexCoord).rgb;
    vec3 normal = normalize(v_Normal);
    vec3 light_dir = normalize(u_LightPos - v_FragPos);
    vec3 view_dir = normalize(u_ViewPos - v_FragPos);

    // ambient
    vec3 ambient = u_AmbientStrength * color;

    // diffuse
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diff * u_LightColor * color;

    // specular
    float spec = 0.0;
    if (u_BlinnPhong == 1) {
        // Blinn-Phong: use halfway vector
        vec3 halfway_dir = normalize(light_dir + view_dir);
        spec = pow(max(dot(normal, halfway_dir), 0.0), u_Shininess);
    } else {
        // Phong: use reflect direction
        vec3 reflect_dir = reflect(-light_dir, normal);
        spec = pow(max(dot(view_dir, reflect_dir), 0.0), u_Shininess);
    }
    vec3 specular = u_SpecularStrength * spec * u_LightColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
