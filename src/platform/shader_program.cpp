#include "shader_program.h"

#include <stdexcept>
#include <utility>

namespace {

    GLuint compileShader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            glDeleteShader(shader);
            throw std::runtime_error(infoLog);
        }

        return shader;
    }

}

ShaderProgram::ShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    _id = glCreateProgram();
    glAttachShader(_id, vs);
    glAttachShader(_id, fs);
    glLinkProgram(_id);

    GLint success = 0;
    glGetProgramiv(_id, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(_id, 512, nullptr, infoLog);

        glDeleteShader(vs);
        glDeleteShader(fs);
        glDeleteProgram(_id);

        throw std::runtime_error(infoLog);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

ShaderProgram::~ShaderProgram() {
    if (_id != 0) {
        glDeleteProgram(_id);
        _id = 0;
    }
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : _id(other._id)
{
    other._id = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (_id != 0) {
        glDeleteProgram(_id);
    }

    _id = other._id;
    other._id = 0;

    return *this;
}

void ShaderProgram::use() const {
    glUseProgram(_id);
}

void ShaderProgram::setInt(const char* name, int value) const {
    GLint location = glGetUniformLocation(_id, name);
    glUniform1i(location, value);
}