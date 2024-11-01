/* date = June 5th 2024 3:18 pm */

#ifndef RPG_PLATFORM_H
#define RPG_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include "types.h"
    
    inline uint32
        SafeTruncateUInt64(uint64 Value)
    {
        Assert(Value <= 0xFFFFFFFF);
        uint32 Result = (uint32)Value;
        return(Result);
    }
    
    typedef struct thread_context
    {
        int Placeholder;
    } thread_context;
    
    // Services that the platform layer provides to the game
#if RPG_INTERNAL
    struct debug_read_file_result
    {
        uint32 ContentSize;
        void* Content;
    };
    
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context* Thread, void* Memory)
    typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
    
#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context* Thread, char* Filename)
    typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
    
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context* Thread, char* Filename, uint32 MemorySize, void* Memory)
    typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);
    
    
    enum
    {
        DebugCycleCounter_GameUpdateAndRender,
        DebugCycleCounter_ResolveRenderBuffer,
        DebugCycleCounter_ProcessPixel,
        DebugCycleCounter_Count,
    };
    typedef struct debug_cycle_counter
    {
        u64 CycleCount;
        u32 HitCount;
    } debug_cycle_counter;
    
    extern struct game_memory* DebugGlobalMemory;
#if _MSC_VER
#define BEGIN_TIMED_BLOCK(ID) uint64 StartCycleCount##ID = __rdtsc();
#define END_TIMED_BLOCK(ID) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += __rdtsc() - StartCycleCount##ID; ++DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount;
#define END_TIMED_BLOCK_COUNTED(ID, Count) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += __rdtsc() - StartCycleCount##ID; DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount += (Count);
#else
#define BEGIN_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK(ID)
#endif
    
#endif
    
    
    // services that the game provides to the platform layer
    // (this may expand in the future)
    
#define BITMAP_BYTES_PER_PIXEL 4
    typedef struct game_offscreen_buffer
    {
        // NOTE(casey): Pixels are always 32-bits wide, Memory Order BB GG RR XX
        void *Memory;
        int Width;
        int Height;
        int Pitch;
    } game_offscreen_buffer;
    
    struct game_sound_output_buffer
    {
        int SamplesPerSecond;
        int SampleCount;
        int16* Samples;
    };
    
    struct game_button_state
    {
        int HalfTransitionCount;
        bool32 EndedDown;
    };
    
    struct game_controller_input
    {
        bool32 IsConnected;
        bool32 IsAnalog;
        real32 StickAverageX;
        real32 StickAverageY;
        
        union
        {
            game_button_state Buttons[4];
            struct
            {
                game_button_state Select;
                game_button_state Move;
                game_button_state CameraGrip;
                game_button_state Attack;
                //
                
                game_button_state Terminator;
            };
        };
    };
    
    
    
    struct game_input
    {
        // TODO: insert clock value here
        game_controller_input Controllers[5];
        
        fv2 MousePos;
        f32 MouseZ;
        
        real32 dtForFrame;
    };
    
    
    inline game_controller_input* 
        GetController(game_input* Input, int unsigned ControllerIndex)
    {
        Assert(ControllerIndex < ArrayCount(Input->Controllers));
        game_controller_input* Result = &Input->Controllers[ControllerIndex];
        return(Result);
    }
    
    inline b32 
        WasPressed(game_button_state State)
    {
        b32 Result = ((State.HalfTransitionCount > 1) ||
                      ((State.HalfTransitionCount == 1) && (State.EndedDown)));
        
        return(Result);
    }
    
    inline b32 
        IsDown(game_button_state State)
    {
        b32 Result = (State.EndedDown);
        
        return(Result);
    }
    
    struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue* Queue, void* Data)
    typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);
    
    
    typedef void platform_add_entry(platform_work_queue* Queue, platform_work_queue_callback* Callback, void* Data);
    typedef void platform_complete_all_work(platform_work_queue* Queue);
    struct game_memory
    {
        uint64 Size;
        void* Base; // required to be cleared to 0 at startup
        
        platform_work_queue* HighPriorityQueue;
        platform_work_queue* LowPriorityQueue;
        
        platform_add_entry* PlatformAddEntry;
        platform_complete_all_work* PlatformCompleteAllWork;
        
        debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
        debug_platform_free_file_memory* DEBUGPlatformFreeFileMemory;
        debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile;
        
#if RPG_INTERNAL
        debug_cycle_counter Counters[DebugCycleCounter_Count];
#endif
    };
    
#define GAME_INITIALIZE_MEMORY(name) void name(thread_context* Thread, game_memory* Memory)
    typedef GAME_INITIALIZE_MEMORY(game_initialize_memory); 
    GAME_INITIALIZE_MEMORY(GameInitializeMemoryStub)
    {
    }
    
#define GAME_UPDATE_AND_RENDER(name) void name(thread_context* Thread, game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer)
    typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
    GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
    {
    }
    
    // Needs to be a VERY fast function
#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context* Thread, game_memory* Memory, game_sound_output_buffer* SoundBuffer)
    typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
    GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub)
    {
    }
    
#ifdef __cplusplus
}
#endif


#endif //RPG_PLATFORM_H
