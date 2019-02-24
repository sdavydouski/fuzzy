#pragma once

#define WIN32_FILE_PATH MAX_PATH

struct win32_state
{
    u64 TotalSize;
    void *GameMemoryBlock;

    char EXEDirectoryFullPath[WIN32_FILE_PATH];
};

struct win32_game_code
{
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;

    game_update_and_render *UpdateAndRender;

    b32 IsValid;
};

PLATFORM_PRINT_OUTPUT(PlatformPrintOutput)
{
    OutputDebugStringA(Output);
}

PLATFORM_FREE_FILE(PlatformFreeFile)
{
    if (Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

PLATFORM_READ_FILE(PlatformReadFile)
{
    read_file_result Result = {};

    HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            u32 FileSize32 = (u32)FileSize.QuadPart;
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (Result.Contents)
            {
                DWORD BytesRead;
                if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && FileSize32 == BytesRead)
                {
                    Result.Size = FileSize32;
                }
                else
                {
                    PlatformFreeFile(Result.Contents);
                    Result.Size = 0;
                }
            }
        }
    }

    return Result;
}
