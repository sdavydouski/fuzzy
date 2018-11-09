#include <Windows.h>
//#include <glad/glad.h>
#include "../../generated/glad/src/glad.c"
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <string>
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>

#include "types.h"
#include "win32_fuzzy.h"
#include "fuzzy_platform.h"

#pragma warning(disable:4302)
#pragma warning(disable:4311)

const s32 SCREEN_WIDTH = 1280;
const s32 SCREEN_HEIGHT = 720;

constexpr inline u64 Kilobytes(u64 bytes) {
    u64 Result = bytes * 1024;
    return Result;
}

constexpr inline u64 Megabytes(u64 bytes) {
    u64 Result = Kilobytes(bytes) * 1024;
    return Result;
}

constexpr inline u64 Gigabytes(u64 bytes) {
    u64 Result = Megabytes(bytes) * 1024;
    return Result;
}

constexpr inline u64 Terabytes(u64 bytes) {
    u64 Result = Gigabytes(bytes) * 1024;
    return Result;
}

struct win32_state {
    u64 TotalSize;
    void* GameMemoryBlock;
};

struct win32_game_code {
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;

    game_update_and_render* UpdateAndRender;

    b32 IsValid;
};

GAME_PRINT_OUTPUT(GamePrintOutput) {
    OutputDebugStringA(Output.c_str());
}

GAME_READ_TEXT_FILE(GameReadTextFile) {
    std::ifstream In(Path);

    assert(In.good());

    std::ostringstream Result;
    Result << In.rdbuf();
    return Result.str();
}

GAME_READ_JSON_FILE(GameReadJsonFile) {
    std::fstream In(Path);

    assert(In.good());
    
    json Result;
    In >> Result;
    return Result;
}

game_params GameParams = {};

s32 CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, s32 nCmdShow) {
    win32_state Win32State = {};
    game_memory GameMemory = {};

    GameMemory.PermanentStorageSize = Megabytes(64);
    GameMemory.TransientStorageSize = Megabytes(128);

    void* BaseAddress = (void*)Terabytes(2);

    Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
    Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, Win32State.TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
    GameMemory.TransientStorage = (u8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;

    GameMemory.PlatformAPI = {};
    GameMemory.PlatformAPI.PrintOutput = GamePrintOutput;
    GameMemory.PlatformAPI.ReadTextFile = GameReadTextFile;
    GameMemory.PlatformAPI.ReadJsonFile = GameReadJsonFile;

    win32_game_code GameCode = {};
    GameCode.GameCodeDLL = LoadLibraryA("fuzzy.dll");

    if (GameCode.GameCodeDLL) {
        GameCode.UpdateAndRender = (game_update_and_render*) GetProcAddress(GameCode.GameCodeDLL, "GameUpdateAndRender");
        GameCode.IsValid = (b32)GameCode.UpdateAndRender;
    }

    GameParams.ScreenWidth = SCREEN_WIDTH;
    GameParams.ScreenHeight = SCREEN_HEIGHT;
    
    if (!glfwInit()) {
        OutputDebugStringA("Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    srand((u32) glfwGetTimerValue());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Fuzzy", nullptr, nullptr);
    if (!window) {
        OutputDebugStringA("Failed to create GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);

    glfwSetWindowPos(window, (vidmode->width - SCREEN_WIDTH) / 2, (vidmode->height - SCREEN_HEIGHT) / 2);

    glfwSetKeyCallback(window, [](GLFWwindow* window, s32 key, s32 scancode, s32 action, s32 mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (action == GLFW_PRESS) {
            GameParams.Keys[key] = true;
        } else if (action == GLFW_RELEASE) {
            GameParams.Keys[key] = false;
            GameParams.ProcessedKeys[key] = false;
        }
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, s32 width, s32 height) {
        glViewport(0, 0, width, height);
    });

    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    //if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    //    OutputDebugStringA("Failed to initialize OpenGL context\n");
    //    return EXIT_FAILURE;
    //}
    
    f64 lastTime = glfwGetTime();
    f64 currentTime = glfwGetTime();

    //f32 wait = 0.f;

    while (!glfwWindowShouldClose(window)) {
        currentTime = glfwGetTime();
        GameParams.Delta = (f32) (currentTime - lastTime);
        lastTime = currentTime;

        //wait += GameParams.Delta;

        glfwPollEvents();

        if (GameCode.IsValid) {
            GameCode.UpdateAndRender(&GameMemory, &GameParams);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
