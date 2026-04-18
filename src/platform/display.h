#pragma once

#include <vector>
#include <vector_types.h>

#include "shader_program.h"
#include "texture2d.h"
#include "screen_quad.h"

class Texture2D;
class ScreenQuad;

class Display {
public:
    Display(const int width, const int height);
    void display(const uchar4* pixels);

private:

    ShaderProgram _shader;
    Texture2D _texture;
    ScreenQuad _quad;
};