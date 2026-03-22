#version 460 core

in vec3 v_LocalPos;

layout(location = 0) out vec4 FragColor;

uniform sampler2D u_EquirectangularMap;

const vec2 kInvAtan = vec2(0.15915494, 0.31830989);

vec2 SampleSphericalMap(vec3 direction)
{
    vec2 uv = vec2(atan(direction.z, direction.x), asin(clamp(direction.y, -1.0, 1.0)));
    uv *= kInvAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec3 direction = normalize(v_LocalPos);
    vec2 uv = SampleSphericalMap(direction);
    vec3 color = texture(u_EquirectangularMap, uv).rgb;
    FragColor = vec4(color, 1.0);
}
