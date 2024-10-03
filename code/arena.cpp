
struct arena
{
    u8* base;
    u64 bytes_used;
    u64 max_bytes;
};

#define PushArray(arena_, type, count) (type *)ArenaPush((arena_), sizeof(type)*(count))
#define PushStruct(arena_, type) PushArray(arena_, type, 1)

// TODO: implement
#define PushZeroArray(arena_, type, count) PushArray(arena_, type, count)
#define PushZeroSTruct(arena_, type) PushStruct(arena_, type)

inline arena 
MakeArena(void* base, u64 size)
{
    arena ret = {};
    
    ret.base = (u8*)base;
    ret.bytes_used = 0;
    ret.max_bytes = size;
    
    return ret;
};

void*
ArenaPush(arena* Arena, u64 bytes)
{
    void* old_location = 0;
    if (Arena->bytes_used + bytes <= Arena->max_bytes)
    {
        old_location = (void*)(Arena->base + Arena->bytes_used);
        Arena->bytes_used += bytes;
    }
    else
    {
        InvalidCodePath;
    }
    return old_location;
}

inline void
ArenaReset(arena* Arena)
{
    Arena->bytes_used = 0;
}

inline arena
NestArena(arena* outer_arena, u64 size)
{
    return MakeArena(ArenaPush(outer_arena, size), size);
}

inline void
ArenaPop(arena* Arena, u8* location)
{
    Arena->bytes_used = location - Arena->base;
}

