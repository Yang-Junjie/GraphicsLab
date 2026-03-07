#version 430 core
layout(location = 0) in vec2 a_Pos;

struct QuadInstance {
    vec4 color;
    vec2 position;
    vec2 size;
    float rotation;
    float _pad[3];
};

layout(std430, binding = 0) readonly buffer QuadSSBO {
    QuadInstance instances[];
};

uniform mat4 u_Proj;
out vec4 v_Color;

void main() {
    QuadInstance q = instances[gl_InstanceID];
    float c = cos(q.rotation);
    float s = sin(q.rotation);
    vec2 scaled = a_Pos * q.size;
    vec2 rotated = vec2(scaled.x * c - scaled.y * s,
                        scaled.x * s + scaled.y * c);
    gl_Position = u_Proj * vec4(rotated + q.position, 0.0, 1.0);
    v_Color = q.color;
}
