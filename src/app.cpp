#include <app.h>

using namespace sdbox;

Window InitOpenGL(const WindowOpts& winOpts) {
    if (!glfwInit())
        FATAL("Couldn't initialize OpenGL context.");

    // Create window and context
    Window win{winOpts};

    int glver = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    if (glver == 0)
        FATAL("Failed to initialize OpenGL loader");

#ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(OpenGLErrorCallback, 0);
#endif

    // Print system info
    auto renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    auto vendor   = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    auto version  = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    auto glslVer  = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    LOGI("OpenGL Renderer: {} ({})", renderer, vendor);
    LOGI("OpenGL Version: {}", version);
    LOGI("GLSL Version: {}", glslVer);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDepthRange(0.0, 1.0);
    glClearDepth(1.0);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glDisable(GL_CULL_FACE); // We're rendering skybox back faces

    return win;
}

SdboxApp::~SdboxApp() {
    for (auto ctx : sharedCtxs)
        glfwDestroyWindow(ctx);
    glfwTerminate();
}

void SdboxApp::init() {
    win = InitOpenGL({.width = 640, .height = 480, .visible = true});

    // Create shared contexts for threads
    for (auto& ctx : sharedCtxs)
        ctx = CreateContext({.share = win.context()});

    auto callbacks = WindowCallbacks{
        .resizeCb = [this](int w, int h) { this->reshape(w, h); },
        .keysCb =
            [this](int key, int scancode, int action, int mods) {
                this->processKeys(key, scancode, action, mods);
            },
        .mouseClickCb =
            [this](MouseButton btn, bool isPressed) { this->mouseClick(btn, isPressed); },
        .mouseMotionCb =
            [this](double x, double y) {
                this->mouseMotion(x, y);
            }};

    win.setCallbacks(callbacks);

    // Triple slot ring buffer
    uniformBuffer.create(
        EBufferType::UNIFORM, 3, sizeof(MainUniformBlock),
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    uniformBuffer.registerBind(0, 0, sizeof(MainUniformBlock));
}

void SdboxApp::setUniforms() {
    auto ub = uniformBuffer.get<MainUniformBlock>();

    const auto& [x, y] = win.getMouse();
    const auto& [w, h] = win.getDimensions();

    ub->uResolution = {w, h, 0};
    ub->uMouse      = {x, y, 0, 0};
    ub->uTime       = time;
    ub->uTimeDelta  = deltaTime;
    ub->uFrameRate  = fps;
    ub->uFrame      = frameNum;
}

void SdboxApp::render() {
    uniformBuffer.wait();
    uniformBuffer.rebind();

    setUniforms();

    uniformBuffer.lock();
    uniformBuffer.swap();
}

void SdboxApp::loop() {
    glfwSetTime(0.0);

    while (!glfwWindowShouldClose(win.context())) {
        updateTime();
        render();

        LOGI("{}", time);

        win.swapBuffers();
        win.pollEvents();

        ++frameNum;
    }
}