#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

// Push constants for dynamic resolution and motion
layout(push_constant) uniform PushConstants {
    float aspectRatio;  // width / height
    float time;         // elapsed time in seconds
    float offsetX;      // integrated horizontal offset
    float offsetY;      // integrated vertical offset
} pc;

void main() {
    // Correct UV coordinates to make circles truly circular
    vec2 correctedUV = fragUV;
    correctedUV.x *= pc.aspectRatio;
    
    // Dot parameters
    float radius = 0.05;
    float spacing = 0.2;  // Space between dots
    float topEdgeY = 0.05;  // Y position of top edge dots (closer to top)
    float bottomEdgeY = 0.95;  // Y position of bottom edge dots (closer to bottom)
    float fadeWidth = 0.25;  // Width of fade zone (much more gradual)
    
    // Use integrated offsets directly (no wrapping)
    float offsetX = pc.offsetX;
    float offsetY = pc.offsetY;
    
    float maxAlpha = 0.0;
    
    // Top edge dots - extend range to cover extra dots that might scroll in
    for (float x = -2.0; x <= 2.0; x += spacing) {
        vec2 dotPos = vec2((x + offsetX) * pc.aspectRatio, topEdgeY + offsetY);
        float dist = distance(correctedUV, dotPos);
        float alpha = smoothstep(radius + 0.01, radius, dist);
        
        // Horizontal fade bands: fade above and below the top dot line
        float edgeFade = 1.0;
        
        // Fade zone above top dots (screen top to topEdgeY)
        if (correctedUV.y / pc.aspectRatio < topEdgeY) {
            float distFromTop = correctedUV.y / pc.aspectRatio;
            edgeFade *= smoothstep(topEdgeY - fadeWidth, topEdgeY, distFromTop);
        }
        // Fade zone below top dots (topEdgeY to topEdgeY + fadeWidth)
        if (correctedUV.y / pc.aspectRatio > topEdgeY && correctedUV.y / pc.aspectRatio < topEdgeY + fadeWidth) {
            float distFromTopEdge = correctedUV.y / pc.aspectRatio - topEdgeY;
            edgeFade *= smoothstep(fadeWidth, 0.0, distFromTopEdge);
        }
        
        maxAlpha = max(maxAlpha, alpha * edgeFade);
    }
    
    // Bottom edge dots
    for (float x = -2.0; x <= 2.0; x += spacing) {
        vec2 dotPos = vec2((x + offsetX) * pc.aspectRatio, bottomEdgeY + offsetY);
        float dist = distance(correctedUV, dotPos);
        float alpha = smoothstep(radius + 0.01, radius, dist);
        
        // Horizontal fade bands: fade above and below the bottom dot line
        float edgeFade = 1.0;
        
        // Fade zone above bottom dots (bottomEdgeY - fadeWidth to bottomEdgeY)
        if (correctedUV.y / pc.aspectRatio < bottomEdgeY && correctedUV.y / pc.aspectRatio > bottomEdgeY - fadeWidth) {
            float distFromBottomEdge = bottomEdgeY - correctedUV.y / pc.aspectRatio;
            edgeFade *= smoothstep(fadeWidth, 0.0, distFromBottomEdge);
        }
        // Fade zone below bottom dots (bottomEdgeY to screen bottom)
        if (correctedUV.y / pc.aspectRatio > bottomEdgeY) {
            float distFromBottom = 1.0 - correctedUV.y / pc.aspectRatio;
            edgeFade *= smoothstep(1.0 - bottomEdgeY - fadeWidth, 1.0 - bottomEdgeY, distFromBottom);
        }
        
        maxAlpha = max(maxAlpha, alpha * edgeFade);
    }
    
    // Red color with alpha
    outColor = vec4(1.0, 0.0, 0.0, maxAlpha * 0.5);  // 50% transparent red
}
