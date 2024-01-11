#include <shader.h>

#include <glad/glad.h>

#include <util.h>
#include <check.h>

#include <regex>

using namespace sdbox;
using namespace std::filesystem;

namespace {

const std::string           DefaultVer   = "460 core";

} // namespace

enum class sdbox::ShaderType : unsigned int {
    Vertex   = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
    Geometry = GL_GEOMETRY_SHADER,
    Compute  = GL_COMPUTE_SHADER
};

Shader::Shader(const std::string& name, ShaderType type, const std::string& src)
    : name(name), source(src), type(type) {
    handle = glCreateShader(static_cast<GLenum>(type));
    if (handle == 0) {
        LOG_ERROR("Could not create shader {}", name);
    } else {
        if (!hasVersionDir())
            setVersion(DefaultVer);

        handleIncludes();
    }
}

Shader::~Shader() {
    if (handle > 0)
        glDeleteShader(handle);
}

void Shader::handleIncludes() {
    std::regex  rgx(R"([ ]*#[ ]*include[ ]+[\"<](.*)[\">].*)");
    std::smatch smatch;
    std::vector processed{name};

    while (std::regex_search(source, smatch, rgx)) {
        auto file = smatch[1].str();
        if (std::find(processed.begin(), processed.end(), file) != processed.end())
            FATAL("Repeated/Recursively including '{}' at '{}'.", file, name);

        auto filePath = ShaderFolder / file; // TODO: Improve this by going through shader parent
        auto src      = util::ReadTextFile(filePath);
        if (!src)
            FATAL("Couldn't open included shader '{}' in '{}'", file, name);

        source.replace(smatch.position(), smatch.length(), src.value());

        processed.push_back(file);
    }
}

bool Shader::hasVersionDir() {
    return source.find("#version ") != std::string::npos;
}

std::string Shader::getVersion() {
    auto start = source.find("#version ");

    if (start == std::string::npos)
        return "none";

    start += std::strlen("#version ");
    auto end = source.find('\n', start);
    return source.substr(start, end - start);
}

void Shader::setVersion(const std::string& ver) {
    auto start = source.find("#version");

    auto directive = std::format("#version {}\n", ver);
    if (start != std::string::npos) {
        auto end = source.find('\n', start);
        source.replace(start, end - start, directive);
    } else {
        source.insert(0, directive);
    }
}

void Shader::include(const std::string& include) {
    auto start = source.find("#version");
    auto end   = 0;

    if (start != std::string::npos)
        end = source.find('\n', start);

    source.insert(end + 1, include);
}

bool Shader::compile(const std::string& defines) {
    if (!isValid()) {
        LOG_WARN("Trying to compile shader {} with invalid handle.", name);
        return false;
    }

    include(defines);

    const char* sources[] = {source.c_str()};
    glShaderSource(handle, 1, sources, 0);
    glCompileShader(handle);

    GLint result;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE) {
        std::string message = GetShaderLog(handle);
        LOG_ERROR("Shader {} compilation log:\n{}", name, message);
        return false;
    }

    return true;
}

Program::Program(const std::string& name) : name(name) {
    handle = glCreateProgram();
    if (handle == 0) {
        std::string message = GetProgramLog(handle);
        LOG_ERROR("Could not create program {}", name);
    }
}

Program::~Program() {
    if (handle != 0)
        glDeleteProgram(handle);
}

void Program::addShader(const Shader& src) {
    srcHandles.push_back(src.id());
}

bool Program::link() {
    if (!isValid()) {
        LOG_WARN("Trying to link program {}, with invalid handle.", name);
        return false;
    }

    for (GLuint sid : srcHandles)
        glAttachShader(handle, sid);

    glLinkProgram(handle);

    for (GLuint sid : srcHandles)
        glDetachShader(handle, sid);

    GLint res;
    glGetProgramiv(handle, GL_LINK_STATUS, &res);
    if (res != GL_TRUE) {
        std::string message = GetProgramLog(handle);
        LOG_ERROR("Program linking error: {}", message);
        return false;
    }

    return true;
}

ProgramBinary Program::getBinary() const {
    GLint binSize;
    glGetProgramiv(handle, GL_PROGRAM_BINARY_LENGTH, &binSize);

    ProgramBinary bin;
    bin.data = std::make_unique<std::byte[]>(binSize);
    glGetProgramBinary(handle, binSize, &bin.size, &bin.format, bin.data.get());

    return bin;
}

void Program::cleanShaders() {
    for (auto sid : srcHandles)
        if (glIsShader(sid) == GL_TRUE)
            glDeleteShader(sid);
}

std::string sdbox::GetShaderLog(unsigned int handle) {
    GLint logLen;
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);

    auto log = std::make_unique<char[]>(logLen);
    glGetShaderInfoLog(handle, logLen, &logLen, log.get());

    return {log.get()};
}

std::string sdbox::GetProgramLog(unsigned int handle) {
    GLint logLen;
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &logLen);

    auto log = std::make_unique<char[]>(logLen);
    glGetProgramInfoLog(handle, logLen, &logLen, log.get());

    return {log.get()};
}

ShaderType sdbox::DeduceShaderType(const std::string& fileName) {
    auto ext = std::filesystem::path(fileName).extension();
    if (ext == ".frag" || ext == ".fs" || ext == ".glsl")
        return ShaderType::Fragment;
    else if (ext == ".vert" || ext == ".vs")
        return ShaderType::Vertex;

    LOG_WARN(
        "Couldn't deduce type for shader: {}. Choosing 'fragment' shader by default.", fileName);
    return ShaderType::Fragment;
}

std::unique_ptr<Shader>
LoadShaderFromMemory(const std::string& name, const char* bytes, std::size_t size) {
    std::string contents{bytes, size};
    ShaderType  type = DeduceShaderType(name);
    return std::make_unique<Shader>(name, type, contents);
}

std::unique_ptr<Shader> sdbox::LoadShaderFile(const fs::path& filePath) {
    // Deduce type from extension, if possible
    auto type = DeduceShaderType(filePath);
    return LoadShaderFile(type, filePath);
}

std::unique_ptr<Shader> sdbox::LoadShaderFile(ShaderType type, const fs::path& filePath) {
    auto source = util::ReadTextFile(filePath);
    if (!source.has_value())
        return nullptr;

    return std::make_unique<Shader>(filePath.filename(), type, source.value());
}

std::string sdbox::BuildDefinesBlock(std::span<std::string> defines) {
    std::string defBlock = "";
    for (auto& def : defines) {
        if (!def.empty())
            defBlock.append(std::format("#define {}\n", def));
    }
    return defBlock;
}

std::unique_ptr<Program> sdbox::CompileAndLinkProgram(
    const std::string& name, std::span<std::string> sourceNames,
    std::span<std::string> definesList) {
    auto program = std::make_unique<Program>(name);
    auto defines = BuildDefinesBlock(definesList);

    for (const auto& fname : sourceNames) {
        auto sh = LoadShaderFile(fname);
        if (!sh || !sh->isValid())
            FATAL("Failed to load/create shader {}.", fname);

        if (!sh->compile(defines))
            FATAL("Failed to compile shader {}.", fname);

        program->addShader(*sh);
    }

    if (!program->link())
        FATAL("Couldn't link program {}.", name);

    program->cleanShaders();
    return program;
}