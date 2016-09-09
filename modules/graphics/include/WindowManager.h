#ifndef FUZZY_GRAPHICS_WINDOWMANAGER_H
#define FUZZY_GRAPHICS_WINDOWMANAGER_H

#include "Window.h"
#include <string>
#include <iostream>

namespace graphics {

    class WindowManager {
    public:
        static WindowManager& Instance();
        WindowManager(WindowManager const&) = delete;
        void operator =(WindowManager const&) = delete;

        Window createWindow(int width,
                            int height,
                            std::string title,
                            bool isFullScreen = false,
                            bool vsync = true);
    private:
        WindowManager();
        ~WindowManager();
    };

};

#endif //FUZZY_GRAPHICS_WINDOWMANAGER_H
