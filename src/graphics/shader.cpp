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

void extractUniforms() {
    auto uni =
        R"([ ]*uniform[ ]*(float|int|unsigned int|vec2|vec3|vec4|mat2|mat3|mat4)[ ]*([a-zA-Z]+[a-zA-Z\d]*)[ ]*;)";
}

void Shader::compile(const std::string& defines) {
    handle = glCreateShader(type);
    if (handle == 0)
        FATAL("Could not create shader {}", name);

    const char* sources[] = {defines.c_str(), source.c_str()};
    glShaderSource(handle, 2, sources, 0);
    glCompileShader(handle);

    GLint result;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE) {
        std::string message = GetShaderLog(handle);
        FATAL("Shader {} compilation log:\n{}", name, message);
    }
}

Program::~Program() {
    if (handle != 0)
        glDeleteProgram(handle);
}

void Program::addShader(const Shader& src) {
    srcHandles.push_back(src.id());
}

void Program::link() {
    handle = glCreateProgram();
    if (handle == 0) {
        std::string message = GetProgramError(handle);
        FATAL("Could not create program {}", name);
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
        FATAL("Program linking error: {}", message);
    }

    // Detach shaders after successful linking
    for (GLuint sid : srcHandles)
        glDetachShader(handle, sid);
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

Shader sdbox::LoadShaderFile(const std::string& fileName) {
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

Shader sdbox::LoadShaderFile(ShaderType type, const std::string& fileName) {
    auto filePath = ShaderFolder / fileName;
    auto source   = util::ReadTextFile(ShaderFolder / fileName, std::ios_base::in);
    if (!source.has_value())
        FATAL("Couldn't load shader file {}", filePath.string());

    return {filePath.filename(), type, source.value()};
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

    for (auto& fname : sourceNames) {
        Shader s = LoadShaderFile(fname);
        s.compile(defines);
        program->addShader(s);
    }

    program->link();
    return program;
}