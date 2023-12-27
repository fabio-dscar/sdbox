#ifndef __SDBOX_GRAPHICS_H__
#define __SDBOX_GRAPHICS_H__

#include <glad/glad.h>

#include <sdbox.h>

namespace sdbox {

void RenderQuad();
void CleanupGeometry();

inline std::size_t AlignUniformBuffer(std::size_t structSize) {
    // Query implementation alignment and return aligned size
    GLint uboAlignment;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uboAlignment);
    return (structSize + uboAlignment - 1) & ~(uboAlignment - 1);
}

void GLAPIENTRY OpenGLErrorCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
    const void* userParam);

} // namespace sdbox

#endif // __SDBOX_GRAPHICS_H__