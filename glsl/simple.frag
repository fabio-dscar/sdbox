#include <builtins.glsl>

in vec2 FsTexCoords;
out vec4 FragColor;

void mainImage(out vec4 color, in vec2 pixel);

void main() {
    vec4 color = vec4(0, 0, 0, 1);
    mainImage(color, FsTexCoords.xy * uResolution.xy);
    FragColor = color;
}