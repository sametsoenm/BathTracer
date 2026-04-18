#include "texture2d.h"

#include <utility>


Texture2D::Texture2D(int width, int height)
    : _width(width), _height(height)
{
    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB8,
        _width,
        _height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D::~Texture2D() {
    if (_id != 0) {
        glDeleteTextures(1, &_id);
        _id = 0;
    }
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : _id(other._id),
    _width(other._width),
    _height(other._height)
{
    other._id = 0;
    other._width = 0;
    other._height = 0;
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (_id != 0) {
        glDeleteTextures(1, &_id);
    }

    _id = other._id;
    _width = other._width;
    _height = other._height;

    other._id = 0;
    other._width = 0;
    other._height = 0;

    return *this;
}

void Texture2D::upload(const uint8_t* data) {
    glBindTexture(GL_TEXTURE_2D, _id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        _width,
        _height,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        data
    );
}

void Texture2D::upload(const uchar4* data) {
    glBindTexture(GL_TEXTURE_2D, _id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        _width,
        _height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
    );
}

void Texture2D::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, _id);
}