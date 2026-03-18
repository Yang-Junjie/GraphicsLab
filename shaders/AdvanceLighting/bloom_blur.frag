#version 460 core

in vec2 v_TexCoord;
out vec4 FragColor;

uniform sampler2D u_Image;
uniform bool u_Horizontal;

// 5-tap Gaussian weights (sigma ≈ 1.5)
const float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 tex_offset = 1.0 / textureSize(u_Image, 0); // size of single texel
    vec3 result = texture(u_Image, v_TexCoord).rgb * weight[0];

    if (u_Horizontal) {
        for (int i = 1; i < 5; ++i) {
            result += texture(u_Image, v_TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(u_Image, v_TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            result += texture(u_Image, v_TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(u_Image, v_TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }

    FragColor = vec4(result, 1.0);
}
