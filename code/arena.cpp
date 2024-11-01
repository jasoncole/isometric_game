
struct arena
{
    u8* Base;
    memory_index Used;
    memory_index Size;
};

#define ArenaTop(a) ((a)->Base + (a)->Used)
#define PushArray(arena_, type, count, ...) (type *)ArenaPush((arena_), sizeof(type)*(count), ##__VA_ARGS__)
#define PushStruct(arena_, type, ...) PushArray(arena_, type, 1, ##__VA_ARGS__)

// TODO: implement
#define PushZeroArray(arena_, type, count, ...) PushArray(arena_, type, count, ##__VA_ARGS__)
#define PushZeroStruct(arena_, type, ...) PushStruct(arena_, type, ##__VA_ARGS__)

inline arena 
MakeArena(void* Base, memory_index Size)
{
    arena ret = {};
    
    ret.Base = (u8*)Base;
    ret.Used = 0;
    ret.Size = Size;
    
    return ret;
};

inline memory_index
GetAlignmentOffset(arena* Arena, memory_index Alignment)
{
    memory_index AlignmentOffset = 0;
    
    memory_index UsedLocation = (memory_index)Arena->Base + Arena->Used;
    memory_index AlignmentMask = Alignment - 1;
    if (UsedLocation & AlignmentMask)
    {
        AlignmentOffset = Alignment - (UsedLocation & AlignmentMask);
    }
    
    return AlignmentOffset;
}

inline memory_index
GetArenaSizeRemaining(arena* Arena, memory_index Alignment = 4)
{
    memory_index Result = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Alignment));
}

void*
ArenaPush(arena* Arena, memory_index Bytes, memory_index Alignment = 4)
{
    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
    Bytes += AlignmentOffset;
    
    if (Arena->Used + Bytes <= Arena->Size)
    {
        void* Result = Arena->Base + Arena->Used + AlignmentOffset;
        Arena->Used += Bytes;
        return Result;
    }
    
    InvalidCodePath;
    return 0;
}

inline void
ArenaReset(arena* Arena)
{
    Arena->Used = 0;
}

inline arena
NestArena(arena* outer_arena, memory_index size, memory_index Alignment = 4)
{
    arena Result = MakeArena(ArenaPush(outer_arena, size, Alignment), size);
    return Result;
}

inline void
ArenaPop(arena* Arena, u8* location)
{
    Arena->Used = location - Arena->Base;
}

inline void
ArenaPopIndex(arena* Arena, memory_index UsedIndex)
{
    Arena->Used = UsedIndex;
}

inline u8*
ArenaPrint(arena* Arena, char* str)
{
    u8* At = ArenaTop(Arena);
    u8* ret = At;
    while (*str)
    {
        *At++ = (u8)(*str);
        str++;
        Arena->Used++;
    }
    return ret;
}

