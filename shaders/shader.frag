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

vec3 diffuse(in vec3 materialDiffuse, in vec3 normal, in Light light) {
    vec3 L = light.position - fragPosition;
    float nDotL = clamp(dot(normal, L), 0.0, 1.0);

    return materialDiffuse * light.color * nDotL;
}

vec3 specular(in vec3 materialSpecular, in Light light) {
    vec3 matSpec = vec3(1.0);
    vec3 eyePos = vec3(0.0);
    vec3 L = light.position - fragPosition;
    vec3 V = eyePos - fragPosition;

    vec3 nRefL = reflect(L, fragNormal);
    float refDotV = dot(nRefL, V);

    return materialSpecular *  light.color * pow(refDotV, 0.2);
}

void main() {
    Light light = Light(vec3(2.0), vec3(1.0)); // random position and color for light
    vec4 matAlbedo = texture(texSampler, fragTexCoord);

    vec3 diff = diffuse(matAlbedo.rgb, fragNormal, light);
    //vec3 spec = specular(matAlbedo.rgb, light);

    outColor = vec4(diff, 1.0); // Ignore the alpha component for now
}