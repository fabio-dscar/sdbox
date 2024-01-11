#include <app.h>

#include <shader.h>

using namespace sdbox;
using namespace std::literals;

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
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
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

void RebuildProgram(const Resource<Shader>& sh, ResourceRegistry& reg) {
    auto vert = reg.getResource<Shader>(Hash("simple.vert"));
    auto frag = reg.getResource<Shader>(Hash("simple.frag"));
    DCHECK(vert && frag);

    auto prog = std::make_unique<Program>(sh.name);
    prog->addShader(*(vert.value().resource));
    prog->addShader(*(frag.value().resource));
    prog->addShader(*sh.resource);

    if (!prog->link())
        return;

    for (int s = 0; s < 8; ++s)
        glProgramUniform1i(prog->id(), s, s);

    reg.addResource(Resource{sh.name, sh.nameHash, sh.hash, std::move(prog)});
}

std::optional<Resource<Shader>> LoadShaderResource(const fs::path& path, ResourceRegistry& reg) {
    auto shader = LoadShaderFile(path);
    if (!shader)
        return std::nullopt;

    const auto fileName = path.filename();

    const auto nameHash = HashBytes64(fileName);
    const auto srcHash  = HashBytes64(shader->getSource());
    if (reg.exists<Shader>(nameHash, srcHash) || !shader->compile()) {
        LOGD("[Shader] Leaving early... {}", srcHash);
        return std::nullopt;
    }

    auto resource = Resource{fileName, nameHash, srcHash, std::move(shader)};
    reg.addResource(resource);

    return resource;
}

void SdboxApp::createDirectoryWatcher(const fs::path& folderPath) {
    auto errorCallback = [](const std::string& err) {
        LOG_ERROR("{}", err);
    };

    auto watcherCallback = [](const WatcherEvent& ev) {
        std::cout << ev << '\n';
    };

    auto fileChanged = [&](const WatcherEvent& ev) {
        workers->enqueue([&, ev]() {
            if (!IsBuiltinName(ev.name))
                return;

            auto shader = LoadShaderResource(dirPath / ev.name, res);
            if (shader)
                RebuildProgram(shader.value(), res);
        });
    };

    watcher = CreateDirectoryWatcher(folderPath);

    using enum EventType;
    watcher->registerCallback(FileCreated, watcherCallback);
    watcher->registerCallback(FileMoved, watcherCallback);
    watcher->registerCallback(FileDeleted, watcherCallback);
    watcher->registerCallback(FileChanged, fileChanged);
    watcher->registerErrorCallback(errorCallback);
    watcher->init();

    std::thread watcherThread{[&]() {
        SetThreadName("dirwatcher");
        watcher->watch();
    }};
    watcherThread.detach();
}

void SdboxApp::createUniforms() {
    using enum BufferFlag;

    // Triple slot ring buffer
    auto uboSize = AlignUniformBuffer(sizeof(MainUniformBlock));
    uniformBuffer.create(BufferType::Uniform, 3, uboSize, Write | Persistent | Coherent);

    uniformBuffer.registerBind(0, 0, uboSize);

    // Create uniform cache, etc
}

void SdboxApp::createThreadPool() {
    // Create thread pool and share OGL context
    workers = std::make_unique<ThreadPool>(NumWorkers, [&](std::size_t idx) {
        SetThreadName(std::format("threadpool#{}", idx));
        glfwMakeContextCurrent(sharedCtxs[idx]);
    });
}

void SdboxApp::loadInitialShaders(const fs::path& folderPath) {
    auto vert = LoadShaderResource(ShaderFolder / "simple.vert", res);
    auto frag = LoadShaderResource(ShaderFolder / "simple.frag", res);
    CHECK(vert && frag);

    auto main = LoadShaderResource(folderPath / "main.glsl", res);
    if (!main)
        FATAL("Make sure there is a valid main.glsl file on the specified folder.");

    RebuildProgram(*main, res);
}

void SdboxApp::init(const fs::path& folderPath) {
    SetThreadName("main");
    InitLogger();

    dirPath = folderPath;

    win = InitOpenGL({.width = 800, .height = 600, .visible = true});

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

    loadInitialShaders(folderPath);
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

void SdboxApp::setProgram() {
    auto prog = res.getResource<Program>(Hash("main.glsl"));
    if (prog && prog->hash != mainProg.hash) {
        mainProg = prog.value();
        glUseProgram(mainProg.resource->id());
        resetTime();
    }
}

void SdboxApp::render() {
    uniformBuffer.wait();
    uniformBuffer.rebind();

    setProgram();
    setUniforms();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderQuad();

    uniformBuffer.lockAndSwap();
}

void SdboxApp::loop() {
    glfwSetTime(0.0);

    while (!glfwWindowShouldClose(win.context())) {
        render();

        if (!paused)
            updateTime();

        win.swapBuffers();
        win.pollEvents();
    }
}