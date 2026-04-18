#pragma once

#include <vector_types.h>

#include <glad/glad.h>
#include <cstdint>

class Texture2D {
public:
    Texture2D(int width, int height);
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    void upload(const uint8_t* data);
    void upload(const uchar4* data);
    void bind(int unit = 0) const;

private:
    GLuint _id = 0;
    int _width = 0;
    int _height = 0;
};