#include <graphics.h>

using namespace sdbox;

void GLAPIENTRY sdbox::OpenGLErrorCallback(
    GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*) {
    std::cerr << std::format(
        "[OPENGL] Type = 0x{:x}, Severity = 0x{:x}, Message = {}\n", type, severity, message);
}