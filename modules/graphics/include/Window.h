#ifndef FUZZY_GRAPHICS_WINDOW_H
#define FUZZY_GRAPHICS_WINDOW_H

#include "GLFW/glfw3.h"
#include <iostream>

namespace graphics {

    class Window {
    public:
        Window(int width, int height, std::string title, bool isFullScreen, bool vsync);

        ~Window();

        void setKeyCallback(GLFWkeyfun callback);

        void setMouseCallback(GLFWcursorposfun callback);

        void setScrollCallback(GLFWscrollfun callback);

        bool isClosing();

        void setIsShoudClose(bool isShoudClose);

        bool isVSyncEnabled();

        void setVSync(bool vsync);

        void makeContextCurrent();

        void setInputMode(int mode, int value);

        void swapBuffers();

    private:
        GLFWwindow *_window;
        int _width;
        int _height;
        std::string _title;
        bool _vsync;
    };

}

#endif //FUZZY_GRAPHICS_WINDOW_H
