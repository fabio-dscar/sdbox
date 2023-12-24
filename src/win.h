#ifndef __SDBOX_WINDOW_H__
#define __SDBOX_WINDOW_H__

#include <sdbox.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace sdbox {

using OpenglContext = GLFWwindow;

struct WindowOpts {
    int            width   = 32;
    int            height  = 32;
    bool           visible = false;
    std::string    title   = "sdbox";
    OpenglContext* share   = nullptr;
};

enum class MouseButton { Left = 0, Right = 1, Middle = 2 };

using ResizeCallback      = std::function<void(int, int)>;
using KeysCallback        = std::function<void(int, int, int, int)>;
using MouseMotionCallback = std::function<void(double, double)>;
using MouseClickCallback  = std::function<void(MouseButton, bool)>;

struct WindowCallbacks {
    ResizeCallback      resizeCb;
    KeysCallback        keysCb;
    MouseClickCallback  mouseClickCb;
    MouseMotionCallback mouseMotionCb;
};

inline OpenglContext* CreateContext(const WindowOpts& opts) {
    glfwWindowHint(GLFW_VISIBLE, opts.visible ? GLFW_TRUE : GLFW_FALSE);
    OpenglContext* win =
        glfwCreateWindow(opts.width, opts.height, opts.title.c_str(), NULL, opts.share);
    return win;
}

class Window {
public:
    Window() = default;
    explicit Window(const WindowOpts& opts)
        : width(opts.width), height(opts.height), winTitle(opts.title) {
        ctx = CreateContext(opts);
        if (!ctx) {
            glfwTerminate();
            FATAL("Failed to create glfw window.");
        }
        glfwMakeContextCurrent(ctx);
    }

    void setTitle(const std::string& title) {
        winTitle = title;
        glfwSetWindowTitle(ctx, title.c_str());
    }

    OpenglContext* context() const { return ctx; }

    void swapBuffers() const { glfwSwapBuffers(ctx); }
    void pollEvents() const { glfwPollEvents(); }

    void reshape(int w, int h) {
        width  = w;
        height = h;

        if (callbacks.resizeCb)
            callbacks.resizeCb(w, h);
    }

    void processKeys(int key, int scancode, int action, int mods) {
        if (callbacks.keysCb)
            callbacks.keysCb(key, scancode, action, mods);
    }

    void processMouseClick(int button, int action, int mods) {
        if (button > 2)
            return;

        if (callbacks.mouseClickCb)
            callbacks.mouseClickCb(static_cast<MouseButton>(button), action == GLFW_PRESS);
    }

    void processMouseMotion(double x, double y) {
        double dx = mouseX - x;
        double dy = -(mouseY - y);

        mouseX = x;
        mouseY = y;

        if (callbacks.mouseMotionCb)
            callbacks.mouseMotionCb(x, y);
    }

    std::tuple<double, double> getMouse() { return {mouseX, mouseY}; }
    std::tuple<int, int>       getDimensions() { return {width, height}; }

    void setCallbacks(WindowCallbacks winCallbacks) {
        callbacks = winCallbacks;

        glfwSetWindowUserPointer(ctx, this);

        glfwSetKeyCallback(ctx, [](OpenglContext* window, int k, int s, int a, int m) {
            Window* app = static_cast<Window*>(glfwGetWindowUserPointer(window));
            app->processKeys(k, s, a, m);
        });

        glfwSetFramebufferSizeCallback(ctx, [](OpenglContext* window, int w, int h) {
            Window* app = static_cast<Window*>(glfwGetWindowUserPointer(window));
            app->reshape(w, h);
        });

        glfwSetMouseButtonCallback(ctx, [](OpenglContext* window, int b, int a, int m) {
            Window* app = static_cast<Window*>(glfwGetWindowUserPointer(window));
            app->processMouseClick(b, a, m);
        });

        glfwSetCursorPosCallback(ctx, [](OpenglContext* window, double x, double y) {
            Window* app = static_cast<Window*>(glfwGetWindowUserPointer(window));
            app->processMouseMotion(x, y);
        });
    }

private:
    int         width    = 32;
    int         height   = 32;
    std::string winTitle = "sdbox";

    double mouseX = 0.0;
    double mouseY = 0.0;

    WindowCallbacks callbacks;
    OpenglContext*  ctx;
};

} // namespace sdbox

#endif // __SDBOX_WINDOW_H__