#version 430 core
in vec2 v_Local;
in vec4 v_Color;
out vec4 FragColor;

void main() {
    float dist = dot(v_Local, v_Local);
    if (dist > 1.0) discard;
    float edge = fwidth(sqrt(dist));
    float alpha = 1.0 - smoothstep(1.0 - edge * 1.5, 1.0, sqrt(dist));
    FragColor = vec4(v_Color.rgb, v_Color.a * alpha);
}
