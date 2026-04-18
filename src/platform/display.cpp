#include "display.h"

Display::Display(int width, int height)
    : _shader(shaders::vertexShaderSrc, shaders::fragmentShaderSrc),
    _texture(width, height),
    _quad()
{
    _shader.use();
    _shader.setInt("uTexture", 0);
}

void Display::display(const uchar4* pixels) {

    _texture.upload(pixels);
    _shader.use();
    _texture.bind(0);
    _quad.draw();
}