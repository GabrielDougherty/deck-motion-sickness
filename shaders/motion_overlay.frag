#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

// Push constants for dynamic resolution and motion
layout(push_constant) uniform PushConstants {
    float aspectRatio;  // width / height
    float time;         // elapsed time in seconds
    float velocityX;    // horizontal motion velocity
    float velocityY;    // vertical motion velocity
} pc;

void main() {
    // Correct UV coordinates to make circles truly circular
    vec2 correctedUV = fragUV;
    correctedUV.x *= pc.aspectRatio;
    
    // Dot parameters
    float radius = 0.05;
    float spacing = 0.2;  // Space between dots
    float edgeDistance = 0.1;  // Distance from edge
    
    // Calculate motion-based offset
    // Move dots in OPPOSITE direction of device motion
    // If device moves right (positive velocityX), dots move left (negative offset)
    float baseSpeed = 0.1;  // Base animation speed when no motion detected
    float motionScale = 2.0;  // How much to scale the motion effect
    
    // Use velocity to create counter-motion, fall back to base animation if no motion
    float velocityMagnitude = length(vec2(pc.velocityX, pc.velocityY));
    float offsetX = 0.0;
    float offsetY = 0.0;
    
    if (velocityMagnitude > 0.01) {
        // Motion detected - move opposite to velocity
        offsetX = -pc.velocityX * motionScale * pc.time;
        offsetY = -pc.velocityY * motionScale * pc.time;
    } else {
        // No motion detected - use base animation
        offsetX = mod(pc.time * baseSpeed, spacing);
    }
    
    // Wrap offsets to spacing interval
    offsetX = mod(offsetX, spacing);
    offsetY = mod(offsetY, spacing);
    
    float maxAlpha = 0.0;
    
    // Top edge dots
    for (float x = edgeDistance - spacing; x <= 1.0; x += spacing) {
        vec2 dotPos = vec2((x + offsetX) * pc.aspectRatio, edgeDistance + offsetY);
        float dist = distance(correctedUV, dotPos);
        float alpha = smoothstep(radius + 0.01, radius, dist);
        maxAlpha = max(maxAlpha, alpha);
    }
    
    // Bottom edge dots
    for (float x = edgeDistance - spacing; x <= 1.0; x += spacing) {
        vec2 dotPos = vec2((x + offsetX) * pc.aspectRatio, 0.9 + offsetY);
        float dist = distance(correctedUV, dotPos);
        float alpha = smoothstep(radius + 0.01, radius, dist);
        maxAlpha = max(maxAlpha, alpha);
    }
    
    // Red color with alpha
    outColor = vec4(1.0, 0.0, 0.0, maxAlpha * 0.5);  // 50% transparent red
}
