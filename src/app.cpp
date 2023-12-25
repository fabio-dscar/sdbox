#include <app.h>

#include <shader.h>

using namespace sdbox;
using namespace std::literals;

static std::unique_ptr<Program> program;

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

    return win;
}

SdboxApp::~SdboxApp() {
    workers->stopWorkers();

    CleanupGeometry();

    for (auto ctx : sharedCtxs)
        glfwDestroyWindow(ctx);

    glfwDestroyWindow(win.context());
    glfwTerminate();
}

void SdboxApp::createDirectoryWatcher(const fs::path& folderPath) {
    auto watcherCallback = [&](const WatcherEvent& ev) {
        std::cout << ev << '\n';
    };

    auto fileChanged = [&](const WatcherEvent& ev) {
        workers->enqueue([=]() {
            auto shaders = std::array{"simple.vert"s, "simple.frag"s, "my_shader.frag"s};
            auto program = sdbox::CompileAndLinkProgram("brdf", shaders);

            auto v = sdbox::ExtractUniforms(*program);
            for (const auto& u : v)
                LOGI("{} {} {} {}", u.name, u.size, u.type, u.loc);

            LOGI("Compiled successfully! {}", program->id());
        });
    };

    watcher = CreateDirectoryWatcher(folderPath);

    using enum EventType;
    watcher->registerCallback(FileCreated, watcherCallback);
    watcher->registerCallback(FileMoved, watcherCallback);
    watcher->registerCallback(FileDeleted, watcherCallback);
    watcher->registerCallback(FileChanged, fileChanged);
    watcher->init();

    std::thread watcherThread{[&]() {
        SetThreadName("dirwatcher");
        watcher->watch();
    }};
    watcherThread.detach();
}

void SdboxApp::createUniforms() {
    // Triple slot ring buffer
    uniformBuffer.create(
        BufferType::Uniform, 3, sizeof(MainUniformBlock),
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    uniformBuffer.registerBind(0, 0, sizeof(MainUniformBlock));

    // Create uniform cache, etc
}

void SdboxApp::createThreadPool() {
    // Create thread pool and share OGL context
    workers = std::make_unique<ThreadPool>(4, [&](std::size_t idx) {
        SetThreadName(std::format("threadpool#{}", idx));
        glfwMakeContextCurrent(sharedCtxs[idx]);
    });
}

void SdboxApp::init(const fs::path& folderPath) {
    sdbox::SetThreadName("main");
    sdbox::InitLogger();

    win = InitOpenGL({.width = 640, .height = 480, .visible = true});

    // Create shared contexts for threads
    for (auto& ctx : sharedCtxs) {
        ctx = CreateContext({.share = win.context()});
        if (!ctx)
            FATAL("Failed to create shared OpenGL context.");
    }

    setWinCallbacks();
    createDirectoryWatcher(folderPath);
    createThreadPool();
    createUniforms();

    auto shaders = std::array{"simple.vert"s, "simple.frag"s, "my_shader.frag"s};
    program      = sdbox::CompileAndLinkProgram("main", shaders);
    glUseProgram(program->id());
}

void SdboxApp::setWinCallbacks() {
    auto resizeFunc = [this](int w, int h) {
        this->reshape(w, h);
    };

    auto keysFunc = [this](int key, int scancode, int action, int mods) {
        this->processKeys(key, scancode, action, mods);
    };

    auto mouseClickFunc = [this](MouseButton btn, KeyState state) {
        this->mouseClick(btn, state);
    };

    auto mouseMotionFunc = [this](double x, double y) {
        this->mouseMotion(x, y);
    };

    win.setCallbacks(
        {.resizeCb      = resizeFunc,
         .keysCb        = keysFunc,
         .mouseClickCb  = mouseClickFunc,
         .mouseMotionCb = mouseMotionFunc});
}

void SdboxApp::setUniforms() {
    auto ub = uniformBuffer.get<MainUniformBlock>();

    const auto& [w, h] = win.getDimensions();
    const auto& mouse  = win.getMouse();

    const auto& [left, right, mid] = mouse.buttons;

    ub->uResolution = {w, h, 0};
    ub->uMouse      = {mouse.x, mouse.y, left, right};
    ub->uTime       = time;
    ub->uTimeDelta  = deltaTime;
    ub->uFrameRate  = fps;
    ub->uFrame      = frameNum;
}

void SdboxApp::render() {
    uniformBuffer.wait();
    uniformBuffer.rebind();

    setUniforms();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderQuad();

    uniformBuffer.lockAndSwap();
}

void SdboxApp::loop() {
    glfwSetTime(0.0);

    while (!glfwWindowShouldClose(win.context())) {
        updateTime();
        render();

        win.swapBuffers();
        win.pollEvents();

        ++frameNum;
    }
}