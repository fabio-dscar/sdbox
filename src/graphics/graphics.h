#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include <glad/glad.h>

#include <sdbox.h>

namespace sdbox {
void GLAPIENTRY OpenGLErrorCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
    const void* userParam);
}

#endif // __GRAPHICS_H__