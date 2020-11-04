#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Camera {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 position;
} cam;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    gl_Position = cam.projection * cam.view * cam.model * vec4(inPosition, 1.0f);
    fragPosition = inPosition;
    fragNormal = (transpose(inverse(cam.model)) * vec4(inNormal, 1.0f)).xyz;
    fragTexCoord = inTexCoord;
}