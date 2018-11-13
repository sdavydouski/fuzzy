#include <windows.h>
#include <cstdlib>
#include <string>
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>

#include "..\..\generated\glad\src\glad.c"
#include <GLFW/glfw3.h>

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "fuzzy_types.h"
#include "fuzzy_memory.h"
#include "fuzzy_platform.h"
#include "win32_fuzzy.h"

#pragma warning(disable:4302)
#pragma warning(disable:4311)

game_params GameParams = {};

// todo: should probably move these out...
s32 StringLength(const char* String) {
    s32 Length = 0;

    while (*String++) {
        ++Length;
    }

    return Length;
}

void ConcatenateStrings(const char* SourceA, const char* SourceB, char* Dest) {
    s32 SourceALength = StringLength(SourceA);
    for (s32 Index = 0; Index < SourceALength; ++Index) {
        *Dest++ = *SourceA++;
    }

    s32 SourceBLength = StringLength(SourceB);
    for (s32 Index = 0; Index < SourceBLength; ++Index) {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}

void Win32GetFullPathToEXEDirectory(win32_state* State) {
    char EXEFullPath[WIN32_FILE_PATH];
    GetModuleFileNameA(0, EXEFullPath, sizeof(EXEFullPath));

    char* OnePastLastEXEFullPathSlash = EXEFullPath;

    for (char* Scan = EXEFullPath; *Scan; ++Scan) {
        if (*Scan == '\\') {
            OnePastLastEXEFullPathSlash = Scan + 1;
        }
    }

    for (char Index = 0; Index < OnePastLastEXEFullPathSlash - EXEFullPath; ++Index) {
        State->EXEDirectoryFullPath[Index] = EXEFullPath[Index];
    }
}

FILETIME Win32GetLastWriteTime(const char* FileName) {
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA FileInformation;
    if (GetFileAttributesExA(FileName, GetFileExInfoStandard, &FileInformation)) {
        LastWriteTime = FileInformation.ftLastWriteTime;
    }

    return LastWriteTime;
}

b32 Win32FileExists(const char* FileName) {
    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    b32 Exists = GetFileAttributesExA(FileName, GetFileExInfoStandard, &Ignored);

    return Exists;
}

win32_game_code Win32LoadGameCode(const char* SourceDLLName, const char* TempDLLName, const char* LockFileName) {
    win32_game_code Result = {};

    if (!Win32FileExists(LockFileName)) {
        Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

        CopyFile(SourceDLLName, TempDLLName, false);

        Result.GameCodeDLL = LoadLibraryA(TempDLLName);

        if (Result.GameCodeDLL) {
            Result.UpdateAndRender = (game_update_and_render*)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");

            Result.IsValid = (b32)Result.UpdateAndRender;
        }
    }

    return Result;
}

void Win32UnloadGameCode(win32_game_code* GameCode) {
    if (GameCode->GameCodeDLL) {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }

    GameCode->IsValid = false;
    GameCode->UpdateAndRender = 0;
}

void Win32InitOpenGLRenderer(game_memory* GameMemory) {
    GameMemory->Renderer = {};
    GameMemory->Renderer.glCreateShader = glCreateShader;
    GameMemory->Renderer.glShaderSource = glShaderSource;
    GameMemory->Renderer.glCompileShader = glCompileShader;
    GameMemory->Renderer.glGetShaderiv = glGetShaderiv;
    GameMemory->Renderer.glGetShaderInfoLog = glGetShaderInfoLog;
    GameMemory->Renderer.glDeleteShader = glDeleteShader;
    GameMemory->Renderer.glGetUniformLocation = glGetUniformLocation;
    GameMemory->Renderer.glUniform1i = glUniform1i;
    GameMemory->Renderer.glUniform2f = glUniform2f;
    GameMemory->Renderer.glUniformMatrix4fv = glUniformMatrix4fv;
    GameMemory->Renderer.glGenTextures = glGenTextures;
    GameMemory->Renderer.glBindTexture = glBindTexture;
    GameMemory->Renderer.glTexParameteri = glTexParameteri;
    GameMemory->Renderer.glTexImage2D = glTexImage2D;
    GameMemory->Renderer.glCreateProgram = glCreateProgram;
    GameMemory->Renderer.glAttachShader = glAttachShader;
    GameMemory->Renderer.glLinkProgram = glLinkProgram;
    GameMemory->Renderer.glGetProgramiv = glGetProgramiv;
    GameMemory->Renderer.glGetProgramInfoLog = glGetProgramInfoLog;
    GameMemory->Renderer.glUseProgram = glUseProgram;
    GameMemory->Renderer.glGenVertexArrays = glGenVertexArrays;
    GameMemory->Renderer.glBindVertexArray = glBindVertexArray;
    GameMemory->Renderer.glGenBuffers = glGenBuffers;
    GameMemory->Renderer.glBindBuffer = glBindBuffer;
    GameMemory->Renderer.glBufferData = glBufferData;
    GameMemory->Renderer.glBufferSubData = glBufferSubData;
    GameMemory->Renderer.glVertexAttribPointer = glVertexAttribPointer;
    GameMemory->Renderer.glVertexAttribIPointer = glVertexAttribIPointer;
    GameMemory->Renderer.glEnableVertexAttribArray = glEnableVertexAttribArray;
    GameMemory->Renderer.glVertexAttribDivisor = glVertexAttribDivisor;
    GameMemory->Renderer.glBlendFunc = glBlendFunc;
    GameMemory->Renderer.glClear = glClear;
    GameMemory->Renderer.glClearColor = glClearColor;
    GameMemory->Renderer.glDrawArraysInstanced = glDrawArraysInstanced;
}

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

    GameMemory.Platform = {};
    GameMemory.Platform.PrintOutput = GamePrintOutput;
    GameMemory.Platform.ReadTextFile = GameReadTextFile;
    GameMemory.Platform.ReadJsonFile = GameReadJsonFile;
    GameMemory.Platform.ReadImageFile = stbi_load;
    GameMemory.Platform.FreeImageFile = stbi_image_free;

    Win32InitOpenGLRenderer(&GameMemory);

    Win32GetFullPathToEXEDirectory(&Win32State);

    char SourceGameCodeDLLFullPath[WIN32_FILE_PATH];
    ConcatenateStrings(Win32State.EXEDirectoryFullPath, "fuzzy.dll", SourceGameCodeDLLFullPath);

    char TempGameCodeDLLFullPath[WIN32_FILE_PATH];
    ConcatenateStrings(Win32State.EXEDirectoryFullPath, "fuzzy_temp.dll", TempGameCodeDLLFullPath);

    char GameCodeLockFullPath[WIN32_FILE_PATH];
    ConcatenateStrings(Win32State.EXEDirectoryFullPath, "lock.tmp", GameCodeLockFullPath);

    win32_game_code GameCode = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);
    
    GameParams.ScreenWidth = 1280;
    GameParams.ScreenHeight = 720;
    
    if (!glfwInit()) {
        OutputDebugStringA("Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    srand((u32) glfwGetTimerValue());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);

    GLFWwindow* window = glfwCreateWindow(GameParams.ScreenWidth, GameParams.ScreenHeight, "Fuzzy", nullptr, nullptr);
    if (!window) {
        OutputDebugStringA("Failed to create GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwSetWindowPos(window, (vidmode->width - GameParams.ScreenWidth) / 2, (vidmode->height - GameParams.ScreenHeight) / 2);

    glfwSetKeyCallback(window, [](GLFWwindow* window, s32 key, s32 scancode, s32 action, s32 mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (action == GLFW_PRESS) {
            GameParams.Input.Keys[key] = true;
        } else if (action == GLFW_RELEASE) {
            GameParams.Input.Keys[key] = false;
            GameParams.Input.ProcessedKeys[key] = false;
        }
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, s32 width, s32 height) {
        glViewport(0, 0, width, height);
    });

    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        OutputDebugStringA("Failed to initialize OpenGL context\n");
        return EXIT_FAILURE;
    }

    char* OpenGLVersion = (char*)glGetString(GL_VERSION);
    OutputDebugStringA(OpenGLVersion);
    OutputDebugStringA("\n");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0, 0, GameParams.ScreenWidth, GameParams.ScreenHeight);
    
    f64 lastTime = glfwGetTime();
    f64 currentTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
        if (CompareFileTime(&NewDLLWriteTime, &GameCode.DLLLastWriteTime) != 0) {
            Win32UnloadGameCode(&GameCode);
            GameCode = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);
        }

        currentTime = glfwGetTime();
        GameParams.Delta = (f32) (currentTime - lastTime);
        lastTime = currentTime;

        glfwPollEvents();

        if (GameCode.IsValid) {
            GameCode.UpdateAndRender(&GameMemory, &GameParams);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
