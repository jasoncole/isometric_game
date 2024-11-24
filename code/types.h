/* date = June 3rd 2024 1:11 pm */
#ifndef TYPES_H
#define TYPES_H

// TODO: handmade includes this file in platform.h
// also platform.h has an extern C
// look into this

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
// TODO(casey): Moar compilerz!!!
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_LLVM
#include <x86intrin.h>
#else
#error SEE/NEON optimizations are not available for this compiler yet!!!!
#endif


// TODO: implement sine ourselves
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <float.h>

#define Real32Maximum FLT_MAX
#define U32Maximum 0xFFFFFFFF

#define internal static 
#define local_persist static 
#define global_variable static 

#define Ps32 3.14159265359f
#define EPSILON 0.0000001

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef intptr_t intptr;
typedef uintptr_t uintptr;

typedef size_t memory_index;

typedef int8 s8;
typedef int16 s16;
typedef int32 s32;
typedef int64 s64;

typedef uint8 u8;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef float real32;
typedef double real64;

typedef float f32;
typedef double f64;

typedef uintptr_t umm;
typedef intptr_t smm;

typedef bool32 b32;

typedef b32 b32x;
typedef u32 u32x;

#if RPG_SLOW
#define Assert(Expression) if(!(Expression)) {*(int*)0 = 0;}
#else
#define Assert(Expression)
#endif

#define InvalidCodePath Assert(!"InvalidCodePath");
#define InvalidDefaultCase default: {InvalidCodePath;} break

#define Kilobytes(Value) ((Value)* 1024LL)
#define Megabytes(Value) (Kilobytes(Value)* 1024LL)
#define Gigabytes(Value) (Megabytes(Value)* 1024LL)
#define Terabytes(Value) (Gigabytes(Value)* 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define ABS(N) ((N)<0?-(N):(N))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define DIV32(A)        ( ((A)+31) /32)
#define SetBit(A,k)     ( A[(k)/32] |= (1 << ((k)%32)) )
#define ClearBit(A,k)   ( A[(k)/32] &= ~(1 << ((k)%32)) )
#define TestBit(A,k)    ( A[(k)/32] & (1 << ((k)%32)) )

#define Align16(Value) ((Value + 15) & ~15)
#define OffsetOf(Struct, Member) (memory_index)&((Struct*)0)->Member

#if 0
// Doubly linked lists
#define DLLPushBack_NP(f,l,n, next, prev) (((f)==0?\
(f)=(l)=(n),(n)->next = (n)->prev=0:\
((n)->prev = (l),(l)->next=(n),(l)=(n), (n)->next=0))

#define DLLPushBack(f,l,n) DLLPushBack_NP(f,l,n,next,prev)

#define DLLPushFront(f,l,n) DLLPushBack_NP(l,f,n,prev,next)

#define DLLRemove_NP(f,l,n,next,prev) (((f) == (l) && (f) == (n))?\
(f) = 0, (l) = 0:\
(f) == (n)?\
((f) = (f)->next,(f)->prev = 0):\
(l) == (n) ?\
((l) = (l)->prev,(l)->next = 0):\
((n)->next->prev = (n)->prev,\
(n)->prev->next = (n)->next))

#define DLLRemove(f,l,n) DLLRemove_NP(f,l,n,next,prev)
#endif

#define ArrayCopy(dest, source) CopyCharArray((u8*)(dest), (u8*)(source), sizeof(dest))

#define RoundFloatToInt(f) ((int)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))

inline void
CopyCharArray(u8* dest, u8* source, u32 bytes)
{
    for (u32 index=0;
         index < bytes;
         ++index)
    {
        *(dest + index) = *(source + index);
    }
}

struct sv2
{
    union
    {
        struct
        {
            s32 x;
            s32 y;
        };
        
        s32 V[2];
    };
    
    s32& operator[](int index)
    {
        return this->V[index];
    }
    
    
};

union fv2
{
    struct
    {
        f32 x;
        f32 y;
    };
    
    f32 V[2];
    
    f32& operator[](int index)
    {
        return this->V[index];
    }
    
    
};

union fv3
{
    struct
    {
        f32 x, y, z;
    };
    
    struct
    {
        fv2 xy;
        f32 Ignored0_;
    };
    
    struct
    {
        f32 Ignored1_;
        fv2 yz;
    };
    
    f32 V[3];
    
    f32& operator[](int index)
    {
        return this->V[index];
    }
};



union fv4
{
    struct
    {
        union
        {
            fv3 xyz;
            struct
            {
                f32 x, y, z;
            };
        };
        f32 w;
    };
    
    struct
    {
        union
        {
            fv3 rgb;
            struct
            {
                f32 r, g, b;
            };
        };
        f32 a;
    };
    
    struct
    {
        fv2 xy;
        f32 Ignored0_;
        f32 Ignored1_;
    };
    
    struct
    {
        f32 Ignored2_;
        fv2 yz;
        f32 Ignored3_;
    };
    
    f32 V[4];
    
    f32& operator[](int index)
    {
        return this->V[index];
    }
    
};

union sv2Rectangle
{
    struct
    {
        sv2 Min;
        sv2 Max;
    };
    struct
    {
        s32 MinX;
        s32 MinY;
        s32 MaxX;
        s32 MaxY;
    };
};

struct fv2Rectangle
{
    fv2 Min;
    fv2 Max;
};

struct fv3Cube
{
    fv3 Min;
    fv3 Max;
};




#endif //TYPES_H
