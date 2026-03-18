#version 460 core

in vec2 v_TexCoord;
out vec4 FragColor;

uniform sampler2D u_SceneColor;
uniform sampler2D u_BloomBlur;
uniform bool u_EnableBloom;
uniform float u_BloomIntensity;

uniform bool u_EnableHDR;
uniform int u_ToneMappingMode; // 0 = Reinhard, 1 = Exposure
uniform float u_Exposure;

uniform bool u_ApplyGamma;

void main()
{
    vec3 hdr_color = texture(u_SceneColor, v_TexCoord).rgb;

    if (u_EnableBloom) {
        vec3 bloom_color = texture(u_BloomBlur, v_TexCoord).rgb;
        hdr_color += bloom_color * u_BloomIntensity;
    }

    vec3 result = hdr_color;
    if (u_EnableHDR) {
        if (u_ToneMappingMode == 0) {
            // Reinhard tone mapping
            result = result / (result + vec3(1.0));
        } else {
            // Exposure tone mapping (LearnOpenGL default)
            result = vec3(1.0) - exp(-result * u_Exposure);
        }
    }

    // Gamma correction must be applied after tone mapping.
    // If the default framebuffer is sRGB and GL_FRAMEBUFFER_SRGB is enabled,
    // leave the output linear and let the framebuffer do the conversion.
    if (u_ApplyGamma) {
        result = pow(result, vec3(1.0 / 2.2));
    }

    FragColor = vec4(result, 1.0);
}
