#pragma once

#include <glad/glad.h>

class ScreenQuad {
public:
    ScreenQuad();
    ~ScreenQuad();

    ScreenQuad(const ScreenQuad&) = delete;
    ScreenQuad& operator=(const ScreenQuad&) = delete;

    ScreenQuad(ScreenQuad&& other) noexcept;
    ScreenQuad& operator=(ScreenQuad&& other) noexcept;

    void draw() const;

private:
    GLuint _vao = 0;
    GLuint _vbo = 0;
};
