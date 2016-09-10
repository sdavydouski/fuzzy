#pragma once

#include "Window.h"
#include <string>
#include <iostream>

namespace graphics {

    class WindowManager {
    public:
        static WindowManager& Instance();
        WindowManager(WindowManager const&) = delete;
        void operator =(WindowManager const&) = delete;

        void startUp();
        void shutDown();
    private:
        WindowManager();
        ~WindowManager();
    };

};
