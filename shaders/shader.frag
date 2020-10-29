#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Camera {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalTransform;
    vec3 position;
} cam;

layout(binding = 1) uniform Light {
    vec3 position;
    vec3 emissiveColor;
} light;

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

vec3 diffuse(in vec3 materialDiffuse, in vec3 normal) {
    vec3 L = normalize(light.position - fragPosition);
    float nDotL = max(0.0, dot(normal, L));

    return materialDiffuse * light.emissiveColor * nDotL;
}

vec3 specular(in vec3 materialSpecular, in vec3 normal) {
    vec3 matSpec = vec3(1.0);
    vec3 eyePos = normalize(cam.position);

    vec3 L = normalize(light.position - fragPosition);
    vec3 V = normalize(eyePos - fragPosition);
    vec3 B = normalize(L + V);

    float nDotB = max(0.0, dot(normal, B));

    return materialSpecular * pow(nDotB, 100);
}

void main() {
    vec4 matAlbedo = texture(texSampler, fragTexCoord);
    vec3 diff = diffuse(matAlbedo.rgb, fragNormal);
    vec3 spec = specular(matAlbedo.rgb, fragNormal);

    vec3 distanceToLight = light.position - fragPosition;
    float d = length(distanceToLight);
    float attenuation = clamp(10.0 / d, 0.0, 1.0);

    vec3 finalColor = attenuation * clamp(diff + spec, 0.0, 1.0);
    outColor = vec4(finalColor, 1.0); // Ignore the alpha component for now
}