#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

// Push constants for dynamic resolution
layout(push_constant) uniform PushConstants {
    float aspectRatio;  // width / height
} pc;

void main() {
    // Correct UV coordinates to make circles truly circular
    vec2 correctedUV = fragUV;
    correctedUV.x *= pc.aspectRatio;
    
    // Dot parameters
    float radius = 0.05;
    float spacing = 0.2;  // Space between dots
    float edgeDistance = 0.1;  // Distance from edge
    
    float maxAlpha = 0.0;
    
    // Top edge dots
    for (float x = edgeDistance; x <= 0.9; x += spacing) {
        vec2 dotPos = vec2(x * pc.aspectRatio, edgeDistance);
        float dist = distance(correctedUV, dotPos);
        float alpha = smoothstep(radius + 0.01, radius, dist);
        maxAlpha = max(maxAlpha, alpha);
    }
    
    // Bottom edge dots
    for (float x = edgeDistance; x <= 0.9; x += spacing) {
        vec2 dotPos = vec2(x * pc.aspectRatio, 0.9);
        float dist = distance(correctedUV, dotPos);
        float alpha = smoothstep(radius + 0.01, radius, dist);
        maxAlpha = max(maxAlpha, alpha);
    }
    
    // Red color with alpha
    outColor = vec4(1.0, 0.0, 0.0, maxAlpha * 0.5);  // 50% transparent red
}
