#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    // Four dots - one in each corner
    vec2 topLeft = vec2(0.1, 0.1);
    vec2 topRight = vec2(0.9, 0.1);
    vec2 bottomLeft = vec2(0.1, 0.9);
    vec2 bottomRight = vec2(0.9, 0.9);
    
    // Distance from each dot
    float distTL = distance(fragUV, topLeft);
    float distTR = distance(fragUV, topRight);
    float distBL = distance(fragUV, bottomLeft);
    float distBR = distance(fragUV, bottomRight);
    
    // Red dot with radius 0.05 (5% of screen)
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
