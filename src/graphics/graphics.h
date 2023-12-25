#ifndef __SDBOX_GRAPHICS_H__
#define __SDBOX_GRAPHICS_H__

#include <glad/glad.h>

#include <sdbox.h>

namespace sdbox {

void RenderQuad();
void CleanupGeometry();

void GLAPIENTRY OpenGLErrorCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
    const void* userParam);

}

#endif // __SDBOX_GRAPHICS_H__