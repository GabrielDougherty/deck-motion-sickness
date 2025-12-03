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
    
    // Four dots - one in each corner (with aspect correction)
    vec2 topLeft = vec2(0.1 * pc.aspectRatio, 0.1);
    vec2 topRight = vec2(0.9 * pc.aspectRatio, 0.1);
    vec2 bottomLeft = vec2(0.1 * pc.aspectRatio, 0.9);
    vec2 bottomRight = vec2(0.9 * pc.aspectRatio, 0.9);
    
    // Distance from each dot
    float distTL = distance(correctedUV, topLeft);
    float distTR = distance(correctedUV, topRight);
    float distBL = distance(correctedUV, bottomLeft);
    float distBR = distance(correctedUV, bottomRight);
    
    // Red dot with radius 0.05 (5% of screen height)
    float radius = 0.05;
    
    // Smooth edge with antialiasing for each dot
    float alphaTL = smoothstep(radius + 0.01, radius, distTL);
    float alphaTR = smoothstep(radius + 0.01, radius, distTR);
    float alphaBL = smoothstep(radius + 0.01, radius, distBL);
    float alphaBR = smoothstep(radius + 0.01, radius, distBR);
    
    // Combine all dots (max to avoid overlap issues)
    float alpha = max(max(alphaTL, alphaTR), max(alphaBL, alphaBR));
    
    // Red color with alpha
    outColor = vec4(1.0, 0.0, 0.0, alpha * 0.5);  // 50% transparent red
}
