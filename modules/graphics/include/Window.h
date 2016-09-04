#ifndef FUZZY_MODULES_GRAPHICS_WINDOW_H
#define FUZZY_MODULES_GRAPHICS_WINDOW_H

#include "GLFW/glfw3.h"
#include <iostream>

class Window {
public:
    int width;
    int height;
    std::string title;

    Window(int width, int height, std::string title, bool isFullScreen, bool vsync);
    ~Window();

    GLFWwindow* get();
    bool isClosing();
    void setIsShoudClose(bool isShoudClose);
    bool isVSyncEnabled();
    void setVSync(bool vsync);
    void setInputMode(int mode, int value);
    void update();
private:
    GLFWwindow* window;
    bool vsync;
};

#endif //FUZZY_MODULES_GRAPHICS_WINDOW_H
