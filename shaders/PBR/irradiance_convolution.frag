#version 460 core

in vec3 v_LocalPos;

layout(location = 0) out vec4 FragColor;

uniform samplerCube u_EnvironmentMap;

const float PI = 3.14159265359;

void main()
{
    vec3 N = normalize(v_LocalPos);
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    vec3 irradiance = vec3(0.0);
    float sample_delta = 0.025;
    float sample_count = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += sample_delta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sample_delta) {
            vec3 tangent_sample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sample_vec = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * N;

            irradiance += texture(u_EnvironmentMap, sample_vec).rgb * cos(theta) * sin(theta);
            sample_count += 1.0;
        }
    }

    irradiance = PI * irradiance / max(sample_count, 1.0);
    FragColor = vec4(irradiance, 1.0);
}
