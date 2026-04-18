#include "window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

#include "rendersettings.h"


Window::Window(int __width, int __height, const char* title) :
    _width(__width), _height(__height) {

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    _window = glfwCreateWindow(_width, _height, title, nullptr, nullptr);
    if (!_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowAspectRatio(_window, _width, _height);
    glfwMakeContextCurrent(_window);
    glfwSwapInterval(1);

    // callbacks
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferSizeCallback);
    glfwSetScrollCallback(_window, scrollCallback);
    glfwSetCursorPosCallback(_window, Window::cursorPositionCallback);
    glfwSetMouseButtonCallback(_window, Window::mouseButtonCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(_window);
        _window = nullptr;
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(_window, &fbWidth, &fbHeight);
    _width = fbWidth;
    _height = fbHeight;
    glViewport(0, 0, fbWidth, fbHeight);
}

Window::~Window() {
    if (_window) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }

    glfwTerminate();
}

Window::Window(Window&& other) noexcept
    : _window(other._window),
    _width(other._width),
    _height(other._height)
{
    other._window = nullptr;
    other._width = 0;
    other._height = 0;

    if (_window) {
        glfwSetWindowUserPointer(_window, this);
    }
}

Window& Window::operator=(Window&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (_window) {
        glfwDestroyWindow(_window);
    }

    _window = other._window;
    _width = other._width;
    _height = other._height;

    other._window = nullptr;
    other._width = 0;
    other._height = 0;

    if (_window) {
        glfwSetWindowUserPointer(_window, this);
    }

    return *this;
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(_window);
}

void Window::setShouldClose(bool value) {
    glfwSetWindowShouldClose(_window, value ? GLFW_TRUE : GLFW_FALSE);
}

void Window::pollEvents() const {
    glfwPollEvents();
}

void Window::swapBuffers() const {
    glfwSwapBuffers(_window);
}

GLFWwindow* Window::handle() const {
    return _window;
}

int Window::width() const {
    return _width;
}

int Window::height() const {
    return _height;
}

float Window::consumeScrollOffsetY() {
    const float value = _scrollOffsetY;
    _scrollOffsetY = 0.0f;
    return value;
}

bool Window::isLeftMouseButtonDown() const {
    return _leftMouseDown;
}

bool Window::isRightMouseButtonDown() const {
    return _rightMouseDown;
}

bool Window::consumeMouseDelta(double& dx, double& dy) {
    dx = _mouseDeltaX;
    dy = _mouseDeltaY;

    _mouseDeltaX = 0.0;
    _mouseDeltaY = 0.0;

    return dx != 0.0 || dy != 0.0;
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);

    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->_width = width;
        self->_height = height;
    }
}

void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self)
        return;

    self->_scrollOffsetY += static_cast<float>(yoffset);
}

void Window::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }

    if (!self->_hasLastMousePosition) {
        self->_lastMouseX = xpos;
        self->_lastMouseY = ypos;
        self->_hasLastMousePosition = true;
        return;
    }

    const double dx = xpos - self->_lastMouseX;
    const double dy = ypos - self->_lastMouseY;

    self->_lastMouseX = xpos;
    self->_lastMouseY = ypos;

    if (self->_leftMouseDown || self->_rightMouseDown) {
        self->_mouseDeltaX += dx;
        self->_mouseDeltaY += dy;
    }
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            self->_leftMouseDown = true;

            double xpos = 0.0;
            double ypos = 0.0;
            glfwGetCursorPos(window, &xpos, &ypos);

            self->_lastMouseX = xpos;
            self->_lastMouseY = ypos;
            self->_hasLastMousePosition = true;
        }
        else if (action == GLFW_RELEASE) {
            self->_leftMouseDown = false;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            self->_rightMouseDown = true;

            double xpos = 0.0;
            double ypos = 0.0;
            glfwGetCursorPos(window, &xpos, &ypos);

            self->_lastMouseX = xpos;
            self->_lastMouseY = ypos;
            self->_hasLastMousePosition = true;
        }
        else if (action == GLFW_RELEASE) {
            self->_rightMouseDown = false;
        }
    }
}