#ifndef __IBL_SHADER_H__
#define __IBL_SHADER_H__

#include <sdbox.h>

#include <glad/glad.h>

namespace fs = std::filesystem;

namespace sdbox {

enum ShaderType {
    Vertex   = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
    Geometry = GL_GEOMETRY_SHADER,
    Compute  = GL_COMPUTE_SHADER
};

static const std::string           DefaultVer   = "460 core";
static const std::filesystem::path ShaderFolder = "./glsl";

class Shader {
public:
    Shader(const std::string& name, ShaderType type, const std::string& src);
    ~Shader() {
        if (handle > 0)
            glDeleteShader(handle);
    }

    unsigned int id() const { return handle; }

    void setVersion(const std::string& ver);
    void include(const std::string& source);

    bool compile(const std::string& defines = "");
    bool isValid() const { return handle > 0; }

    const std::string& getName() const { return name; }
    const std::string& getSource() const { return source; }

private:
    std::string getVersion();
    bool        hasVersionDir();

    void handleIncludes();

    std::string  name;
    std::string  source;
    ShaderType   type;
    unsigned int handle = 0;
};

class Program {
public:
    explicit Program(const std::string& name);
    ~Program();

    void addShader(const Shader& src);

    unsigned int id() const { return handle; }

    bool link();
    bool isValid() const { return handle > 0; }
    void cleanShaders();

private:
    std::vector<unsigned int> srcHandles;
    std::string               name;
    unsigned int              handle = 0;
};

ShaderType DeduceShaderType(const std::string& fileName);

std::unique_ptr<Shader>
LoadShaderFromMemory(const std::string& name, const char* bytes, std::size_t size);

std::unique_ptr<Shader> LoadShaderFile(const fs::path& filePath);
std::unique_ptr<Shader> LoadShaderFile(ShaderType type, const fs::path& filePath);

std::unique_ptr<Program> CompileAndLinkProgram(
    const std::string& name, std::span<std::string> sourceNames,
    std::span<std::string> definesList = {});

std::string BuildDefinesBlock(std::span<std::string> defines);
std::string GetShaderLog(unsigned int handle);
std::string GetProgramLog(unsigned int handle);

struct UniformInfo {
    GLenum      type;
    std::string name;
    std::size_t size;
    int         loc;
};

inline std::vector<UniformInfo> ExtractUniforms(const Program& prog) {
    GLint count;
    glGetProgramiv(prog.id(), GL_ACTIVE_UNIFORMS, &count);

    std::vector<UniformInfo> uniformList;
    uniformList.reserve(count);
    for (int i = 0; i < count; ++i) {
        GLint  size;
        GLenum type;
        GLchar name[256];

        glGetActiveUniform(prog.id(), i, 256, NULL, &size, &type, name);
        GLint loc = glGetUniformLocation(prog.id(), name);

        // Exclude uniform block variables
        if (loc != -1)
            uniformList.emplace_back(type, name, size, loc);
    }

    return uniformList;
}

} // namespace sdbox

#endif // __IBL_SHADER_H__