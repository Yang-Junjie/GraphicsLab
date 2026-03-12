#version 460 core
layout(location = 0) in vec2 a_Pos;

struct Particle {
    vec4  color;
    vec2  position;
    vec2  velocity;
    float size;
    float _pad[3];
};

layout(std430, binding = 0) readonly buffer ParticleSSBO {
    Particle particles[];
};

uniform mat4  u_Proj;
uniform float u_Time;
out vec4 v_Color;

void main() {
    Particle p = particles[gl_InstanceID];
    vec2 scaled = a_Pos * vec2(p.size, p.size);
    gl_Position = u_Proj * vec4(scaled + p.position, 0.0, 1.0);

    float r = 0.5 + 0.5 * sin(p.position.x * 0.01 + u_Time);
    float g = 0.5 + 0.5 * sin(p.position.y * 0.01 + u_Time * 1.3);
    v_Color = vec4(r, g, p.color.b, 0.8);
}
