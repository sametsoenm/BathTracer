#pragma once

#include <glad/glad.h>

class ShaderProgram {
public:
    ShaderProgram(const char* vertexSrc, const char* fragmentSrc);
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    void use() const;
    void setInt(const char* name, int value) const;

private:
    GLuint _id = 0;
};

namespace shaders {

    constexpr const char* vertexShaderSrc = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aUV;
    
        out vec2 vUV;
    
        void main() {
            vUV = aUV;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";
    
    constexpr const char* fragmentShaderSrc = R"(
        #version 330 core
        in vec2 vUV;
        out vec4 FragColor;
    
        uniform sampler2D uTexture;
    
        void main() {
            FragColor = texture(uTexture, vUV);
        }
    )";
}