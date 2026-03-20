#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec4 a_Tangent;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out mat3 v_TBN;

void main()
{
    vec4 world_pos = u_Model * vec4(a_Position, 1.0);
    mat3 normal_matrix = mat3(transpose(inverse(u_Model)));

    vec3 normal = normalize(normal_matrix * a_Normal);
    vec3 tangent = normalize(normal_matrix * a_Tangent.xyz);
    tangent = tangent - dot(tangent, normal) * normal;
    if (length(tangent) < 0.0001) {
        tangent = normalize(abs(normal.z) < 0.999 ? cross(normal, vec3(0.0, 0.0, 1.0))
                                                  : cross(normal, vec3(0.0, 1.0, 0.0)));
    } else {
        tangent = normalize(tangent);
    }

    vec3 bitangent = normalize(cross(normal, tangent)) * a_Tangent.w;

    v_FragPos = world_pos.xyz;
    v_Normal = normal;
    v_TexCoord = a_TexCoord;
    v_TBN = mat3(tangent, bitangent, normal);

    gl_Position = u_Projection * u_View * world_pos;
}
