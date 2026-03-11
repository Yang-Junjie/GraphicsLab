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
out vec2 v_Local;
out vec4 v_Color;

void main() {
    Particle p = particles[gl_InstanceID];
    vec2 world = a_Pos * p.size * 2.0 + p.position;
    gl_Position = u_Proj * vec4(world, 0.0, 1.0);
    v_Local = a_Pos * 2.0;

    float g = 0.5 + 0.5 * sin(p.position.x * 0.01 + u_Time * 0.7);
    float b = 0.5 + 0.5 * sin(p.position.y * 0.01 + u_Time * 1.1);
    v_Color = vec4(p.color.r, g, b, 0.8);
}
