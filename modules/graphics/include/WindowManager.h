#pragma once

#include "Window.h"

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
