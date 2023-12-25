#include <shader.h>

#include <regex>
#include <iostream>

#include <util.h>

#include <check.h>

using namespace sdbox;
using namespace std::filesystem;

void Shader::handleIncludes() {
    std::regex  rgx(R"([ ]*#[ ]*include[ ]+[\"<](.*)[\">].*)");
    std::smatch smatch;
    std::vector processed{name};

    while (std::regex_search(source, smatch, rgx)) {
        auto file = smatch[1].str();
        if (std::find(processed.begin(), processed.end(), file) != processed.end())
            FATAL("Repeated/Recursively including '{}' at '{}'.", file, name);

        auto filePath = ShaderFolder / file;
        auto src      = util::ReadTextFile(filePath, std::ios_base::in);
        if (!src)
            FATAL("Couldn't open included shader '{}' in '{}'", file, name);

        source.replace(smatch.position(), smatch.length(), src.value());

        processed.push_back(file);
    }
}

Shader::Shader(const std::string& name, ShaderType type, const std::string& src)
    : name(name), source(src), type(type) {
    handleIncludes();

    handle = glCreateShader(type);
    if (handle == 0)
        LOG_ERROR("Could not create shader {}", name);
}

bool Shader::compile(const std::string& defines) {
    if (!isValid()) {
        LOG_WARN("Trying to compile shader {} with invalid handle.", name);
        return false;
    }

    const char* sources[] = {defines.c_str(), source.c_str()};
    glShaderSource(handle, 2, sources, 0);
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
        std::string message = GetProgramError(handle);
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

    GLint res;
    glGetProgramiv(handle, GL_LINK_STATUS, &res);
    if (res != GL_TRUE) {
        for (GLuint sid : srcHandles)
            glDetachShader(handle, sid);

        std::string message = GetProgramError(handle);
        LOG_ERROR("Program linking error: {}", message);
        return false;
    }

    // Detach shaders after successful linking
    for (GLuint sid : srcHandles)
        glDetachShader(handle, sid);

    return true;
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

std::string sdbox::GetProgramError(unsigned int handle) {
    GLint logLen;
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &logLen);

    auto log = std::make_unique<char[]>(logLen);
    glGetProgramInfoLog(handle, logLen, &logLen, log.get());

    return {log.get()};
}

std::unique_ptr<Shader> sdbox::LoadShaderFile(const std::string& fileName) {
    ShaderType type = Fragment;

    // Deduce type from extension, if possible
    auto ext = path(fileName).extension();
    if (ext == ".frag" || ext == ".fs")
        type = Fragment;
    else if (ext == ".vert" || ext == ".vs")
        type = Vertex;
    else
        FATAL("Couldn't deduce type for shader: {}", fileName);

    return LoadShaderFile(type, fileName);
}

std::unique_ptr<Shader> sdbox::LoadShaderFile(ShaderType type, const std::string& fileName) {
    auto filePath = ShaderFolder / fileName;
    auto source   = util::ReadTextFile(ShaderFolder / fileName, std::ios_base::in);
    if (!source.has_value()) {
        LOG_ERROR("Couldn't load shader file {}", filePath.string());
        return nullptr;
    }

    return std::make_unique<Shader>(filePath.filename(), type, source.value());
}

std::string sdbox::BuildDefinesBlock(std::span<std::string> defines) {
    std::string defBlock = VerDirective;
    for (auto& def : defines) {
        if (!def.empty()) {
            defBlock.append("#define ");
            defBlock.append(def.begin(), def.end());
            defBlock.append("\n");
        }
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