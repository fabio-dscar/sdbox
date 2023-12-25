#include <graphics.h>

using namespace sdbox;

static GLuint QuadVao, QuadVbo;

void sdbox::RenderQuad() {
    if (QuadVao == 0) {
        // Positions and uvs
        const float quadVertices[] = {-1.0f, 1.0f, 0.0f,  0.0f, 1.0f, -1.0f, -1.0f,
                                      0.0f,  0.0f, 0.0f,  1.0f, 1.0f, 0.0f,  1.0f,
                                      1.0f,  1.0f, -1.0f, 0.0f, 1.0f, 0.0f};

        constexpr std::size_t vertexSize = 5 * sizeof(float);

        glCreateVertexArrays(1, &QuadVao);
        glCreateBuffers(1, &QuadVbo);
        glEnableVertexArrayAttrib(QuadVao, 0);
        glVertexArrayVertexBuffer(QuadVao, 0, QuadVbo, 0, vertexSize);
        glVertexArrayAttribFormat(QuadVao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(QuadVao, 1);
        glVertexArrayVertexBuffer(QuadVao, 1, QuadVbo, 3 * sizeof(float), vertexSize);
        glVertexArrayAttribFormat(QuadVao, 1, 2, GL_FLOAT, GL_FALSE, 0);
        glNamedBufferStorage(QuadVbo, sizeof(quadVertices), &quadVertices, 0);
    }

    glBindVertexArray(QuadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void sdbox::CleanupGeometry() {
    if (QuadVao != 0) {
        glDeleteVertexArrays(1, &QuadVao);
        glDeleteBuffers(1, &QuadVbo);
    }
}

void GLAPIENTRY sdbox::OpenGLErrorCallback(
    GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*) {
    LOGT("[OPENGL] Type = 0x{:x}, Severity = 0x{:x}, Message = {}", type, severity, message);
}