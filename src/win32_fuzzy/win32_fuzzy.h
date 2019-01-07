#pragma once

#define WIN32_FILE_PATH MAX_PATH

struct win32_state {
    u64 TotalSize;
    void *GameMemoryBlock;

    char EXEDirectoryFullPath[WIN32_FILE_PATH];
};

struct win32_game_code {
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;

    game_update_and_render *UpdateAndRender;

    b32 IsValid;
};

GAME_PRINT_OUTPUT(GamePrintOutput) {
    OutputDebugStringA(Output);
}

GAME_READ_TEXT_FILE(GameReadTextFile) {
    std::ifstream In(Path);

    assert(In.good());

    std::ostringstream Result;
    Result << In.rdbuf();
    return Result.str();
}
