#include "screen_quad.h"

#include <utility>

ScreenQuad::ScreenQuad() {
    const float quadVertices[] = {
        // position   // uv
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 0.0f,

        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f
    };

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // position xy
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        reinterpret_cast<void*>(0)
    );
    glEnableVertexAttribArray(0);

    // uv
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

ScreenQuad::~ScreenQuad() {
    if (_vbo != 0) {
        glDeleteBuffers(1, &_vbo);
        _vbo = 0;
    }

    if (_vao != 0) {
        glDeleteVertexArrays(1, &_vao);
        _vao = 0;
    }
}

ScreenQuad::ScreenQuad(ScreenQuad&& other) noexcept
    : _vao(other._vao),
    _vbo(other._vbo)
{
    other._vao = 0;
    other._vbo = 0;
}

ScreenQuad& ScreenQuad::operator=(ScreenQuad&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (_vbo != 0) {
        glDeleteBuffers(1, &_vbo);
    }

    if (_vao != 0) {
        glDeleteVertexArrays(1, &_vao);
    }

    _vao = other._vao;
    _vbo = other._vbo;

    other._vao = 0;
    other._vbo = 0;

    return *this;
}

void ScreenQuad::draw() const {
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}