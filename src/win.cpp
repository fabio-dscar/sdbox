#include <win.h>

using namespace sdbox;

Window::Window(const WindowOpts& opts)
    : width(opts.width), height(opts.height), winTitle(opts.title) {
    ctx = CreateContext(opts);
    if (!ctx) {
        glfwTerminate();
        FATAL("Failed to create glfw window.");
    }
    glfwMakeContextCurrent(ctx);
}

void Window::setCallbacks(WindowCallbacks winCallbacks) {
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

void Window::processKeys(int key, int scancode, int action, int mods) {
    auto keyState      = ActionToKeyState(action);
    keyboard.keys[key] = keyState;

    if (callbacks.keysCb)
        callbacks.keysCb(key, scancode, action, mods);
}

void Window::processMouseClick(int button, int action, int /*mods*/) {
    if (button > 2)
        return;

    auto state            = ActionToKeyState(action);
    mouse.buttons[button] = state;

    if (callbacks.mouseClickCb)
        callbacks.mouseClickCb(static_cast<MouseButton>(button), state);
}

void Window::processMouseMotion(double x, double y) {
    // double dx = mouse.x - x;
    // double dy = -(mouse.y - y);

    mouse.x = x;
    mouse.y = height - y;

    if (callbacks.mouseMotionCb)
        callbacks.mouseMotionCb(x, y);
}

void Window::reshape(int w, int h) {
    width  = w;
    height = h;

    if (callbacks.resizeCb)
        callbacks.resizeCb(w, h);
}

OpenglContext* sdbox::CreateContext(const WindowOpts& opts) {
    glfwWindowHint(GLFW_VISIBLE, opts.visible ? GLFW_TRUE : GLFW_FALSE);
    OpenglContext* win =
        glfwCreateWindow(opts.width, opts.height, opts.title.c_str(), NULL, opts.share);
    return win;
}