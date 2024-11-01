/* date = September 22nd 2024 10:34 pm */

#ifndef MATH_H
#define MATH_H

// Operator Overloads
// -------------------------------

// sv2

inline sv2 operator+(sv2 left, sv2 right)
{
    sv2 Result = {
        left.x + right.x,
        left.y + right.y
    };
    return Result;
}

inline sv2 operator-(sv2 left, sv2 right)
{
    sv2 Result = {
        left.x - right.x,
        left.y - right.y
    };
    return Result;
}

inline sv2& operator+=(sv2& left, sv2 right)
{
    left = left + right;
    return left;
}

inline sv2& operator-=(sv2& left, sv2 right)
{
    left = left - right;
    return left;
}

inline b32 operator!=(sv2 left, sv2 right)
{
    b32 ret = left.x != right.x;
    ret |= left.y != right.y;
    return ret;
}

inline b32 operator==(sv2 left, sv2 right)
{
    b32 ret = left.x == right.x;
    ret &= left.y == right.y;
    return ret;
}

// fv2

inline fv2 operator+(fv2 left, fv2 right)
{
    fv2 Result = {
        left.x + right.x,
        left.y + right.y
    };
    return Result;
}

inline fv2 operator-(fv2 left, fv2 right)
{
    fv2 Result = {
        left.x - right.x,
        left.y - right.y
    };
    return Result;
}

inline fv2 operator*(fv2 left, f32 Operand)
{
    fv2 Result = {
        left.x * Operand,
        left.y * Operand
    };
    return Result;
}

inline fv2 operator/(fv2 left, f32 Operand)
{
    f32 Inv = 1.0f/Operand;
    fv2 Result = {
        left.x * Inv,
        left.y * Inv
    };
    return Result;
}

inline fv2& operator+=(fv2& left, fv2 right)
{
    left = left + right;
    return left;
}

inline fv2& operator-=(fv2& left, fv2 right)
{
    left = left - right;
    return left;
}

fv2& operator*=(fv2& left, f32 right)
{
    left = left * right;
    return left;
}

inline fv2 operator*(f32 scalar, fv2 vector)
{
    fv2 ret = vector * scalar;
    return ret;
}

//fv3

inline fv3 operator+(fv3 left, fv3 right)
{
    fv3 Result = {
        left.x + right.x,
        left.y + right.y,
        left.z + right.z,
    };
    return Result;
}

inline fv3 operator-(fv3 left, fv3 right)
{
    fv3 Result = {
        left.x - right.x,
        left.y - right.y,
        left.z - right.z,
    };
    return Result;
}

fv3 operator*(fv3 vector, f32 scalar)
{
    fv3 Result = {
        vector.x * scalar,
        vector.y * scalar,
        vector.z * scalar,
    };
    return Result;
}

fv3 operator/(fv3 vector, f32 scalar)
{
    fv3 Result = {
        vector.x / scalar,
        vector.y / scalar,
        vector.z / scalar,
    };
    return Result;
}

fv3& operator+=(fv3& left, fv3 right)
{
    left = left + right;
    return left;
}

fv3& operator-=(fv3& left, fv3 right)
{
    left = left - right;
    return left;
}

fv3& operator*=(fv3& vector, f32 scalar)
{
    vector = vector * scalar;
    return vector;
}

b32 operator==(fv3 left, fv3 right)
{
    b32 ret = (left.x == right.x);
    ret &= (left.y == right.y);
    ret &= (left.z == right.z);
    return ret;
}

b32 operator!=(fv3 left, fv3 right)
{
    b32 ret = left == right;
    return !ret;
}

inline fv3
operator*(f32 scalar, fv3 vector)
{
    fv3 ret = {
        scalar*vector.x, 
        scalar*vector.y, 
        scalar*vector.z
    };
    return ret;
}

//fv4

inline fv4 operator+(fv4 left, fv4 right)
{
    fv4 Result = {
        left.x + right.x,
        left.y + right.y,
        left.z + right.z,
        left.w + right.w
    };
    return Result;
}

inline fv4 operator-(fv4 left, fv4 right)
{
    fv4 Result = {
        left.x - right.x,
        left.y - right.y,
        left.z - right.z,
        left.w - right.w
    };
    return Result;
}

fv4 operator*(fv4 vector, f32 scalar)
{
    fv4 Result = {
        vector.x * scalar,
        vector.y * scalar,
        vector.z * scalar,
        vector.w * scalar
    };
    return Result;
}

fv4 operator/(fv4 vector, f32 scalar)
{
    fv4 Result = {
        vector.x / scalar,
        vector.y / scalar,
        vector.z / scalar,
        vector.w / scalar
    };
    return Result;
}

fv4& operator+=(fv4& left, fv4 right)
{
    left = left + right;
    return left;
}

fv4& operator-=(fv4& left, fv4 right)
{
    left = left - right;
    return left;
}

fv4& operator*=(fv4& vector, f32 scalar)
{
    vector = vector * scalar;
    return vector;
}

b32 operator==(fv4 left, fv4 right)
{
    b32 ret = (left.x == right.x);
    ret &= (left.y == right.y);
    ret &= (left.z == right.z);
    ret &= (left.w == right.w);
    return ret;
}

b32 operator!=(fv4 left, fv4 right)
{
    b32 ret = left == right;
    return !ret;
}

inline fv4 operator*(f32 scalar, fv4 vector)
{
    fv4 ret = {
        scalar*vector.x, 
        scalar*vector.y,
        scalar*vector.z,
        scalar*vector.w
    };
    return ret;
}

// Clamp
// -------------------------------

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

// Vector Init
// -------------------------------

inline sv2
SV2(s32 x, s32 y)
{
    sv2 ret = {x, y};
    return ret;
}

inline fv2
FV2(f32 x, f32 y)
{
    fv2 ret = {x, y};
    return ret;
}

inline fv3
FV3(f32 x, f32 y, f32 z)
{
    fv3 ret = {x, y, z};
    return ret;
}

inline fv4
FV4(f32 x, f32 y, f32 z, f32 w)
{
    fv4 ret = {x, y, z, w};
    return ret;
}

// Square
// -------------------------------

inline f32
Square(f32 Value)
{
    f32 ret = Value * Value;
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
    f32 ret = V.x * V.x + V.y * V.y + V.z * V.z;
    return ret;
}

inline f32
fv4RSquared(fv4 V)
{
    f32 ret = Square(V.x) + Square(V.y) + Square(V.z) + Square(V.a);
    return ret;
}

// Vector Length
// -------------------------------

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

// Normalize
// -------------------------------
inline fv2
fv2Normalize(fv2 V)
{
    fv2 ret = V / fv2Length(V);
    return ret;
}

inline fv3
fv3Normalize(fv3 V)
{
    fv3 ret = V / fv3Length(V);
    return ret;
}

inline fv4
fv4Normalize(fv4 V)
{
    fv4 ret = V / fv4Length(V);
    return ret;
}

// Hadamard
// -------------------------------

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

// Dot
// -------------------------------

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

// Projection
// -------------------------------

inline f32
fv2Proj(fv2 A, fv2 B)
{
    return (fv2Dot(A, B) / fv2Length(B));
}

// Perpendicular
// -------------------------------

inline fv2
fv2Perp(fv2 V)
{
    fv2 ret = {
        V.y,
        -V.x
    };
    return ret;
}

inline fv3
fv3Cross(fv3 A, fv3 B)
{
    fv3 Result = {
        A.y * B.z - A.z * B.y,
        A.z * B.x - A.x * B.z,
        A.x * B.y - A.y * B.x
    };
    return(Result);
}

// Lerp
// -------------------------------

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

// Vector Conversions
// -------------------------------

inline sv2
Roundfv2tosv2(fv2 V)
{
    sv2 ret = {
        RoundReal32ToInt32(V.x),
        RoundReal32ToInt32(V.y)
    };
    return ret;
}

inline fv2
sv2tofv2(sv2 V)
{
    fv2 ret = {
        (f32)V.x,
        (f32)V.y
    };
    return ret;
}

inline fv3
fv2tofv3(fv2 V, f32 Z)
{
    fv3 ret = {
        V.x,
        V.y,
        Z
    };
    return ret;
}

inline fv4
fv3tofv4(fv3 V, f32 W)
{
    fv4 ret = {
        V.x,
        V.y,
        V.z,
        W
    };
    return ret;
}

// Misc
// -------------------------------

inline sv2Rectangle sv2RectangleIntersection(sv2Rectangle A, sv2Rectangle B)
{
    sv2Rectangle Result;
    
    Result.MinX = (A.MinX > B.MinX) ? A.MinX : B.MinX;
    Result.MinY = (A.MinY > B.MinY) ? A.MinY : B.MinY;
    Result.MaxX = (A.MaxX < B.MaxX) ? A.MaxX : B.MaxX;
    Result.MaxY = (A.MaxY < B.MaxY) ? A.MaxY : B.MaxY;
    
    return Result;
}

inline sv2Rectangle sv2RectangleUnion(sv2Rectangle A, sv2Rectangle B)
{
    sv2Rectangle Result;
    
    Result.MinX = (A.MinX < B.MinX) ? A.MinX : B.MinX;
    Result.MinY = (A.MinY < B.MinY) ? A.MinY : B.MinY;
    Result.MaxX = (A.MaxX > B.MaxX) ? A.MaxX : B.MaxX;
    Result.MaxY = (A.MaxY > B.MaxY) ? A.MaxY : B.MaxY;
    
    return Result;
}

inline s32 GetClampedRectArea(sv2Rectangle A)
{
    s32 Width = (A.MaxX - A.MinX);
    s32 Height = (A.MaxY - A.MinY);
    if (Width < 0 && Height < 0)
    {
        return 0;
    }
    s32 Result = Width * Height;
    return Result;
}

inline b32 HasArea(sv2Rectangle A)
{
    b32 Result = ((A.Min.x < A.Max.x) && (A.Min.y < A.Max.y));
    return Result;
}

#endif //MATH_H
