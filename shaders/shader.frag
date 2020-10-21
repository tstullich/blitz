#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

struct Light {
    vec3 position;
    vec3 color;
};

vec3 diffuse(in vec3 materialDiffuse, in Light light) {
    vec3 L = light.position - fragPosition;
    float nDotL = dot(fragNormal, L);

    return materialDiffuse * light.color * nDotL;
}

vec3 specular(in vec3 materialDiffuse, in Light light) {
    vec3 matSpec = vec3(1.0f);
    vec3 eyePos = vec3(0.0f);
    vec3 L = light.position - fragPosition;
    vec3 V = eyePos - fragPosition;

    vec3 nRefL = reflect(L, fragNormal);
    float refDotV = dot(nRefL, V);

    return matSpec *  light.color * pow(refDotV, 0.2f);
}

void main() {
    Light light = Light(vec3(2.5f, 1.0f, 1.0f), vec3(1.0f)); // random position and color for light
    vec4 matAlbedo = texture(texSampler, fragTexCoord);

    vec3 diff = diffuse(matAlbedo.rgb, light);
    //vec3 spec = specular(matAlbedo.rgb, light);

    outColor = vec4(diff, 1.0f); // Ignore the alpha component for now
}