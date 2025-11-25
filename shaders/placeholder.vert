#version 450

// Fullscreen triangle vertex shader
layout(location = 0) out vec2 fragUV;

void main() {
    // Generate a fullscreen triangle using vertex ID
    // Vertex 0: (-1, -1), Vertex 1: (3, -1), Vertex 2: (-1, 3)
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2(3.0, -1.0),
        vec2(-1.0, 3.0)
    );
    
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    
    // Convert from [-1, 1] to [0, 1] UV coordinates
    fragUV = positions[gl_VertexIndex] * 0.5 + 0.5;
}
