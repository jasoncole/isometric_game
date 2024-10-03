/* date = September 22nd 2024 10:34 pm */

#ifndef MATH_H
#define MATH_H

inline s32
s32Clamp(s32 low, s32 high, s32 value)
{
    Assert(high > low);
    if (value > high)
    {
        return high;
    }
    else if (value < low)
    {
        return low;
    }
    return value;
}

inline f32
f32Clamp(f32 low, f32 high, f32 value)
{
    Assert(high > low);
    if (value > high)
    {
        return high;
    }
    else if (value < low)
    {
        return low;
    }
    return value;
}

inline f32
f32Clamp01(f32 value)
{
    return f32Clamp(0.0f, 1.0f, value);
}

inline f32
Square(f32 Value)
{
    f32 ret = Value * Value;
    return ret;
}

inline fv2
sv2Tofv2(sv2 V)
{
    fv2 ret = {
        (f32)V.x,
        (f32)V.y
    };
    return ret;
}

inline f32
fv2RSquared(fv2 V)
{
    f32 ret =  V.x * V.x + V.y * V.y;
    return ret;
}

inline f32
fv3RSquared(fv3 V)
{
    return V.x * V.x + V.y * V.y + V.z * V.z;
}

inline f32
fv4RSquared(fv4 V)
{
    return V.x * V.x + V.y * V.y + V.z * V.z + V.a * V.a;
}

inline f32 
fv2Length(fv2 V)
{
    f32 ret = (f32)sqrt(fv2RSquared(V));
    return ret;
}

inline f32 
fv3Length(fv3 V)
{
    f32 ret = (f32)sqrt(fv3RSquared(V));
    return ret;
}

inline f32 
fv4Length(fv4 V)
{
    f32 ret = (f32)sqrt(fv4RSquared(V));
    return ret;
}

inline fv2
fv2Normalize(fv2 V)
{
    f32 l = fv2Length(V);
    return V/l;
}

inline fv3
fv3Normalize(fv3 V)
{
    f32 l = fv3Length(V);
    return V/l;
}

inline fv4
fv4Normalize(fv4 V)
{
    f32 l = fv4Length(V);
    return V/l;
}

inline fv2
fv2Hadamard(fv2 A, fv2 B)
{
    fv2 ret = {A.x * B.x, A.y * B.y};
    return ret;
}

inline fv3
fv3Hadamard(fv3 A, fv3 B)
{
    fv3 ret = {A.x * B.x, A.y * B.y, A.z * B.z};
    return ret;
}

inline fv4
fv4Hadamard(fv4 A, fv4 B)
{
    fv4 ret = {A.x * B.x, A.y * B.y, A.z * B.z, A.w * B.w};
    return ret;
}

inline f32
fv2Dot(fv2 A, fv2 B)
{
    return A.x*B.x + A.y*B.y;
}

inline f32
fv3Dot(fv3 A, fv3 B)
{
    return A.x*B.x + A.y*B.y + A.z*B.z;
}

inline f32
fv4Dot(fv4 A, fv4 B)
{
    return A.x*B.x + A.y*B.y + A.z*B.z + A.a*B.a;
}

inline f32
fv2Proj(fv2 A, fv2 B)
{
    return (fv2Dot(A, B) / fv2Length(B));
}

inline fv2
fv2Perp(fv2 V)
{
    fv2 Result = {-V.y, V.x};
    return(Result);
}

inline real32
f32Lerp(f32 A, f32 t, f32 B)
{
    f32 Result = (1.0f - t) * A + t * B;
    return Result;
}

inline fv2
fv2Lerp(fv2 A, f32 t, fv2 B)
{
    fv2 Result = (1.0f - t) * A + t * B;
    return Result;
}

inline fv3
fv3Lerp(fv3 A, f32 t, fv3 B)
{
    fv3 Result = (1.0f - t) * A + t * B;
    return Result;
}

inline fv4
fv4Lerp(fv4 A, f32 t, fv4 B)
{
    fv4 Result = (1.0f - t) * A + t * B;
    return Result;
}

#endif //MATH_H
