#pragma once

#include <cstddef>

struct GLFWwindow;

class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    bool shouldClose() const;
    void setShouldClose(bool value);

    void pollEvents() const;
    void swapBuffers() const;

    GLFWwindow* handle() const;
    int width() const;
    int height() const;

    float consumeScrollOffsetY();
    bool isLeftMouseButtonDown() const;
    bool isRightMouseButtonDown() const;
    bool consumeMouseDelta(double& dx, double& dy);

private:
    // callbacks
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    GLFWwindow* _window = nullptr;
    size_t _width;
    size_t _height;

    float _scrollOffsetY = 0.0f;

    bool _leftMouseDown = false;
    bool _rightMouseDown = false;
    double _lastMouseX = 0.0;
    double _lastMouseY = 0.0;
    bool _hasLastMousePosition = false;
    double _mouseDeltaX = 0.0;
    double _mouseDeltaY = 0.0;
};