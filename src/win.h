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

enum class MouseButton : int { Left = 0, Right = 1, Middle = 2 };
enum class KeyState : int { Released = 0, Pressed = 1, Repeat = 2 };

struct MouseState {
    double                  x = 0.0, y = 0.0;
    std::array<KeyState, 3> buttons = {KeyState::Released};
};

static constexpr int MaxKeyNum = GLFW_KEY_LAST;
struct KeyboardState {
    std::array<KeyState, MaxKeyNum> keys = {KeyState::Released};
};

inline KeyState ActionToKeyState(int action) {
    switch (action) {
    case GLFW_PRESS:
        return KeyState::Pressed;
    case GLFW_RELEASE:
        return KeyState::Released;
    case GLFW_REPEAT:
        return KeyState::Repeat;
    default:
        LOG_ERROR("Unknown key action {}.", action);
        return KeyState::Released;
    };
}

using ResizeCallback      = std::function<void(int, int)>;
using KeysCallback        = std::function<void(int, int, int, int)>;
using MouseMotionCallback = std::function<void(double, double)>;
using MouseClickCallback  = std::function<void(MouseButton, KeyState)>;

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
        auto keyState      = ActionToKeyState(action);
        keyboard.keys[key] = keyState;

        if (callbacks.keysCb)
            callbacks.keysCb(key, scancode, action, mods);
    }

    void processMouseClick(int button, int action, int /*mods*/) {
        if (button > 2)
            return;

        auto state            = ActionToKeyState(action);
        mouse.buttons[button] = state;

        if (callbacks.mouseClickCb)
            callbacks.mouseClickCb(static_cast<MouseButton>(button), state);
    }

    void processMouseMotion(double x, double y) {
        // double dx = mouse.x - x;
        // double dy = -(mouse.y - y);

        mouse.x = x;
        mouse.y = height - y;

        if (callbacks.mouseMotionCb)
            callbacks.mouseMotionCb(x, y);
    }

    const MouseState&    getMouse() { return mouse; }
    std::tuple<int, int> getDimensions() { return {width, height}; }

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

    MouseState    mouse;
    KeyboardState keyboard;

    WindowCallbacks callbacks;
    OpenglContext*  ctx;
};

} // namespace sdbox

#endif // __SDBOX_WINDOW_H__