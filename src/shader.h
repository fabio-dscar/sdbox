#ifndef __IBL_SHADER_H__
#define __IBL_SHADER_H__

#include <sdbox.h>

#include <glad/glad.h>

namespace sdbox {

enum ShaderType {
    Vertex   = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
    Geometry = GL_GEOMETRY_SHADER,
    Compute  = GL_COMPUTE_SHADER
};

static const std::string           VerDirective = "#version 460 core\n\n";
static const std::filesystem::path ShaderFolder = "./glsl";

class Shader {
public:
    Shader(const std::string& name, ShaderType type, const std::string& src)
        : name(name), source({src}), type(type) {
        handleIncludes();
    }

    unsigned int id() const { return handle; }

    void compile(const std::string& defines = VerDirective);

private:
    void handleIncludes();

    std::string  name;
    std::string  source;
    ShaderType   type;
    unsigned int handle = 0;
};

class Program {
public:
    explicit Program(const std::string& name) : name(name) {}

    ~Program();

    void addShader(const Shader& src);

    unsigned int id() const { return handle; }

    void link();
    void cleanShaders();

private:
    std::vector<unsigned int> srcHandles;
    std::string               name;
    unsigned int              handle = 0;
};

Shader LoadShaderFile(const std::string& filePath);
Shader LoadShaderFile(ShaderType type, const std::string& filePath);

std::unique_ptr<Program> CompileAndLinkProgram(
    const std::string& name, std::span<std::string> sourceNames,
    std::span<std::string> definesList = {});

std::string BuildDefinesBlock(std::span<std::string> defines);
std::string GetShaderLog(unsigned int handle);
std::string GetProgramError(unsigned int handle);

} // namespace sdbox

#endif // __IBL_SHADER_H__