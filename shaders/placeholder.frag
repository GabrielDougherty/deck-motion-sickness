#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    // Center point in UV space
    vec2 center = vec2(0.5, 0.5);
    
    // Distance from center
    float dist = distance(fragUV, center);
    
    // Red dot with radius 0.05 (5% of screen)
    float radius = 0.05;
    
    // Smooth edge with antialiasing
    float alpha = smoothstep(radius + 0.01, radius, dist);
    
    // Red color with alpha
    outColor = vec4(1.0, 0.0, 0.0, alpha * 0.5);  // 50% transparent red
}
