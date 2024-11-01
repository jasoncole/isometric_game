/* date = September 22nd 2024 11:57 pm */

#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "math.h"

#if COMPILER_MSVC
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier();
inline uint32 AtomicCompareExchangeUInt32(uint32 volatile* Value, uint32 Expected, uint32 New)
{
    uint32 Result = _InterlockedCompareExchange((long*)Value, Expected, New);
    
    return Result;
}
#else
// TODO(casey): Need to define these on GCC/LLVM?
#endif

inline int32
SignOf(int32 Value)
{
    int32 Result = (Value >= 0) ? 1 : -1;
    return(Result);
}

inline real32
SquareRoot(real32 Real32)
{
    real32 Result = sqrtf(Real32);
    return(Result);
}

inline real32
AbsoluteValue(real32 Real32)
{
    real32 Result = fabsf(Real32);
    return(Result);
}

inline uint32
RotateLeft(uint32 Value, int32 Amount)
{
#if COMPILER_MSVC
    uint32 Result = _rotl(Value, Amount);
#else
    // TODO(casey): Actually port this to other compiler platforms!
    Amount &= 31;
    uint32 Result = ((Value << Amount) | (Value >> (32 - Amount)));
#endif
    
    return(Result);
}

inline uint32
RotateRight(uint32 Value, int32 Amount)
{
#if COMPILER_MSVC
    uint32 Result = _rotr(Value, Amount);
#else
    // TODO(casey): Actually port this to other compiler platforms!
    Amount &= 31;
    uint32 Result = ((Value >> Amount) | (Value << (32 - Amount)));
#endif
    
    return(Result);
}

inline s32
RoundReal32ToInt32(f32 Real32)
{
    s32 Result = _mm_cvtss_si32(_mm_set_ss(Real32));
    return(Result);
}

inline u32
RoundReal32ToUInt32(real32 Real32)
{
    u32 Result = (u32)_mm_cvtss_si32(_mm_set_ss(Real32));
    return(Result);
}

inline s32
s32Floor(real32 Real32)
{
    return (s32)floorf(Real32);
}

inline s32
s32Ceiling(real32 Real32)
{
    return (s32)ceilf(Real32);
}

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Pointer, Count) ZeroSize(Count * sizeof(Pointer[0]), Pointer)
inline void
ZeroSize(memory_index Size, void *Ptr)
{
    // TODO(casey): Check this guy for performance
    uint8 *Byte = (uint8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

inline void
CopySize(memory_index Size, void* Src, void* Dest)
{
    // TODO(casey): Check this guy for performance
    uint8* ByteSrc = (uint8*)Src;
    uint8* ByteDest = (uint8*)Dest;
    while(Size--)
    {
        *ByteDest++ = *ByteSrc++;
    }
}


inline real32
Sin(real32 Angle)
{
    real32 Result = sinf(Angle);
    return(Result);
}

inline real32
Cos(real32 Angle)
{
    real32 Result = cosf(Angle);
    return(Result);
}

inline real32
ATan2(real32 Y, real32 X)
{
    real32 Result = atan2f(Y, X);
    return(Result);
}


struct bit_scan_result
{
    bool32 Found;
    uint32 Index;
};
inline bit_scan_result
FindLeastSignificantSetBit(uint32 Value)
{
    bit_scan_result Result = {};
    
#if COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
    for(s32 Test = 0;
        Test < 32;
        ++Test)
    {
        if(Value & (1 << Test))
        {
            Result.Index = Test;
            Result.Found = true;
            break;
        }
    }
#endif
    
    return(Result);
}

inline bit_scan_result
FindMostSignificantSetBit(uint32 Value)
{
    bit_scan_result Result = {};
    
#if COMPILER_MSVC
    Result.Found = _BitScanReverse((unsigned long *)&Result.Index, Value);
#else
    for(s32 Test = 32;
        Test > 0;
        --Test)
    {
        if(Value & (1 << (Test - 1)))
        {
            Result.Index = Test - 1;
            Result.Found = true;
            break;
        }
    }
#endif
    
    return(Result);
}

#endif //INTRINSICS_H
