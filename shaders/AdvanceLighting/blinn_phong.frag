#version 460 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec4 v_FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D u_FloorTexture;
uniform sampler2D u_ShadowMap;
uniform vec3 u_LightPos;
uniform vec3 u_ViewPos;
uniform vec3 u_LightColor;
uniform float u_Shininess;
uniform float u_AmbientStrength;
uniform float u_SpecularStrength;
uniform int u_BlinnPhong; // 0 = Phong, 1 = Blinn-Phong
uniform int u_UseGammaCorrection;
uniform int u_EnableShadows;
uniform int u_PCFregionSize;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // 将片段位置从齐次裁剪空间转换到纹理坐标空间
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // 将范围从 [-1, 1] 转换到 [0, 1]
    projCoords = projCoords * 0.5 + 0.5;

    // 处理光锥体外的区域
    if (projCoords.z > 1.0)
        return 0.0;

    // 获取阴影贴图中存储的最近深度
    float closestDepth = texture(u_ShadowMap, projCoords.xy).r; 

    // 获取当前像素的深度
    float currentDepth = projCoords.z;

    // 计算阴影偏移，防止阴影 acne
    float bias =  max(0.05 * (1.0 - dot(normalize(v_Normal), normalize(u_LightPos - v_FragPos))), 0.005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
    for(int x = -u_PCFregionSize/2; x <= u_PCFregionSize/2; ++x)
    {
        for(int y = -u_PCFregionSize/2; y <= u_PCFregionSize/2; ++y)
        {
            float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            // shadow test
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= u_PCFregionSize * u_PCFregionSize; // 平均阴影值

    return shadow;
}

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

    // shadow
    float shadow = 0.0;
    if (u_EnableShadows == 1) {
        shadow = ShadowCalculation(v_FragPosLightSpace);
    }

    vec4 final_color = vec4(ambient + (1.0 - shadow) * (diffuse + specular), 1.0);
    if (u_UseGammaCorrection == 1) {
        final_color.rgb = pow(final_color.rgb, vec3(1.0 / 2.2));
    }
    FragColor = final_color;
}
