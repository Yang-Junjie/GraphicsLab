#version 430 core
layout(location = 0) in vec2 a_Pos;

struct CircleInstance {
    vec4 color;
    vec2 position;
    float radius;
    float _pad;
};

layout(std430, binding = 1) readonly buffer CircleSSBO {
    CircleInstance instances[];
};

uniform mat4 u_Proj;
out vec2 v_Local;
out vec4 v_Color;

void main() {
    CircleInstance ci = instances[gl_InstanceID];
    vec2 world = a_Pos * ci.radius * 2.0 + ci.position;
    gl_Position = u_Proj * vec4(world, 0.0, 1.0);
    v_Local = a_Pos * 2.0;
    v_Color = ci.color;
}
