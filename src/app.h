#ifndef __SDBOX_APP_H__
#define __SDBOX_APP_H__

#include <sdbox.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <graphics.h>

#include <functional>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <ringbuffer.h>
#include <thread.h>
#include <win.h>
#include <watcher/watcher.h>
#include <util.h>

#include <unordered_set>

#include <resource.h>

namespace fs = std::filesystem;

namespace sdbox {

struct MainUniformBlock {
    glm::vec4 uMouse;      // xy = current, zw = click
    glm::vec3 uResolution; // Viewport res
    float     uTime;       // Shader playback (seconds)
    float     uTimeDelta;  // Render time (seconds)
    float     uFrameRate;  // Frame rate
    int       uFrame;      // Number of the frame
};

constexpr int NumWorkers = 2;

class SdboxApp {
public:
    ~SdboxApp();

    void init(const fs::path& folderPath);
    void setUniforms();
    void render();
    void loop();

private:
    void setProgram();

    void resetTime() {
        time      = 0.0;
        deltaTime = 0.0;
        frameNum  = 0;

        glfwSetTime(0.0);
    }

    void updateTime() {
        const double timeSinceStart = glfwGetTime();

        deltaTime = timeSinceStart - time;
        time      = timeSinceStart;
        fps       = 0.5 * fps + 0.5 / deltaTime;

        ++frameNum;
    }

    void setWinCallbacks();
    void reshape(int w, int h) { glViewport(0, 0, w, h); }
    void mouseMotion(double x, double y) {}
    void mouseClick(MouseButton btn, KeyState state) {}
    void processKeys(int key, int scancode, int action, int mods) {
        if (key == 'P' && action == GLFW_RELEASE) {
            paused = !paused;
            if (!paused)
                glfwSetTime(time);
        }
    }

    void createUniforms();
    void createThreadPool();
    void createDirectoryWatcher(const fs::path& folderPath);
    void loadInitialShaders(const fs::path& folderPath);

    Window win;

    int    frameNum  = 0;
    double fps       = 0.0;
    double time      = 0.0;
    double deltaTime = 0.0;
    bool   paused    = false;

    fs::path dirPath;

    RingBuffer uniformBuffer{};

    std::unique_ptr<ThreadPool>       workers;
    std::unique_ptr<DirectoryWatcher> watcher;

    std::array<OpenglContext*, NumWorkers> sharedCtxs;

    ResourceRegistry res;

    Resource<Program> mainProg;
};

} // namespace sdbox

#endif // __SDBOX_APP_H__