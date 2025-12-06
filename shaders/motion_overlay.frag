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
    // Dot parameters
    float topEdgeY = 0.10;  // Y position where fading starts at top
    float bottomEdgeY = 0.80;  // Y position where fading starts at bottom
    float fadeWidth = 0.25;  // Width of fade zone (much more gradual)
    
    // Early exit if in center zone where dots are completely invisible
    if (fragUV.y > topEdgeY + fadeWidth && fragUV.y < bottomEdgeY - fadeWidth) {
        outColor = vec4(0.0);
        return;
    }
    
    // Correct UV coordinates to make circles truly circular
    vec2 correctedUV = fragUV;
    correctedUV.x *= pc.aspectRatio;
    
    float radius = 0.03;  // Smaller radius (was 0.05)
    float spacing = 0.12;  // Closer spacing (was 0.2)
    
    // Use raw offsets - let dots scroll continuously off-screen
    float offsetX = pc.offsetX;
    float offsetY = pc.offsetY;
    
    // Wrap offsets for repeating pattern
    float wrappedOffsetX = mod(offsetX, spacing);
    float wrappedOffsetY = mod(offsetY, spacing);
    
    float maxAlpha = 0.0;
    
    // Create a continuous grid of dots - reduced range since dots are closer together
    for (float y = -0.5; y <= 1.5; y += spacing) {
        for (float x = -1.0; x <= 2.0; x += spacing) {
            float dotY = y + wrappedOffsetY;
            
            // Early exit: skip dots that are completely off-screen vertically
            if (dotY < -fadeWidth || dotY > 1.0 + fadeWidth) {
                continue;
            }
            
            vec2 dotPos = vec2((x + wrappedOffsetX) * pc.aspectRatio, y + wrappedOffsetY);
            float dist = distance(correctedUV, dotPos);
            // alpha: how close the pixel is to the dot center
            float alpha = smoothstep(radius + 0.01, radius, dist);
            
            // Performance optimization: skip if fragment is outside dot's circular radius
            if (alpha < 0.001) {
                continue;
            }
            
            // Calculate horizontal fade (left-right edges of screen)
            float horizontalFade = 1.0;
            float horizontalPos = (x + wrappedOffsetX) * pc.aspectRatio;
            float leftEdge = 0.0;
            float rightEdge = pc.aspectRatio;
            
            // Fade out at left edge
            if (horizontalPos < leftEdge + fadeWidth) {
                horizontalFade *= smoothstep(leftEdge, leftEdge + fadeWidth, horizontalPos);
            }
            // Fade out at right edge
            if (horizontalPos > rightEdge - fadeWidth) {
                horizontalFade *= smoothstep(rightEdge, rightEdge - fadeWidth, horizontalPos);
            }
            
            // Vertical fade: fade at top and bottom, completely fade out center
            float verticalFade = 1.0;
            // dotY already calculated above for early exit
            
            // Fade zone at top (above topEdgeY fades out towards screen top)
            if (dotY < topEdgeY) {
                float distFromTop = dotY;
                verticalFade *= smoothstep(topEdgeY - fadeWidth, topEdgeY, distFromTop);
            }
            // Fade zone at bottom (below bottomEdgeY fades out towards screen bottom)
            else if (dotY > bottomEdgeY) {
                float distFromBottom = 1.0 - dotY;
                verticalFade *= smoothstep(1.0 - bottomEdgeY - fadeWidth, 1.0 - bottomEdgeY, distFromBottom);
            }
            // Center area - fade out completely
            else {
                // Calculate distance from nearest edge (top or bottom)
                float distFromTopEdge = dotY - topEdgeY;
                float distFromBottomEdge = bottomEdgeY - dotY;
                float distFromNearestEdge = min(distFromTopEdge, distFromBottomEdge);
                verticalFade *= smoothstep(fadeWidth, 0.0, distFromNearestEdge);
            }
            
            maxAlpha = max(maxAlpha, alpha * verticalFade * horizontalFade);
        }
    }
    
    // Red color with alpha
    outColor = vec4(1.0, 0.0, 0.0, maxAlpha * 0.5);  // 50% transparent red
}
