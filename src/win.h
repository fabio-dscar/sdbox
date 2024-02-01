#ifndef SDBOX_WINDOW_H
#define SDBOX_WINDOW_H

#include <sdbox.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace sdbox {

// ------------------------------------------------------------------
//      Mouse & Keyboard
// ------------------------------------------------------------------
enum class MouseButton : int { Left = 0, Right = 1, Middle = 2 };
enum class KeyState : int { Released = 0, Pressed = 1, Repeat = 2 };

struct MouseState {
    double                  x       = 0.0;
    double                  y       = 0.0;
    double                  dx      = 0.0;
    double                  dy      = 0.0;
    std::array<KeyState, 3> buttons = {KeyState::Released};
};

constexpr int MaxKeyNum = GLFW_KEY_LAST;
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

// ------------------------------------------------------------------
//      Window
// ------------------------------------------------------------------
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

using OpenglContext = GLFWwindow;

struct WindowOpts {
    int            width   = 1;
    int            height  = 1;
    bool           visible = false;
    std::string    title   = "sdbox";
    OpenglContext* share   = nullptr;
};

class Window {
public:
    Window() = default;
    explicit Window(const WindowOpts& opts);

    void setTitle(const std::string& title) {
        winTitle = title;
        glfwSetWindowTitle(ctx, title.c_str());
    }

    OpenglContext* context() const { return ctx; }

    void swapBuffers() const { glfwSwapBuffers(ctx); }
    void pollEvents() const { glfwPollEvents(); }

    void reshape(int w, int h);
    void processKeys(int key, int scancode, int action, int mods);
    void processMouseClick(int button, int action, int /*mods*/);
    void processMouseMotion(double x, double y);

    const MouseState&    getMouse() const { return mouse; }
    const KeyboardState& getKeyboard() const { return keyboard; }
    std::tuple<int, int> getDimensions() const { return {width, height}; }

    void setCallbacks(WindowCallbacks winCallbacks);

private:
    int             width    = 32;
    int             height   = 32;
    std::string     winTitle = "sdbox";
    MouseState      mouse;
    KeyboardState   keyboard;
    WindowCallbacks callbacks;
    OpenglContext*  ctx;
};

OpenglContext* CreateContext(const WindowOpts& opts);

} // namespace sdbox

#endif