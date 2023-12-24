#ifndef __SDBOX_APP_H__
#define __SDBOX_APP_H__

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <check.h>
#include <graphics.h>

#include <functional>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <ringbuffer.h>

#include <win.h>

namespace sdbox {

struct alignas(256) MainUniformBlock {
    glm::vec4 uMouse;      // xy = current, zw = click
    glm::vec3 uResolution; // Viewport res
    float     uTime;       // Shader playback (seconds)
    float     uTimeDelta;  // Render time (seconds)
    float     uFrameRate;  // Frame rate
    int       uFrame;      // Number of the frame
};

class SdboxApp {
public:
    ~SdboxApp();

    void init();
    void setUniforms();
    void render();
    void loop();

private:
    void updateTime() {
        double timeSinceStart = glfwGetTime();

        deltaTime = timeSinceStart - time;
        time      = timeSinceStart;

        if (0) {
            fps = 0;
        }
    }

    void reshape(int w, int h) { glViewport(0, 0, w, h); }
    void mouseMotion(double x, double y) {}
    void mouseClick(MouseButton btn, bool isPressed) {}
    void processKeys(int key, int scancode, int action, int mods) {}

    Window win;

    int   frameNum  = 0;
    float fps       = 0.0f;
    float time      = 0.0f;
    float deltaTime = 0.0f;

    RingBuffer uniformBuffer{};

    std::array<OpenglContext*, 4> sharedCtxs;
};

} // namespace sdbox

#endif // __SDBOX_APP_H__