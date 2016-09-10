#pragma once

#include "GLFW/glfw3.h"
#include <iostream>

namespace graphics {

    class Window {
    public:
        Window(int width,
               int height,
               std::string title,
               bool isFullScreen = false,
               bool vsync = true);

        ~Window();

        void setKeyCallback(GLFWkeyfun callback);

        void setMouseCallback(GLFWcursorposfun callback);

        void setScrollCallback(GLFWscrollfun callback);

        bool isClosing();

        void setIsShoudClose(bool isShouldClose);

        bool isVSyncEnabled();

        void setVSync(bool vsync);

        void makeContextCurrent();

        void setInputMode(int mode, int value);

        void swapBuffers();

        void destroy();

    private:
        GLFWwindow *_window;
        int _width;
        int _height;
        std::string _title;
        bool _vsync;
    };

}
