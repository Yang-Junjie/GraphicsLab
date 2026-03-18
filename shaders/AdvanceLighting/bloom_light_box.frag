#version 460 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

uniform vec3 u_LightColor;        // HDR emissive color (can be > 1.0)
uniform float u_BloomThreshold;   // luminance threshold for bright-pass

void main()
{
    vec3 hdr_color = u_LightColor;

    FragColor = vec4(hdr_color, 1.0);

    float brightness = dot(hdr_color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > u_BloomThreshold) {
        BrightColor = vec4(hdr_color, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}

