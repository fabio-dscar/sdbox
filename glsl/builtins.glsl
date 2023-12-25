#define PI 3.14159265

layout(std140, binding = 0) uniform mainBlock {
    vec4 uMouse;       // xy = current, zw = click
    vec3 uResolution;  // Viewport res
    float uTime;       // Shader playback (seconds)
    float uTimeDelta;  // Render time (seconds)
    float uFrameRate;  // Frame rate
    int uFrame;        // Number of the frame
};

layout(location = 0) uniform sampler2D uTexture0;
layout(location = 1) uniform sampler2D uTexture1;
layout(location = 2) uniform sampler2D uTexture2;
layout(location = 3) uniform sampler2D uTexture3;

layout(location = 4) uniform samplerCube uCube0;
layout(location = 5) uniform samplerCube uCube1;
layout(location = 6) uniform samplerCube uCube2;
layout(location = 7) uniform samplerCube uCube3;