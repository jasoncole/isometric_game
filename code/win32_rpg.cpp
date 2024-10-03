/*
 * TODO: THIS IS NOT A FINAL PLATFORM LAYER
 *
 * Saved game locations
 * Getting a handle to our own exec
 * Asset loading path
 * Threading
 * Raw Input
 * Sleep/timeBeginPeriod
 * ClipCursor for multimonitor support
 * Fullscreen support
 * WM_SETCURSOR (cursor visibility)
 * QueryCancelAutoplay
 * WM_ACTIVATEAPP (when not active)
 * Blit speed improvements (BitBlt)
 * Hardware Acceleration (OpenGL or Direct3D or Both)
 * GetKeyboardLayout
 *
 * Just a partial list ;)
 */

#include "rpg.h"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_rpg.h"

//TODO: This is a global for now
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;
global_variable HCURSOR DEBUGGlobalCursor; // TODO: custom cursor

// X_INPUT_GET_STATE
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// X_INPUT_SET_STATE
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// DIRECT_SOUND_CREATE
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
CatStrings(size_t SourceACount, char *SourceA,
           size_t SourceBCount, char *SourceB,
           size_t DestCount, char *Dest)
{
    // TODO(casey): Dest bounds checking!
    
    for(int Index = 0;
        Index < SourceACount;
        ++Index)
    {
        *Dest++ = *SourceA++;
    }
    
    for(int Index = 0;
        Index < SourceBCount;
        ++Index)
    {
        *Dest++ = *SourceB++;
    }
    
    *Dest++ = 0;
}

internal void
Win32GetEXEFileName(win32_state *State)
{
    // NOTE(casey): Never use MAX_PATH in code that is user-facing, because it
    // can be dangerous and lead to bad results.
    DWORD SizeOfFilename = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
    State->OnePastLastEXEFileNameSlash = State->EXEFileName;
    for(char *Scan = State->EXEFileName;
        *Scan;
        ++Scan)
    {
        if(*Scan == '\\')
        {
            State->OnePastLastEXEFileNameSlash = Scan + 1;
        }
    }
}

internal int
StringLength(char *String)
{
    int Count = 0;
    while(*String++)
    {
        ++Count;
    }
    return(Count);
}

internal void
Win32BuildEXEPathFileName(win32_state *State, char *FileName,
                          int DestCount, char *Dest)
{
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName,
               StringLength(FileName), FileName,
               DestCount, Dest);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};
    
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Content = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Result.Content)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.Content, FileSize32, &BytesRead, 0) &&
                   (FileSize32 == BytesRead))
                {
                    // File read successfully
                    Result.ContentSize = FileSize32; 
                }
                else
                {
                    // TODO: logging
                    DEBUGPlatformFreeFileMemory(Thread, Result.Content);
                    Result.Content = 0;
                }
            }
            else
            {
                // TODO: logging
            }
        }
        else
        {
            // TODO: logging
        }
        CloseHandle(FileHandle);
    }
    else
    {
        // TODO: logging
    }
    
    return(Result);
}


DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 Result = false;
    
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0)) 
        {
            // File read successfully
            Result = (BytesWritten == MemorySize);
        }
        else
        {
            // TODO: logging
        }
        
        CloseHandle(FileHandle);
    }
    else
    {
        // TODO: logging
    }
    
    return(Result);
    
}

inline FILETIME
Win32GetLastWriteTime(char* Filename)
{
    FILETIME LastWriteTime = {};
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }
    
    return(LastWriteTime);
}

internal win32_game_code
Win32LoadGameCode(char* SourceDLLName, char* TempDLLName, char* LockFileName)
{
    win32_game_code Result = {};
    
    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    if (!GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &Ignored))
    {
        Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);
        
        //CopyFile("w:/rpg/code/rpg.exe", "w:/rpg/code/rpg_temp.dll", FALSE);
        CopyFile(SourceDLLName, TempDLLName, FALSE);
        
        Result.GameCodeDLL = LoadLibraryA(TempDLLName);
        if(Result.GameCodeDLL)
        {
            Result.UpdateAndRender = (game_update_and_render*)
                GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
            Result.GetSoundSamples = (game_get_sound_samples*)
                GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");
            Result.IsValid = (Result.UpdateAndRender &&
                              Result.GetSoundSamples);
        }
    }
    
    if(!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
        Result.GetSoundSamples = 0;
    }
    
    return(Result);
}

internal void
Win32UnloadGameCode(win32_game_code* GameCode)
{
    if(GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }
    GameCode->IsValid = false;
    GameCode->UpdateAndRender = 0;
    GameCode->GetSoundSamples = 0;
    
}

internal void
Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
        // TODO: diagnostic
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    if(!XInputLibrary)
    {
        // TODO: diagnostic
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }   
    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}
        XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputSetState) {XInputSetState = XInputSetStateStub;}
        
        // TODO: diagnostic
    }
    else
    {
        // TODO: diagnostic
    }
}

internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    // Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    
    if(DSoundLibrary)
    {
        // Get a DirectSound object
        direct_sound_create *DirectSoundCreate = (direct_sound_create*)
            GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        
        LPDIRECTSOUND DirectSound;
	    if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;
            
            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                
                // Create a primary buffer
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {
                    HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
                    if(SUCCEEDED(Error))
                    {
                        // we have finally set the format
                        OutputDebugStringA("Primary buffer format was set.\n");
                    }
                    else
                    {
                        // TODO: diagnostic
                    }
                    
                }
                else
                {
                    // TODO: diagnostic
                }
            }
            else
            {
                // TODO: diagnostic
            }
            
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
            if(SUCCEEDED(Error))
            {
                OutputDebugStringA("Secondary buffer created successfully.\n");
            }
            else
            {
                // TODO: diagnostic
            }
        }
        else
        {
            // TODO: diagnostic
        }
        
    }
    else
    {
        // TODO: diagnostic
    }
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;  
    
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    
    return(Result);
}



internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    //TODO: bulletproof this
    
    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }
    
    Buffer->Width = Width;
    Buffer->Height = Height;
    
    int BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;
    
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width*BytesPerPixel;
    
    //TODO: clear to black
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer,
                           HDC DeviceContext, int WindowWidth, int WindowHeight)    
{
    int OffsetX = 10;
    int OffsetY = 10;
    PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
    PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
    PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
    PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);
    
    //TODO: aspect ratio correction
    StretchDIBits(DeviceContext,
                  /*
                  X, Y, Width, Height,
                  X, Y, Width, Height,
                  */
                  OffsetX, OffsetY, Buffer->Width, Buffer->Height,
                  0,0, Buffer->Width, Buffer->Height,
                  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK 
Win32MainWindowCallback(HWND   Window,
                        UINT   Message,
                        WPARAM WParam, 
                        LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message) {
        case WM_CLOSE:
        {
            // TODO: Handle this with a message?
            GlobalRunning = false;
        } break;
        
        case WM_SETCURSOR:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_DESTROY:
        {
            // TODO: Handle this as an error - recreate window
            GlobalRunning = false;
        } break;
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert("Keyboard input came in through a non-dispatch message");
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;
        default:
        {
            // OutputDebugStringA("default\n");
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    return(Result);
}



internal void
Win32ClearSoundBuffer(win32_sound_output* SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
                                             &Region1, &Region1Size,
                                             &Region2, &Region2Size,
                                             0)))
    {
        // TODO: assert even multiple of sample size
        uint8* DestSample = (uint8*)Region1;
        for (DWORD ByteIndex = 0;
             ByteIndex < Region1Size;
             ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        
        DestSample = (uint8*)Region2;
        for (DWORD ByteIndex = 0;
             ByteIndex < Region2Size;
             ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void
Win32FillSoundBuffer(win32_sound_output* SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
                     game_sound_output_buffer* SourceBuffer)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                             &Region1, &Region1Size,
                                             &Region2, &Region2Size,
                                             0)))
    {
        // TODO: assert even multiple of sample size
        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        int16* DestSample = (int16*)Region1;
        int16* SourceSample = SourceBuffer->Samples;
        for (DWORD SampleIndex = 0;
             SampleIndex < Region1SampleCount;
             ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        DestSample = (int16*)Region2;
        for (DWORD SampleIndex = 0;
             SampleIndex < Region2SampleCount;
             ++SampleIndex)
        {   
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void
Win32ProcessKeyboardMessage(game_button_state* NewState,
                            bool32 IsDown)
{
    if (NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

#if 0
// TODO: Controller Support?
internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
                                game_button_state* OldState, DWORD ButtonBit,
                                game_button_state* NewState)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal real32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadzoneThreshold)
{
    real32 Result = 0;
    if(Value < -DeadzoneThreshold)
    {
        Result = (real32)((Value + DeadzoneThreshold) / (32768.0f - DeadzoneThreshold));
    }
    else if(Value > DeadzoneThreshold)
    {
        Result = (real32)((Value - DeadzoneThreshold) / (32767.0f - DeadzoneThreshold));
    }
    return(Result);
}
#endif

internal void
Win32ProcessPendingMessages(game_controller_input *KeyboardController)
{
    MSG Message;
    
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;
            case WM_SYSKEYUP:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_KEYDOWN:
            {
                uint32 VKCode = (uint32)Message.wParam;
                bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                if(WasDown != IsDown)
                {
#if RPG_INTERNAL
                    if(VKCode == 'P')
                    {
                        if(IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                    }
#endif
                    if (VKCode == 'F')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Attack, IsDown);
                    }
                    
                }
                bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
                if((VKCode == VK_F4) && AltKeyWasDown)
                {
                    GlobalRunning = false;
                }
            } break;
            case WM_LBUTTONDOWN:
            {
                Win32ProcessKeyboardMessage(&KeyboardController->Select, 1);
            } break;
            case WM_LBUTTONUP:
            {
                Win32ProcessKeyboardMessage(&KeyboardController->Select, 0);
            } break;
            case WM_RBUTTONDOWN:
            {
                Win32ProcessKeyboardMessage(&KeyboardController->Move, 1);
            } break;
            case WM_RBUTTONUP:
            {
                Win32ProcessKeyboardMessage(&KeyboardController->Move, 0);
            } break;
            case WM_MBUTTONDOWN:
            {
                Win32ProcessKeyboardMessage(&KeyboardController->CameraGrip, 1);
            } break;
            case WM_MBUTTONUP:
            {
                Win32ProcessKeyboardMessage(&KeyboardController->CameraGrip, 0);
            } break;
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }
}

inline LARGE_INTEGER
Win32GetWallClock(void)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return(Result);
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / 
                     (real32)GlobalPerfCountFrequency);
    return(Result);
}

#if 0
internal void
Win32DebugDrawVertical(win32_offscreen_buffer* BackBuffer,
                       int X, int Top, int Bottom, uint32 Color)
{
    if(Top<=0)
    {
        Top = 0;
    }
    if(Bottom > BackBuffer->Height)
    {
        Bottom = BackBuffer->Height;
    }
    if((X>=0)&&(X<BackBuffer->Width))
    {
        uint8* Pixel = ((uint8*)BackBuffer->Memory +
                        X*BackBuffer->BytesPerPixel +
                        Top*BackBuffer->Pitch);
        for (int Y = Top;
             Y < Bottom;
             ++Y)
        {
            *(uint32*)Pixel = Color;
            Pixel += BackBuffer->Pitch;
        }
    }
}

inline void
Win32DrawSoundBufferMarker(win32_offscreen_buffer* BackBuffer, 
                           win32_sound_output* SoundOutput, 
                           real32 C, int PadX, int Top, int Bottom,
                           DWORD Value, uint32 Color)
{
    real32 XReal32 = (C * (real32)Value);
    int X = PadX + (int)XReal32;
    Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);
}


internal void
Win32DebugSyncDisplay(win32_offscreen_buffer* BackBuffer, 
                      int MarkerCount, win32_debug_time_marker* Markers,
                      int CurrentMarkerIndex,
                      win32_sound_output* SoundOutput, real32 TargetSecondsPerFrame)
{
    int PadX = 16;
    int PadY = 16;
    
    int LineHeight = 64;
    
    real32 C = (real32)(BackBuffer->Width - 2*PadX) / (real32)SoundOutput->SecondaryBufferSize;
    for (int MarkerIndex  = 0;
         MarkerIndex < MarkerCount;
         ++MarkerIndex)
    {
        win32_debug_time_marker* ThisMarker = &Markers[MarkerIndex];
        Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);
        
        DWORD PlayColor = 0xFFFFFFFF;
        DWORD WriteColor = 0xFFFF0000;
        DWORD ExpectedFlipColor = 0xFFFFFF00;
        DWORD PlayWindowColor = 0xFFFF00FF;
        
        int Top = PadY;
        int Bottom = PadY + LineHeight; 
        if(MarkerIndex == CurrentMarkerIndex)
        {
            Top+=LineHeight+PadY;
            Bottom +=LineHeight+PadY;
            
            int FirstTop = Top;
            
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);
            
            Top += LineHeight+PadY;
            Bottom += LineHeight+PadY;
            
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);
            
            Top+=LineHeight+PadY;
            Bottom +=LineHeight+PadY;
            
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);
        }
        
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + 480*SoundOutput->BytesPerSample, PlayWindowColor);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
    }
    
}
#endif


int CALLBACK
WinMain(HINSTANCE Instance, 
        HINSTANCE PrevInstance, 
        LPSTR CommandLine, 
        int ShowCode)
{
    win32_state Win32State = {};
    
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    
    Win32GetEXEFileName(&Win32State);
    
    char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&Win32State, "rpg.dll",
                              sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);
    
    char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&Win32State, "rpg_temp.dll",
                              sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);
    
    char GameCodeLockFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&Win32State, "lock.tmp",
                              sizeof(GameCodeLockFullPath), GameCodeLockFullPath);
    
    // set scheduler granularity to 1MS for 1MS sleep
    UINT DesiredSchedulerMS = 1;
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);
    
    Win32LoadXInput();
    
    WNDCLASSA WindowClass = {};
    
    Win32ResizeDIBSection(&GlobalBackBuffer, 960, 540);
    
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    // WindowClass.hIcon = ;
    WindowClass.lpszClassName =  "BouncerWindowClass";
    
    
    if(RegisterClass(&WindowClass))
    {
        HWND Window= 
            CreateWindowExA(0,
                            WindowClass.lpszClassName,
                            "Bouncer",  
                            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            0,
                            0,
                            Instance,
                            0);
        if(Window)
        {
            // Sound Test
            win32_sound_output SoundOutput = {};
            
            //TODO: How do we reliably query this on windows?
            int MonitorRefreshHz = 60;
            HDC RefreshDC = GetDC(Window);
            int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(Window, RefreshDC);
            if (Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }
            real32 GameUpdateHz = (MonitorRefreshHz / 2.0f);
            real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;
            
            // TODO(casey): make this like sixty seconds?
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.BytesPerSample = sizeof(int16)*2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
            // TODO: compute this variance
            SoundOutput.SafetyBytes = (int)(((real32)SoundOutput.SamplesPerSecond*(real32)SoundOutput.BytesPerSample / GameUpdateHz)/3.0f);
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearSoundBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0,0,DSBPLAY_LOOPING);   
            
            GlobalRunning = true;
            
#if 0
            // NOTE(casey): This tests the PlayCursor/WriteCursor update frequency
            // On the Handmade Hero machine, it was 480 samples.
            while(GlobalRunning)
            {
                DWORD PlayCursor;
                DWORD WriteCursor;
                GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
                
                char TextBuffer[256];
                _snprintf_s(TextBuffer, sizeof(TextBuffer),
                            "PC:%u WC:%u\n", PlayCursor, WriteCursor);
                OutputDebugStringA(TextBuffer);
            }
#endif
            
            
            // TODO: Pool with bitmap VirtualAlloc  
            int16* Samples = (int16*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, 
                                                  MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            
#if RPG_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
            LPVOID BaseAddress = 0;
#endif
            game_memory GameMemory = {};
            GameMemory.Size = Gigabytes(1);
            
            GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
            
            // TODO(casey): Handle various memory footprints (USING SYSTEM METRICS)
            // TODO(casey): Use MEM_LARGE_PAGES and call adjust token
            // privileges when not on Windows XP?
            Win32State.TotalSize = GameMemory.Size;
            Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize, 
                                                      MEM_RESERVE|MEM_COMMIT, 
                                                      PAGE_READWRITE);
            GameMemory.Base = Win32State.GameMemoryBlock;
            
            if(Samples && GameMemory.Base)
            {   
                
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];
                
                LARGE_INTEGER LastCounter = Win32GetWallClock();
                LARGE_INTEGER FlipWallClock = Win32GetWallClock();
                
                int DebugTimeMarkerIndex= 0;
                win32_debug_time_marker DebugTimeMarkers[30] = {0};
                
                DWORD AudioLatencyBytes = 0;
                real32 AudioLatencySeconds = 0;
                bool32 SoundIsValid = false;
                
                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                         TempGameCodeDLLFullPath,
                                                         GameCodeLockFullPath);
                uint32 LoadCounter = 0;
                
                uint64 LastCycleCount = __rdtsc();
                
                game_initialize_memory* InitializeMemory = (game_initialize_memory*)
                    GetProcAddress(Game.GameCodeDLL, "GameInitializeMemory");
                
                if (InitializeMemory)
                {
                    InitializeMemory(0, &GameMemory);
                }
                
                while(GlobalRunning) 
                {
                    NewInput->dtForFrame = TargetSecondsPerFrame;
                    
                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, 
                                                 TempGameCodeDLLFullPath,
                                                 GameCodeLockFullPath);
                        LoadCounter = 0;
                    }
                    
                    game_controller_input* OldKeyboardController = GetController(OldInput, 0);
                    game_controller_input* NewKeyboardController = GetController(NewInput, 0); 
                    *NewKeyboardController = {};
                    NewKeyboardController->IsConnected = true;
                    for(int ButtonIndex = 0;
                        ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
                        ++ButtonIndex)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown = 
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }
                    
                    Win32ProcessPendingMessages(NewKeyboardController);
                    
                    if(!GlobalPause)
                    {
                        win32_window_dimension Dim = Win32GetWindowDimension(Window);
                        POINT MouseP;
                        GetCursorPos(&MouseP);
                        ScreenToClient(Window, &MouseP);
                        // TODO: don't hardcode this
                        NewInput->MousePos.x = ((f32)MouseP.x - 10.0f) / GlobalBackBuffer.Width;
                        NewInput->MousePos.y = ((f32)MouseP.y - 10.0f) / GlobalBackBuffer.Height;
                        NewInput->MouseZ = 0.0f; // TODO(casey): mousewheel support
                    }
                    thread_context Thread = {};
                    
                    game_offscreen_buffer Buffer = {};
                    Buffer.Memory = (void*)GlobalBackBuffer.Memory;
                    Buffer.Width = GlobalBackBuffer.Width;
                    Buffer.Height = GlobalBackBuffer.Height;
                    Buffer.Pitch = GlobalBackBuffer.Pitch;  
                    
                    if (Game.UpdateAndRender)
                    {
                        Game.UpdateAndRender(&Thread, &GameMemory, NewInput, &Buffer);
                    }
                    
                    
                    LARGE_INTEGER AudioWallClock = Win32GetWallClock();
                    real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);
                    
                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                    { 
                        /* Here is how sound output computation works:
                         * When we wake up to write audio,
                         * we will look to see what the play cursor position is
                         * then we will forecast where we think it will be on the next frame boundary
                         *
                         * we define a guard sample size we think our game update loop could vary by
                         * 
                         * we will look to see if the write cursor is before that (minus guard amount)
                         * if it is, the target fill position is the frame boundary + 1 frame
                         * this gives us perfect sync with a fast card
                         *
                         * if the write cursor is after the next frame boundary,
                         * then we assume we can never sync properly,
                         * so we write one frame's worth + some guard samples
                         * (based on variability of frame computation)
                         */
                        
                        if(!SoundIsValid)
                        {
                            SoundOutput.RunningSampleIndex = (WriteCursor / SoundOutput.BytesPerSample);
                            SoundIsValid = true;
                        }
                        
                        DWORD ByteToLock = ((SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) %
                                            SoundOutput.SecondaryBufferSize);
                        
                        DWORD ExpectedSoundBytesPerFrame = 
                        (int)((real32)(SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) 
                              / GameUpdateHz);
                        real32 SecondsLeftUntilFlip = (TargetSecondsPerFrame - FromBeginToAudioSeconds);
                        DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame)*(real32)ExpectedSoundBytesPerFrame);
                        
                        DWORD ExpectedPlayCursorPos = PlayCursor + ExpectedSoundBytesPerFrame;
                        
                        DWORD SafeWriteCursor = WriteCursor;
                        if(SafeWriteCursor < PlayCursor)
                        {
                            SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                        }
                        Assert(SafeWriteCursor >= PlayCursor);
                        SafeWriteCursor += SoundOutput.SafetyBytes;
                        
                        bool32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedPlayCursorPos);
                        
                        DWORD TargetCursor = 0;
                        if(AudioCardIsLowLatency)
                        {
                            TargetCursor = (ExpectedPlayCursorPos + ExpectedSoundBytesPerFrame);
                        }
                        else
                        {
                            TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
                        }
                        TargetCursor = (TargetCursor % SoundOutput.SecondaryBufferSize);
                        
                        DWORD BytesToWrite = 0;
                        if(ByteToLock > TargetCursor)
                        {
                            BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;    
                            BytesToWrite += TargetCursor;
                        }
                        else
                        {
                            BytesToWrite = TargetCursor - ByteToLock;
                        }
                        
                        game_sound_output_buffer SoundBuffer = {};
                        SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                        SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                        SoundBuffer.Samples = Samples;
                        if (Game.GetSoundSamples)
                        {
                            Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);
                        }
#if 0
                        win32_debug_time_marker* Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                        Marker->OutputPlayCursor = PlayCursor;
                        Marker->OutputWriteCursor = WriteCursor;
                        Marker->OutputLocation = ByteToLock;
                        Marker->OutputByteCount = BytesToWrite;
                        Marker->ExpectedFlipPlayCursor = ExpectedPlayCursorPos;
                        
                        DWORD UnwrappedWriteCursor = WriteCursor;
                        if(UnwrappedWriteCursor < PlayCursor)
                        {
                            UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                        }
                        AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
                        AudioLatencySeconds = (((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) / 
                                               (real32)SoundOutput.SamplesPerSecond);
                        char TextBuffer[256];
                        _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                    "BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA %u (%fs)\n",
                                    ByteToLock, TargetCursor, BytesToWrite,
                                    PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
                        OutputDebugStringA(TextBuffer);
#endif
                        Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer); 
                    }
                    else
                    {
                        SoundIsValid = false;
                    }
                    
                    LARGE_INTEGER WorkCounter = Win32GetWallClock();
                    real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
                    
                    // TODO(casey): NOT TESTED YET, PROBABLY BUGGY
                    real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                    if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {
                        if(SleepIsGranular)
                        {
                            DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                            if(SleepMS > 0)
                            {
                                Sleep(SleepMS);
                            }
                        }
                        
                        real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                        
                        if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            // TODO: logging, missed sleep
                        }
                        while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                        }
                    }
                    else
                    {
                        // TODO: logging, missed framerate
                    }
                    
                    LARGE_INTEGER EndCounter = Win32GetWallClock();
                    real32 MSPerFrame = 1000.0f*Win32GetSecondsElapsed(LastCounter, EndCounter);
                    LastCounter = EndCounter;
                    
                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    HDC DeviceContext = GetDC(Window);
                    Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,
                                               Dimension.Width, Dimension.Height);
                    ReleaseDC(Window, DeviceContext);
                    
                    FlipWallClock = Win32GetWallClock();
#if RPG_INTERNAL
                    {
                        DWORD PlayCursor_; 
                        DWORD WriteCursor_; 
                        if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor_, &WriteCursor_) == DS_OK)
                        {
                            Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
                            win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                            
                            
                            Marker->FlipPlayCursor = PlayCursor_;
                            Marker->FlipWriteCursor = WriteCursor_;
                        }
                    }
#endif
                    
                    game_input* Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;
                    
                    uint64 EndCycleCount = __rdtsc();
                    uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    LastCycleCount = EndCycleCount;
                    
                    real64 FPS = 1000.0f / MSPerFrame;
                    real64 MCPF = ((real64)CyclesElapsed/1000.0f/1000.0f);
                    
                    char FPSBuffer[256];
                    _snprintf_s(FPSBuffer, sizeof(FPSBuffer), 
                                "%.02fms/f, %.02ff/s, %.02fMc/f\n", MSPerFrame, FPS, MCPF);
                    OutputDebugStringA(FPSBuffer);
                    
#if RPG_INTERNAL
                    ++DebugTimeMarkerIndex;
                    if(DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
                    {
                        DebugTimeMarkerIndex= 0;
                    }
#endif
                }
            }
        }
        else
        {
            // TODO: logging
        }
    }
    else
    {
        // TODO: logging
    }
    
    return(0);
}


