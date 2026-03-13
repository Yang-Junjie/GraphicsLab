#version 460 core

in vec2 v_TexCoord;

out vec4 FragColor;

uniform sampler2D u_Texture;

// float near = 0.1; 
// float far  = 100.0; 

// float LinearizeDepth(float depth) 
// {
//     float z = depth * 2.0 - 1.0; // 转换为 NDC
//     return (2.0 * near * far) / (far + near - z * (far - near));    
// }

void main()
{
    FragColor = texture(u_Texture, v_TexCoord);
//    float depth = LinearizeDepth(gl_FragCoord.z) / far; // 为了演示除以 far
//    FragColor = vec4(vec3(depth), 1.0);
}
