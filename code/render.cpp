

/* 
NOTE:

1) Everywhere outside the renderer, Y always goes upward, X to the right

2) All bitmaps including the render target are assumed to be bottom-up

3) Unless otherwise specified, all inputs to the renderer are in world space

4) All color values specified to the renderer are non-premultiplied alpha


COORDINATE SYSTEMS

World Space: non-isometric 3D coordinates relative to world
Isometric Space: 3D isometric coordinates relative to world
Projected Space: World space but the Z term is factored into the Y, making 2d coordinates
Screen Space: Projected Space relative to camera
Normalized Screen Space: screen space normalized as a percentage of screen dimensions

*/

// Coordinate System Transformations
// ----------------------------------------------

#define ISO_X_AXIS FV2(1.0f, 0.5f)
#define ISO_Y_AXIS FV2(-1.0f, 0.5f)
#define Z_TO_Y     1.0f


inline fv2 BasisToCanonical(fv2 P, fv2 XAxis, fv2 YAxis)
{
    fv2 ret = P.x*XAxis + P.y*YAxis;
    return ret;
}

inline fv2 CanonicalToBasis(fv2 P, fv2 XAxis, fv2 YAxis)
{
    f32 Det = XAxis.x*YAxis.y - XAxis.y*YAxis.x;
    Assert(Det != 0.0f);
    f32 InvDet = 1.0f/Det;
    
    fv2 InvXAxis = {InvDet*YAxis.y, -InvDet*XAxis.y};
    fv2 InvYAxis = {-InvDet*YAxis.x, InvDet*XAxis.x};
    
    fv2 ret = P.x*InvXAxis + P.y*InvYAxis;
    
    return ret;
}

inline fv2 
TransformPoint(fv2 P,
               fv2 SourceXAxis, fv2 SourceYAxis,
               fv2 DestXAxis, fv2 DestYAxis)
{
    fv2 CanonicalBasisP = BasisToCanonical(P, SourceXAxis, SourceYAxis);
    fv2 ret = CanonicalToBasis(CanonicalBasisP, DestXAxis, DestYAxis);
    return ret;
}

inline fv2
NormalizeScreenSpace(camera* Camera, fv2 ScreenSpace)
{
    Assert(!(Camera->Dim.x == 0.0f || Camera->Dim.y == 0.0f));
    
    fv2 ret = {
        ScreenSpace.x / Camera->Dim.x,
        ScreenSpace.y / Camera->Dim.y
    };
    
    return ret;
}

internal inline fv2 
UnNormalizeScreenSpace(camera* Camera, fv2 NormScreenSpace)
{
    return {
        NormScreenSpace.x * Camera->Dim.x,  
        NormScreenSpace.y * Camera->Dim.y
    };
}

inline fv2
WorldToProj(fv3 WorldPos)
{
    fv2 ret = WorldPos.xy;
    ret.y += Z_TO_Y * WorldPos.z;
    return ret;
}

internal fv3
ProjToWorld(fv2 Proj, f32 Z = 0)
{
    fv3 ret = fv2tofv3(Proj, Z);
    ret.y -= Z_TO_Y * ret.z;
    return ret;
}

inline fv3
WorldToIso(fv3 WorldPos)
{
    fv3 ret;
    ret.xy = CanonicalToBasis(WorldPos.xy, ISO_X_AXIS, ISO_Y_AXIS);
    ret.z = WorldPos.z;
    return ret;
}

inline fv3
IsoToWorld(fv3 IsoPos)
{
    fv3 ret;
    ret.xy = BasisToCanonical(IsoPos.xy, ISO_X_AXIS, ISO_Y_AXIS);
    ret.z = IsoPos.z;
    return ret;
}

inline fv2
ProjToNormScreen(camera* Camera, fv2 Proj)
{
    fv2 ret = NormalizeScreenSpace(Camera, (Proj - Camera->Pos));
    return ret;
}

inline fv2
NormScreenToProj(camera* Camera, fv2 NormScreen)
{
    fv2 ret = UnNormalizeScreenSpace(Camera, NormScreen) + Camera->Pos;
    return ret;
}

// TODO: move this to like a vector file? I don't want game code to be using functions from the render file
inline fv2
GetNormalizedProjDirection(fv3 Start, fv3 End)
{
    fv2 ret = fv2Normalize(WorldToProj(Start - End));
    return ret;
}

inline fv4
u32_to_fv4Color(u32 Color)
{
    f32 Inv255 = 1 / 255.0f;
    fv4 ret = {
        (real32)((Color >> 16) & 0xFF) * Inv255,
        (real32)((Color >> 8) & 0xFF) * Inv255,
        (real32)((Color >> 0) & 0xFF) * Inv255,
        (real32)((Color >> 24) & 0xFF) * Inv255
    };
    return ret;
}

inline u32
fv4_to_u32Color(fv4 Color)
{
    u32 ret = RoundReal32ToUInt32(255.0f * Color.r) << 16 |
        RoundReal32ToUInt32(255.0f * Color.g) << 8  |
        RoundReal32ToUInt32(255.0f * Color.b) << 0  |
        RoundReal32ToUInt32(255.0f * Color.a) << 24;
    return ret;
}

inline fv4
BiasNormal(fv4 Normal)
{
    fv4 Result = {
        -1.0f + (2.0f * Normal.x),
        -1.0f + (2.0f * Normal.y),
        -1.0f + (2.0f * Normal.z),
        Normal.w
    };
    return Result;
}

inline fv4
SRGBToLinear(fv4 C)
{
    fv4 Result;
    
    Result.r = Square(C.r);
    Result.g = Square(C.g);
    Result.b = Square(C.b);
    Result.a = C.a;
    
    return(Result);
}

inline fv4
LinearToSRGB(fv4 C)
{
    fv4 Result;
    
    Result.r = SquareRoot(C.r);
    Result.g = SquareRoot(C.g);
    Result.b = SquareRoot(C.b);
    Result.a = C.a;
    
    return(Result);
}

struct bilinear_sample
{
    u32 A, B, C, D;
};

inline bilinear_sample
BilinearSample(bitmap* Texture, int X, int Y)
{
    bilinear_sample Result;
    
    u8* TexelPtr =  (u8*)Texture->Memory + Y*Texture->Pitch + X*sizeof(u32);
    
    Result.A = *(u32*)(TexelPtr );
    Result.B = *(u32*)(TexelPtr + sizeof(u32));
    Result.C = *(u32*)(TexelPtr + Texture->Pitch);
    Result.D = *(u32*)(TexelPtr + Texture->Pitch + sizeof(u32));
    
    return Result;
}

inline fv4
SRGBBilinearBlend(bilinear_sample Sample, f32 fx, f32 fy)
{
    fv4 SampleA = SRGBToLinear(u32_to_fv4Color(Sample.A));
    fv4 SampleB = SRGBToLinear(u32_to_fv4Color(Sample.B));
    fv4 SampleC = SRGBToLinear(u32_to_fv4Color(Sample.C));
    fv4 SampleD = SRGBToLinear(u32_to_fv4Color(Sample.D));
    
    fv4 ret = fv4Lerp(fv4Lerp(SampleA, fx, SampleB),
                      fy,
                      fv4Lerp(SampleC, fx, SampleD));
    return ret;
}

inline sv2Rectangle
GetBitmapDimensions(bitmap* Bitmap)
{
    sv2Rectangle Result = {
        0, 0,
        Bitmap->Width, Bitmap->Height
    };
    return Result;
}

// TODO: remove?
inline void
FillPixel(u32* Pixel, u32 Color)
{
    if ((Color >> 24) >= (*Pixel >> 24))
    {
        *Pixel = Color;
    }
}

// NOTE: dimension is in pixels
// TODO: switch to normalized coordinates?
internal void
DrawRectangle(bitmap* Buffer, sv2Rectangle Dimension, fv4 Color, sv2Rectangle ClipRect, b32 Even)
{
    u32 Color32 = fv4_to_u32Color(Color);
    
    sv2Rectangle FillRect = sv2RectangleIntersection(Dimension, ClipRect);
    if (!Even == (FillRect.MinY&1))
    {
        FillRect.MinY++;
    }
    
    u8* Row = (u8*)Buffer->Memory +
        FillRect.MinY * Buffer->Pitch +
        FillRect.MinX * sizeof(u32);
    for (int Y = FillRect.MinY;
         Y < FillRect.MaxY;
         Y+=2)
    {
        uint32* Pixel = (uint32*)Row;
        for (int X = FillRect.MinX;
             X < FillRect.MaxX;
             ++X)
        {
            *Pixel++ = Color32;
        }
        Row += 2*Buffer->Pitch;
    }
}

// take normalized coordinates and draws line between points
internal void
DEBUGDrawLine(bitmap* Buffer,
              fv2 start, fv2 end, 
              u32 Color)
{
    // clip endpoints to fit inside camera frame
    
    fv2 diff_vector = end - start;
    
    // TODO: handle case where diff_vector is zero
    // find t values for frame boundary intersections
    f32 left_intersection = -start.x / diff_vector.x;
    f32 right_intersection = (1.0f - start.x) / diff_vector.x;
    f32 top_intersection = -start.y / diff_vector.y;
    f32 bottom_intersection = (1.0f - start.y) / diff_vector.y;
    
    fv2 top = start + top_intersection * diff_vector;
    if (top.y > start.y)
    {
        start = top;
    }
    if (top.y > end.y)
    {
        end = top;
    }
    
    fv2 bottom = start + bottom_intersection * diff_vector;
    if (bottom.y < start.y)
    {
        start = bottom;
    }
    if (bottom.y < end.y)
    {
        end = bottom;
    }
    
    fv2 left = start + left_intersection * diff_vector;
    if (left.x > start.x)
    {
        start = left;
    }
    if (left.x > end.x)
    {
        end = left;
    }
    
    fv2 right = start + right_intersection * diff_vector;
    if (right.x < start.x)
    {
        start = right;
    }
    if (right.x < end.x)
    {
        end = right;
    }
    
    int StartX = RoundFloatToInt(start.x * (f32)Buffer->Width);
    int StartY = RoundFloatToInt(start.y * (f32)Buffer->Height);
    int EndX = RoundFloatToInt(end.x * (f32)Buffer->Width);
    int EndY = RoundFloatToInt(end.y * (f32)Buffer->Height);
    
    s32 dy = EndY - StartY;
    s32 dx = EndX - StartX;
    
    StartX = s32Clamp(0, Buffer->Width, StartX);
    EndX = s32Clamp(0, Buffer->Width, EndX);
    StartY = s32Clamp(0, Buffer->Height, StartY);
    EndY = s32Clamp(0, Buffer->Height, EndY);
    
    int BytesPerPixel = BITMAP_BYTES_PER_PIXEL;
    if (dx == 0)
    {
        if (StartY > EndY)
        {
            int temp = StartY;
            StartY = EndY;
            EndY = temp;
        }
        
        u8* Row = (u8*)Buffer->Memory + StartX * BytesPerPixel;
        for (s32 Y = StartY;
             Y < EndY;
             ++Y)
        {
            u32* Pixel = (u32*)(Row + Y * Buffer->Pitch);
            FillPixel(Pixel, Color);
        }
    }
    else
    {
        f32 slope = (f32)dy/(f32)dx;
        int y_intercept = StartY - (int)(slope * (f32)StartX);
        if (slope <= 1.0f && slope >= -1)
        {
            if (EndX < StartX)
            {
                int temp = StartX;
                StartX = EndX;
                EndX = temp;
            }
            for (s32 X = StartX;
                 X < EndX;
                 ++X)
            {
                int Y = (int)(slope * X) + y_intercept;
                Y = s32Clamp(0, Buffer->Height - 1, Y);
                u32* Pixel = (u32*)((u8*)Buffer->Memory + X * BytesPerPixel + Y * Buffer->Pitch);
                FillPixel(Pixel, Color);
            }
        }
        else
        {
            if (EndY < StartY)
            {
                int temp = StartY;
                StartY = EndY;
                EndY = temp;
            }
            for (s32 Y = StartY;
                 Y < EndY;
                 ++Y)
            {
                int X = (int)((f32)(Y - y_intercept) / slope);
                X = s32Clamp(0, Buffer->Width - 1, X);
                u32* Pixel = (u32*)((u8*)Buffer->Memory + X * BytesPerPixel + Y * Buffer->Pitch);
                FillPixel(Pixel, Color);
            }
            
        }
    }
}

inline fv2
fv2FindMin(fv2* Array, int Count)
{
    fv2 Min = {Array[0].x, Array[0].y};
    
    for (int Index = 1;
         Index < Count;
         Index++)
    {
        if (Array[Index].x < Min.x) {Min.x = Array[Index].x;}
        if (Array[Index].y < Min.y) {Min.y = Array[Index].y;}
    }
    
    return Min;
}

inline fv2
fv2FindMax(fv2* Array, int Count)
{
    fv2 Max = {Array[0].x, Array[0].y};
    
    for (int Index = 1;
         Index < Count;
         Index++)
    {
        if (Array[Index].x > Max.x) {Max.x = Array[Index].x;}
        if (Array[Index].y > Max.y) {Max.y = Array[Index].y;}
    }
    
    return Max;
}


void
DrawGridlines(bitmap* Buffer, render_group* RenderGroup)
{
    camera* Camera = RenderGroup->Camera;
    
    fv2 Corners[4] = {
        Camera->Pos,
        {Camera->Pos.x + Camera->Dim.x, Camera->Pos.y                },
        {Camera->Pos.x                , Camera->Pos.y + Camera->Dim.y},
        Camera->Pos + Camera->Dim
    };
    
    for (int CornerIndex = 0;
         CornerIndex < 4;
         ++CornerIndex)
    {
        Corners[CornerIndex] = WorldToIso(fv2tofv3(Corners[CornerIndex], 0)).xy;
    }
    
    fv2 Min = fv2FindMin(Corners, 4);
    fv2 Max = fv2FindMax(Corners, 4);
    
    int MinX = s32Floor(Min.x);
    int MinY = s32Floor(Min.y);
    int MaxX = s32Ceiling(Max.x);
    int MaxY = s32Ceiling(Max.y);
    
    f32 OrthXSlope = ISO_Y_AXIS.y / ISO_Y_AXIS.x;
    f32 OrthYSlope = ISO_X_AXIS.y / ISO_X_AXIS.x;
    
    fv2 Origin = Camera->Pos;
    fv2 FarCorner = Camera->Pos + Camera->Dim;
    s32 Sign = FarCorner.x > Origin.x ? 1 : -1;
    
    for (int XIndex = MinX; 
         XIndex < MaxX + 1;
         XIndex++)
    {
        fv2 start = {
            Origin.x, 
            Origin.x * OrthXSlope + (f32)(Sign * XIndex)
        };
        fv2 end = {
            FarCorner.x, 
            FarCorner.x * OrthXSlope + (f32)(Sign * XIndex)
        };
        start = NormalizeScreenSpace(Camera, start - Origin);
        end = NormalizeScreenSpace(Camera, end - Origin);
        DEBUGDrawLine(Buffer, start, end, 0x02AAAAAA);
    }
    for (int YIndex = MinY; 
         YIndex < MaxY + 1;
         YIndex++)
    {
        fv2 start = {
            Origin.x, 
            Origin.x * OrthYSlope + (f32)(Sign * YIndex)
        };
        fv2 end = {
            FarCorner.x, 
            FarCorner.x * OrthYSlope + (f32)(Sign * YIndex)
        };
        start = NormalizeScreenSpace(Camera, start - Origin);
        end = NormalizeScreenSpace(Camera, end - Origin);
        DEBUGDrawLine(Buffer, start, end, 0x02AAAAAA);
    }
}

// NOTE: base and height are in screen space
inline fv3
SampleEnvironmentMap(camera* Camera, fv2 Base, real32 Height, fv3 SampleDirection, f32 Roughness, environment_map* Map)
{
    u32 LODIndex = (u32)(Roughness*(real32)(ArrayCount(Map->LOD)-1) + 0.5f);
    Assert(LODIndex < ArrayCount(Map->LOD));
    
    bitmap* LOD = &Map->LOD[LODIndex];
    
    f32 tX = 0;
    f32 tY = 0;
    
    if (Height > 0)
    {
        real32 C = -Height / SampleDirection.z;
        fv2 Offset = C * FV2(SampleDirection.x, SampleDirection.y);
        fv2 SampleIntersection = Base + Offset;
        SampleIntersection = NormalizeScreenSpace(Camera, SampleIntersection);
        
        SampleIntersection.x = f32Clamp01(SampleIntersection.x);
        SampleIntersection.y = f32Clamp01(SampleIntersection.y);
        
        tX = (real32)(LOD->Width) * SampleIntersection.x;
        tY = (real32)(LOD->Height) * SampleIntersection.y;
        
    }
    else
    {
        tX = (real32)LOD->Width * 0.5f * (SampleDirection.x + 1.0f);
        tY = (real32)LOD->Height * 0.5f * (SampleDirection.y + 1.0f);
    }
    
    int X = s32Floor(tX);
    int Y = s32Floor(tY);
    
    f32 fx = tX - (f32)X;
    f32 fy = tY - (f32)Y;
    
#if 0
    u8* LODPtr = ((u8*)LOD->Memory) + Y*LOD->Pitch + X *sizeof(u32);
    *(u32*)LODPtr = 0xFFFFFFFF;
#endif
    
    bilinear_sample Sample = BilinearSample(LOD, X, Y);
    fv3 Result = SRGBBilinearBlend(Sample, fx, fy).xyz;
    
    return Result;
}

inline b32 
InsideEdge(fv2 V1, fv2 V2, fv2 P) 
{ 
    return ((P.x - V1.x) * (V2.y - V1.y) - (P.y - V1.y) * (V2.x - V1.x) + EPSILON >= 0); 
}

struct counts{
    int mm_add_ps;
    int mm_sub_ps;
    int mm_mul_ps;
    int mm_castps_si128;
    int mm_and_ps;
    int mm_and_si128;
    int mm_or_si128;
    int mm_andnot_si128;
    int mm_cmpge_ps;
    int mm_cmple_ps;
    int mm_min_ps;
    int mm_max_ps;
    int mm_cvttps_epi32;
    int mm_cvtps_epi32;
    int mm_cvtepi32_ps;
    int mm_srli_epi32;
    int mm_slli_epi32;
    int mm_sqrt_ps;
};


// TODO: this function takes projected space but the cliprect is in pixels
// TODO: maybe this should just take pixels?
internal void
QuickDrawQuad(bitmap* Buffer, camera* Camera,
              fv2 Origin, fv2 XAxis, fv2 YAxis,
              fv4 Color, bitmap* Texture,
              sv2Rectangle ClipRect, b32 Even)
{
    
    Color.rgb *= Color.a;
    
    /*
    f32 XAxisLength = fv2Length(XAxis);
    f32 YAxisLength = fv2Length(YAxis);
    fv2 NormalTexXAxis = (YAxisLength / XAxisLength) * XAxis;
    fv2 NormalTexYAxis = (XAxisLength / YAxisLength ) * YAxis;
    // NOTE: NzScale could be a parameter that we pass in if we want to change how we want the depth to appear to scale
    f32 NzScale = 0.5f * (XAxisLength + YAxisLength);
    */
    
    // BoundingRect
    fv2 Corners[4] = {
        Origin, 
        Origin + XAxis, 
        Origin + XAxis + YAxis, 
        Origin + YAxis
    };
    
    for (int CornerIndex = 0;
         CornerIndex < 4;
         ++CornerIndex)
    {
        Corners[CornerIndex] = ProjToNormScreen(Camera, Corners[CornerIndex]);
        Corners[CornerIndex].x = Corners[CornerIndex].x * (f32)Buffer->Width;
        Corners[CornerIndex].y = Corners[CornerIndex].y * (f32)Buffer->Height;
    }
    
    fv2 Min = fv2FindMin(Corners, 4);
    fv2 Max = fv2FindMax(Corners, 4);
    
    sv2Rectangle BoundingRect = {
        s32Floor(Min.x),
        s32Floor(Min.y),
        s32Ceiling(Max.x) + 1,
        s32Ceiling(Max.y) + 1
    };
    
    //sv2Rectangle ClipRect = {128,128,WidthMax,HeightMax};
    BoundingRect = sv2RectangleIntersection(BoundingRect, ClipRect);
    
    if (!Even == (BoundingRect.MinY&1))
    {
        BoundingRect.MinY++;
    }
    
    if (HasArea(BoundingRect))
    {
        __m128i StartClipMask = _mm_set1_epi8(-1);
        __m128i EndClipMask = _mm_set1_epi8(-1);
        
        __m128i StartClipMasks[] = {
            _mm_slli_si128(StartClipMask, 0 * 4),
            _mm_slli_si128(StartClipMask, 1 * 4),
            _mm_slli_si128(StartClipMask, 2 * 4),
            _mm_slli_si128(StartClipMask, 3 * 4)
        };
        
        __m128i EndClipMasks[] = {
            _mm_srli_si128(EndClipMask, 0 * 4),
            _mm_srli_si128(EndClipMask, 3 * 4),
            _mm_srli_si128(EndClipMask, 2 * 4),
            _mm_srli_si128(EndClipMask, 1 * 4)
        };
        
        if (BoundingRect.MinX & 3)
        {
            StartClipMask = StartClipMasks[BoundingRect.MinX & 3];
            BoundingRect.MinX = BoundingRect.MinX & ~3;
        }
        if (BoundingRect.MaxX & 3)
        {
            EndClipMask = EndClipMasks[BoundingRect.MaxX & 3];
            BoundingRect.MaxX = (BoundingRect.MaxX & ~3) + 4;
        }
        
        s32 MinX = BoundingRect.MinX;
        s32 MinY = BoundingRect.MinY;
        s32 MaxX = BoundingRect.MaxX;
        s32 MaxY = BoundingRect.MaxY;
        
        fv2 PixelXAxis = Corners[1] - Corners[0];
        fv2 PixelYAxis = Corners[3] - Corners[0];
        
        f32 Det = PixelXAxis.x*PixelYAxis.y - PixelXAxis.y*PixelYAxis.x;
        Assert(Det != 0.0f);
        f32 InvDet = 1.0f/Det;
        
        fv2 InvXAxis = {InvDet*PixelYAxis.y, -InvDet*PixelXAxis.y};
        fv2 InvYAxis = {-InvDet*PixelYAxis.x, InvDet*PixelXAxis.x};
        
#if 1
        if (Texture == 0)
        {
            return;
        }
#endif
        s32 TexturePitch = Texture->Pitch;
        void* TextureMemory = Texture->Memory;
        
        __m128i TexturePitch_4x = _mm_set1_epi32(TexturePitch);
        __m128 Inv255_4x = _mm_set1_ps(1/255.0f);
        __m128 One255_4x = _mm_set1_ps(255.0f);
        
        __m128 One = _mm_set1_ps(1.0f);
        __m128 Zero = _mm_set1_ps(0.0f);
        __m128 Half = _mm_set1_ps(0.5f);
        __m128 Four_4x = _mm_set1_ps(4.0f);
        __m128i MaskFF = _mm_set1_epi32(0xFF);
        __m128 Colorr_4x = _mm_set1_ps(Color.r);
        __m128 Colorg_4x = _mm_set1_ps(Color.g);
        __m128 Colorb_4x = _mm_set1_ps(Color.b);
        __m128 Colora_4x = _mm_set1_ps(Color.a);
        __m128 InvXAxisX_4x = _mm_set1_ps(InvXAxis.x);
        __m128 InvXAxisY_4x = _mm_set1_ps(InvXAxis.y);
        __m128 InvYAxisX_4x = _mm_set1_ps(InvYAxis.x);
        __m128 InvYAxisY_4x = _mm_set1_ps(InvYAxis.y);
        __m128 Originx_4x = _mm_set1_ps(Corners[0].x);
        __m128 Originy_4x = _mm_set1_ps(Corners[0].y);
        
        __m128 WidthM2 = _mm_set1_ps((real32)(Texture->Width - 2));
        __m128 HeightM2 = _mm_set1_ps((real32)(Texture->Height - 2));
        
        u8* DestRow = ((u8*)Buffer->Memory +
                       MinX * BITMAP_BYTES_PER_PIXEL + 
                       MinY * Buffer->Pitch);
        s32 RowAdvance = 2*Buffer->Pitch;
        
        BEGIN_TIMED_BLOCK(ProcessPixel);
        
        for (s32 Y = MinY;
             Y < MaxY;
             Y+=2)
        {
            __m128 PixelPx = _mm_set_ps((real32)(MinX + 3) + 0.5f,
                                        (real32)(MinX + 2) + 0.5f,
                                        (real32)(MinX + 1) + 0.5f,
                                        (real32)(MinX + 0) + 0.5f);
            __m128 PixelPy = _mm_set1_ps((real32)Y + 0.5f);
            PixelPx = _mm_sub_ps(PixelPx, Originx_4x);
            PixelPy = _mm_sub_ps(PixelPy, Originy_4x);
            
            __m128i ClipMask = StartClipMask;
            
            u32* Pixel = (u32*)DestRow;
            for (s32 X = MinX;
                 X < MaxX;
                 X += 4)
            {
                
#define mmSquare(a) _mm_mul_ps(a, a)    
#define M(a, i) ((float *)&(a))[i]
#define Mi(a, i) ((uint32 *)&(a))[i]
                
                // TODO: use IACA or LLVM-MCA if want to optimize further
#define COUNT_CYCLES 0
                
#if COUNT_CYCLES
                counts Counts = {};
#define _mm_add_ps(a,b) ++Counts.mm_add_ps; a; b
#define _mm_sub_ps(a,b) ++Counts.mm_sub_ps; a; b
#define _mm_mul_ps(a,b) ++Counts.mm_mul_ps; a; b
#define _mm_castps_si128(a) ++Counts.mm_castps_si128; a
#define _mm_and_ps(a,b) ++Counts.mm_and_ps; a; b
#define _mm_and_si128(a,b) ++Counts.mm_and_si128; a; b
#define _mm_or_si128(a,b) ++Counts.mm_or_si128; a; b
#define _mm_andnot_si128(a,b) ++Counts.mm_andnot_si128; a; b
#define _mm_cmpge_ps(a,b) ++Counts.mm_cmpge_ps; a; b
#define _mm_cmple_ps(a,b) ++Counts.mm_cmple_ps; a; b
#define _mm_min_ps(a,b) ++Counts.mm_min_ps; a; b
#define _mm_max_ps(a,b) ++Counts.mm_max_ps; a; b
#define _mm_cvttps_epi32(a) ++Counts.mm_cvttps_epi32; a
#define _mm_cvtps_epi32(a) ++Counts.mm_cvtps_epi32; a
#define _mm_cvtepi32_ps(a) ++Counts.mm_cvtepi32_ps; a
#define _mm_srli_epi32(a,b) ++Counts.mm_srli_epi32; a
#define _mm_slli_epi32(a,b) ++Counts.mm_slli_epi32; a
#define _mm_sqrt_ps(a) ++Counts.mm_sqrt_ps; a
#undef mmSquare
#define mmSquare(a) ++Counts.mm_mul_ps; a
                
#define __m128 int
#define __m128i int
#define _mm_loadu_si128(a) 0
#define _mm_storeu_si128(a, b)
#endif
                
                
                __m128 TextureSpaceX = _mm_add_ps(_mm_mul_ps(PixelPx, InvXAxisX_4x), _mm_mul_ps(PixelPy, InvYAxisX_4x));
                __m128 TextureSpaceY = _mm_add_ps(_mm_mul_ps(PixelPx, InvXAxisY_4x), _mm_mul_ps(PixelPy, InvYAxisY_4x));
                
                __m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(TextureSpaceX, Zero),
                                                                           _mm_cmple_ps(TextureSpaceX, One)),
                                                                _mm_and_ps(_mm_cmpge_ps(TextureSpaceY, Zero),
                                                                           _mm_cmple_ps(TextureSpaceY, One))));
                WriteMask = _mm_and_si128(WriteMask, ClipMask);
                
                // TODO(casey): Later, re-check if this helps
                // if(_mm_movemask_epi8(WriteMask))
                {
                    __m128i OriginalDest = _mm_load_si128((__m128i*)Pixel);
                    
                    TextureSpaceX = _mm_min_ps(_mm_max_ps(TextureSpaceX, Zero), One);
                    TextureSpaceY = _mm_min_ps(_mm_max_ps(TextureSpaceY, Zero), One);
                    
                    // NOTE: bias texel coordinates by +1 texels, -0.5 texels
                    __m128 tX = _mm_add_ps(_mm_mul_ps(TextureSpaceX, WidthM2), One);
                    __m128 tY = _mm_add_ps(_mm_mul_ps(TextureSpaceY, HeightM2), One);
                    
                    __m128i TexelX_4x = _mm_cvttps_epi32(tX);
                    __m128i TexelY_4x = _mm_cvttps_epi32(tY);
                    
                    __m128 fx = _mm_sub_ps(tX, _mm_cvtepi32_ps(TexelX_4x));
                    __m128 fy = _mm_sub_ps(tY, _mm_cvtepi32_ps(TexelY_4x));
                    
                    TexelX_4x = _mm_slli_epi32(TexelX_4x, 2);
                    TexelY_4x = _mm_or_si128(_mm_mullo_epi16(TexelY_4x, TexturePitch_4x),
                                             _mm_slli_epi32(_mm_mulhi_epi16(TexelY_4x, TexturePitch_4x), 16));
                    __m128i Texel_4x = _mm_add_epi32(TexelX_4x, TexelY_4x);
                    
                    s32 Texel0 = Mi(Texel_4x, 0);
                    s32 Texel1 = Mi(Texel_4x, 1);
                    s32 Texel2 = Mi(Texel_4x, 2);
                    s32 Texel3 = Mi(Texel_4x, 3);
                    
                    u8* TexelPtr0 = (u8*)TextureMemory + Texel0;
                    u8* TexelPtr1 = (u8*)TextureMemory + Texel1;
                    u8* TexelPtr2 = (u8*)TextureMemory + Texel2;
                    u8* TexelPtr3 = (u8*)TextureMemory + Texel3;
                    
                    __m128i SampleA = _mm_setr_epi32(*(u32*)(TexelPtr0),
                                                     *(u32*)(TexelPtr1),
                                                     *(u32*)(TexelPtr2),
                                                     *(u32*)(TexelPtr3));
                    
                    __m128i SampleB = _mm_setr_epi32(*(u32*)(TexelPtr0 + sizeof(u32)),
                                                     *(u32*)(TexelPtr1 + sizeof(u32)),
                                                     *(u32*)(TexelPtr2 + sizeof(u32)),
                                                     *(u32*)(TexelPtr3 + sizeof(u32)));
                    
                    __m128i SampleC = _mm_setr_epi32(*(u32*)(TexelPtr0 + TexturePitch),
                                                     *(u32*)(TexelPtr1 + TexturePitch),
                                                     *(u32*)(TexelPtr2 + TexturePitch),
                                                     *(u32*)(TexelPtr3 + TexturePitch));
                    
                    __m128i SampleD = _mm_setr_epi32(*(u32*)(TexelPtr0 + TexturePitch + sizeof(u32)),
                                                     *(u32*)(TexelPtr1 + TexturePitch + sizeof(u32)),
                                                     *(u32*)(TexelPtr2 + TexturePitch + sizeof(u32)),
                                                     *(u32*)(TexelPtr3 + TexturePitch + sizeof(u32)));
                    
                    // NOTE: Unpack Sample Texels
                    __m128 TexelAb = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(SampleA, MaskFF)));
                    __m128 TexelAg = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 8), MaskFF)));
                    __m128 TexelAr = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 16), MaskFF)));
                    __m128 TexelAa = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 24), MaskFF)));
                    
                    __m128 TexelBb = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(SampleB, MaskFF)));
                    __m128 TexelBg = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 8), MaskFF)));
                    __m128 TexelBr = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 16), MaskFF)));
                    __m128 TexelBa = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 24), MaskFF)));
                    
                    __m128 TexelCb = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(SampleC, MaskFF)));
                    __m128 TexelCg = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleC, 8), MaskFF)));
                    __m128 TexelCr = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleC, 16), MaskFF)));
                    __m128 TexelCa = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleC, 24), MaskFF)));
                    
                    __m128 TexelDb = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(SampleD, MaskFF)));
                    __m128 TexelDg = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleD, 8), MaskFF)));
                    __m128 TexelDr = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleD, 16), MaskFF)));
                    __m128 TexelDa = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleD, 24), MaskFF)));
                    
                    // NOTE(casey): Load destination
                    __m128 Destb = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(OriginalDest, MaskFF)));
                    __m128 Destg = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 8), MaskFF)));
                    __m128 Destr = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 16), MaskFF)));
                    __m128 Desta = _mm_mul_ps(Inv255_4x, _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 24), MaskFF)));
                    
                    // NOTE: Convert from SRGB to Linear Space
                    TexelAr = mmSquare(TexelAr);
                    TexelAg = mmSquare(TexelAg);
                    TexelAb = mmSquare(TexelAb);
                    
                    TexelBr = mmSquare(TexelBr);
                    TexelBg = mmSquare(TexelBg);
                    TexelBb = mmSquare(TexelBb);
                    
                    TexelCr = mmSquare(TexelCr);
                    TexelCg = mmSquare(TexelCg);
                    TexelCb = mmSquare(TexelCb);
                    
                    TexelDr = mmSquare(TexelDr);
                    TexelDg = mmSquare(TexelDg);
                    TexelDb = mmSquare(TexelDb);
                    
                    Destr = mmSquare(Destr);
                    Destg = mmSquare(Destg);
                    Destb = mmSquare(Destb);
                    
                    // NOTE: bilinear texture blend
                    __m128 ifx = _mm_sub_ps(One, fx);
                    __m128 ify = _mm_sub_ps(One, fy);
                    
                    __m128 l0 = _mm_mul_ps(ify, ifx);
                    __m128 l1 = _mm_mul_ps(ify, fx);
                    __m128 l2 = _mm_mul_ps(fy, ifx);
                    __m128 l3 = _mm_mul_ps(fy, fx);
                    
                    __m128 Texelr = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0,TexelAr), _mm_mul_ps(l1, TexelBr)), 
                                               _mm_add_ps(_mm_mul_ps(l2,TexelCr), _mm_mul_ps(l3, TexelDr)));
                    __m128 Texelg = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0,TexelAg), _mm_mul_ps(l1, TexelBg)), 
                                               _mm_add_ps(_mm_mul_ps(l2,TexelCg), _mm_mul_ps(l3, TexelDg)));
                    __m128 Texelb = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0,TexelAb), _mm_mul_ps(l1, TexelBb)), 
                                               _mm_add_ps(_mm_mul_ps(l2,TexelCb), _mm_mul_ps(l3, TexelDb)));
                    __m128 Texela = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0,TexelAa), _mm_mul_ps(l1, TexelBa)), 
                                               _mm_add_ps(_mm_mul_ps(l2,TexelCa), _mm_mul_ps(l3, TexelDa)));
                    
                    // NOTE: modulate by incoming color
                    Texelr = _mm_mul_ps(Texelr, Colorr_4x);
                    Texelg = _mm_mul_ps(Texelg, Colorg_4x);
                    Texelb = _mm_mul_ps(Texelb, Colorb_4x);
                    Texela = _mm_mul_ps(Texela, Colora_4x);
                    
                    // NOTE: clamp colors to valid range
                    Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero), One);
                    Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero), One);
                    Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero), One);
                    
                    // NOTE: Blend Texel with destination
                    __m128 InvTexelA = _mm_sub_ps(One, Texela);
                    __m128 Blendedr = _mm_add_ps(_mm_mul_ps(InvTexelA, Destr), Texelr);
                    __m128 Blendedg = _mm_add_ps(_mm_mul_ps(InvTexelA, Destg), Texelg);
                    __m128 Blendedb = _mm_add_ps(_mm_mul_ps(InvTexelA, Destb), Texelb);
                    __m128 Blendeda = _mm_add_ps(_mm_mul_ps(InvTexelA, Desta), Texela);
                    
                    // NOTE: Convert to SRGB
                    Blendedr = _mm_sqrt_ps(Blendedr);
                    Blendedg = _mm_sqrt_ps(Blendedg);
                    Blendedb = _mm_sqrt_ps(Blendedb);
                    
                    // NOTE: Repack
                    __m128i Intr = _mm_cvtps_epi32(_mm_mul_ps(One255_4x, Blendedr));
                    __m128i Intg = _mm_cvtps_epi32(_mm_mul_ps(One255_4x, Blendedg));
                    __m128i Intb = _mm_cvtps_epi32(_mm_mul_ps(One255_4x, Blendedb));
                    __m128i Inta = _mm_cvtps_epi32(_mm_mul_ps(One255_4x, Blendeda));
                    
                    __m128i Sr = _mm_slli_epi32(Intr, 16);
                    __m128i Sg = _mm_slli_epi32(Intg, 8);
                    __m128i Sb = Intb;
                    __m128i Sa = _mm_slli_epi32(Inta, 24);
                    
                    __m128i Out = _mm_or_si128(_mm_or_si128(Sr, Sg), _mm_or_si128(Sb, Sa));
                    
                    __m128i MaskedOut = _mm_or_si128(_mm_and_si128(WriteMask, Out),
                                                     _mm_andnot_si128(WriteMask, OriginalDest));
                    _mm_store_si128((__m128i*)Pixel, MaskedOut);
                }
                
#if COUNT_CYCLES
#undef _mm_add_ps
#endif
                
                PixelPx = _mm_add_ps(PixelPx, Four_4x);
                Pixel += 4;
                
                if ((X + 8) < MaxX)
                {
                    ClipMask = _mm_set1_epi8(-1);
                }
                else
                {
                    ClipMask = EndClipMask;
                }
            }
            DestRow += RowAdvance;
        }
        END_TIMED_BLOCK_COUNTED(ProcessPixel, GetClampedRectArea(BoundingRect)/2);
    }
}

internal void
DrawQuad(bitmap* Buffer, camera* Camera,
         fv2 Origin, fv2 XAxis, fv2 YAxis,
         fv4 Color, bitmap* Texture, 
         bitmap* NormalMap, fv3 SurfaceNormal, fv3 SurfaceUp,
         environment_map* Top,
         environment_map* Middle,
         environment_map* Bottom)
{
    Color.rgb *= Color.a;
    
    f32 XAxisLength = fv2Length(XAxis);
    f32 YAxisLength = fv2Length(YAxis);
    fv2 NormalTexXAxis = (YAxisLength / XAxisLength) * XAxis;
    fv2 NormalTexYAxis = (XAxisLength / YAxisLength ) * YAxis;
    // NOTE: NzScale could be a parameter that we pass in if we want to change how we want the depth to appear to scale
    f32 NzScale = 0.5f * (XAxisLength + YAxisLength);
    
    int WidthMax = Buffer->Width - 1;
    int HeightMax = Buffer->Height - 1;
    
    // BoundingRect
    fv2 Corners[4] = {
        Origin, 
        Origin + XAxis, 
        Origin + XAxis + YAxis, 
        Origin + YAxis
    };
    
    for (int CornerIndex = 0;
         CornerIndex < 4;
         ++CornerIndex)
    {
        Corners[CornerIndex] = ProjToNormScreen(Camera, Corners[CornerIndex]);
        Corners[CornerIndex].x = Corners[CornerIndex].x * (f32)WidthMax;
        Corners[CornerIndex].y = Corners[CornerIndex].y * (f32)HeightMax;
    }
    
    fv2 Min = fv2FindMin(Corners, 4);
    fv2 Max = fv2FindMax(Corners, 4);
    
    Min.x = f32Clamp(0, (f32)WidthMax,  Min.x);
    Min.y = f32Clamp(0, (f32)HeightMax, Min.y);
    Max.x = f32Clamp(0, (f32)WidthMax,  Max.x);
    Max.y = f32Clamp(0, (f32)HeightMax, Max.y);
    
    int MinX = s32Floor(Min.x);
    int MinY = s32Floor(Min.y);
    int MaxX = s32Ceiling(Max.x);
    int MaxY = s32Ceiling(Max.y);
    
    f32 InvXAxisRSquared = 1 / fv2RSquared(Corners[1] - Corners[0]);
    f32 InvYAxisRSquared = 1 / fv2RSquared(Corners[3] - Corners[0]);
    
    u8* DestRow = ((u8*)Buffer->Memory +
                   MinX * BITMAP_BYTES_PER_PIXEL + 
                   MinY * Buffer->Pitch);
    for (s32 Y = MinY;
         Y <= MaxY;
         ++Y)
    {
        u32* Pixel = (u32*)DestRow;
        for (s32 X = MinX;
             X <= MaxX;
             ++X)
        {
            fv2 PixelCenter = {
                (f32)X + 0.5f,
                (f32)Y + 0.5f
            };
            
            if (InsideEdge(Corners[1], Corners[0], PixelCenter) &&
                InsideEdge(Corners[2], Corners[1], PixelCenter) &&
                InsideEdge(Corners[3], Corners[2], PixelCenter) &&
                InsideEdge(Corners[0], Corners[3], PixelCenter))
            {
                if (Texture)
                {
                    fv2 P = PixelCenter - Corners[0];
                    fv2 TextureSpaceP = CanonicalToBasis(P, 
                                                         Corners[1] - Corners[0],
                                                         Corners[3] - Corners[0]);
                    
#if 0
                    // TODO(casey): SSE clamping.
                    Assert((TextureSpaceP.x >= 0.0f) && (TextureSpaceP.x <= 1.0f));
                    Assert((TextureSpaceP.y >= 0.0f) && (TextureSpaceP.y <= 1.0f));
#endif
                    
                    
                    real32 tX = TextureSpaceP.x * (real32)(Texture->Width - 2);
                    real32 tY = TextureSpaceP.y * (real32)(Texture->Height - 2);
                    
                    int TexelX = s32Floor(tX);
                    int TexelY = s32Floor(tY);
                    
                    f32 fx = tX - (f32)TexelX;
                    f32 fy = tY - (f32)TexelY;
                    
                    bilinear_sample TexelSample = BilinearSample(Texture, TexelX, TexelY);
                    fv4 Texel = SRGBBilinearBlend(TexelSample, fx, fy);
                    
                    if (NormalMap)
                    {
                        bilinear_sample NormalSample = BilinearSample(NormalMap, TexelX, TexelY);
                        
                        fv4 NormalA = u32_to_fv4Color(NormalSample.A);
                        fv4 NormalB = u32_to_fv4Color(NormalSample.B);
                        fv4 NormalC = u32_to_fv4Color(NormalSample.C);
                        fv4 NormalD = u32_to_fv4Color(NormalSample.D);
                        
                        fv4 Normal = fv4Lerp(fv4Lerp(NormalA, fx, NormalB),
                                             fy,
                                             fv4Lerp(NormalC, fx, NormalD));
                        
                        // TODO: what coordinate system are the normals in???
                        // I'm assuming proj for now
                        
                        Normal = BiasNormal(Normal);
                        
                        // rotate according to texture axes
                        Normal.xy = Normal.x * NormalTexXAxis + Normal.y * NormalTexYAxis;
                        //Normal.xy = SquareRoot(1 - Square(Normal.z)) * fv2Normalize(Normal.xy);
                        Normal.z *= NzScale;
                        Normal.xyz = fv3Normalize(Normal.xyz);
                        
                        // orient the normal with respect to the surface normal
                        // Y Axis = Surface Up
                        // Z Axis = Surface Normal
                        fv3 NormalXAxis = fv3Cross(SurfaceNormal, SurfaceUp);
                        
                        fv3 RotatedNormal = Normal.x * NormalXAxis + Normal.y * SurfaceUp + Normal.z * SurfaceNormal;
                        Normal.xyz = fv3Normalize(RotatedNormal);
                        
                        
                        fv3 EyeVector = fv3Normalize(FV3(0, 1, 1)); // Toward camera
                        fv3 BounceDirection = 2.0f * fv3Dot(EyeVector, Normal.xyz) * Normal.xyz - EyeVector;
                        BounceDirection = fv3Normalize(BounceDirection);
                        
                        fv2 TexelNormScreen = {
                            PixelCenter.x / (f32)Buffer->Width, 
                            PixelCenter.y / (f32)Buffer->Height
                        };
                        fv2 TexelScreen = UnNormalizeScreenSpace(Camera, TexelNormScreen);
                        real32 TexelHeight = SurfaceUp.z * (TextureSpaceP.x*XAxis.y + TextureSpaceP.y*YAxis.y);
                        fv2 TexelGroundPosition = TexelScreen - FV2(0, TexelHeight);
                        
                        environment_map* FarMap = 0;
                        f32 tEnvMap =  BounceDirection.z;
                        f32 tFarMap = 0.0f;
                        if (tEnvMap < -0.5f)
                        {
                            FarMap = Bottom;
                            tFarMap = -1.0f - 2.0f*tEnvMap;
                        }
                        else if (tEnvMap > 0.5f)
                        {
                            FarMap = Top;
                            tFarMap = 2.0f * (tEnvMap - 0.5f);
                            TexelHeight = -1; 
                        }
                        else
                        {
                        }
                        
                        fv3 LightColor = {0,0,0}; // TODO: sample middle map
                        if (FarMap)
                        {
                            
                            fv3 FarMapColor = SampleEnvironmentMap(Camera, TexelGroundPosition, TexelHeight, BounceDirection, Normal.w, FarMap);
                            LightColor = fv3Lerp(LightColor, tFarMap, FarMapColor);
                        }
                        
                        Texel.rgb = Texel.rgb + Texel.a*LightColor;
                        
#if 0
                        Texel.rgb = fv3{0.5f, 0.5f, 0.5f} + 0.5f * Normal.rgb;
                        Texel.rgb *= Texel.a;
#endif
                    }
                    
                    Texel = fv4Hadamard(Texel, Color);
                    Texel.r = f32Clamp01(Texel.r);
                    Texel.g = f32Clamp01(Texel.g);
                    Texel.b = f32Clamp01(Texel.b);
                    
                    fv4 Dest = u32_to_fv4Color(*Pixel);
                    Dest = SRGBToLinear(Dest);
                    fv4 Blended = (1.0f-Texel.a)*Dest + Texel;
                    
                    u32 TexelColor = fv4_to_u32Color(Texel);//fv4_to_u32Color(LinearToSRGB(Blended));
                    
                    *Pixel = TexelColor;
                }
                else
                {
                    fv4 Dest = SRGBToLinear(u32_to_fv4Color(*Pixel));
                    fv4 Blended = (1.0f - Color.a)*Dest + Color;
                    
                    *Pixel = fv4_to_u32Color(LinearToSRGB(Blended));
                }
            }
            Pixel++;
        }
        DestRow += Buffer->Pitch;
    }
}

// NOTE: no bilinear filtering
// TODO: mouse over rectangle
// TODO: this test should be performed on a collision bitmap so that transparent areas in the center of the bitmap are still counted
// perhaps these collision bitmaps could be generated automatically when processing the sprites
internal b32
MouseOverBitmap(camera* Camera, fv2 MousePos,
                fv2 Origin, fv2 X_Axis, fv2 Y_Axis,
                bitmap* Hitmap)
{
    fv2 ProjMousePos = MousePos + Camera->Pos;
    fv2 HitmapSpaceP = CanonicalToBasis(ProjMousePos - Origin, X_Axis, Y_Axis);
    
    if ((HitmapSpaceP.x >= 0 && HitmapSpaceP.x <= 1.0f) &&
        (HitmapSpaceP.y >= 0 && HitmapSpaceP.y <= 1.0f))
    {
        
        if (Hitmap)
        {
            // TODO: unjank this
            real32 tX = (HitmapSpaceP.x * (real32)(Hitmap->Width - 2)) + 1;
            real32 tY = (HitmapSpaceP.y * (real32)(Hitmap->Height - 2)) + 1;
            
            int TexelX = RoundFloatToInt(tX);
            int TexelY = RoundFloatToInt(tY);
            
            u8* TexelPtr = ((u8*)Hitmap->Memory + 
                            (Hitmap->Height - TexelY - 1) * Hitmap->Pitch + 
                            TexelX * BITMAP_BYTES_PER_PIXEL);
            
            fv4 Texel = u32_to_fv4Color(*(u32*)TexelPtr);
            
            f32 Threshold = 0.5f;
            return Texel.a > Threshold;
        }
        else
        {
            return true;
        }
    }
    return false;
}

// NOTE: takes normalized screen space
inline render_entry_quad*
PushRenderOnScreenQuad(render_group* RenderGroup, fv2 NormScreenOffset, fv2 XAxis, fv2 YAxis)
{
    render_entry_header* Header = PushStruct(&RenderGroup->Buffer, render_entry_header);
    render_entry_quad* NewEntry = PushStruct(&RenderGroup->Buffer, render_entry_quad);
    
    Header->Type = RenderGroupEntry_Quad;
    
    ZeroStruct(*NewEntry);
    camera* Camera = RenderGroup->Camera;
    NewEntry->ProjPos = NormScreenToProj(Camera, RenderGroup->NormScreenOrigin + NormScreenOffset);
    NewEntry->XAxis = UnNormalizeScreenSpace(Camera, XAxis);
    NewEntry->YAxis = UnNormalizeScreenSpace(Camera, YAxis);
    
    return NewEntry;
}

inline fv3
CalculateAlign(fv2 XAxis, fv2 YAxis, fv2 NormAlign)
{
    fv2 Align = - NormAlign.x * XAxis - NormAlign.y * YAxis;
    fv3 ret = ProjToWorld(Align);
    return ret;
}

inline fv3
CalculateBitmapAlign(fv2 XAxis, fv2 YAxis, bitmap* Bitmap)
{
    fv3 ret = {};
    if (Bitmap)
    {
        ret = CalculateAlign(XAxis, YAxis, Bitmap->Align);
    }
    return ret;
}

inline render_entry_quad*
PushRenderQuad(render_group* RenderGroup, fv3 WorldOffset, fv2 XAxis, fv2 YAxis)
{
    
    
    render_entry_header* Header = PushStruct(&RenderGroup->Buffer, render_entry_header);
    render_entry_quad* NewEntry = PushStruct(&RenderGroup->Buffer, render_entry_quad);
    
    Header->Type = RenderGroupEntry_Quad;
    
    ZeroStruct(*NewEntry);
    
    NewEntry->XAxis = XAxis;
    NewEntry->YAxis = YAxis;
    NewEntry->ProjPos = WorldToProj(RenderGroup->WorldOrigin + WorldOffset);
    
    
    // TODO: make "card view" the default orientation. doesn't matter right now since no normal maps
    
    return NewEntry;
}

inline render_entry_quad*
PushRenderQuad(render_group* RenderGroup, game_asset_id AssetID, fv3 WorldOffset, fv2 XAxis, fv2 YAxis)
{
    render_entry_quad* Result = 0;
    bitmap* Bitmap = GetBitmap(RenderGroup->Assets, AssetID);
    if (Bitmap)
    {
        Result = PushRenderQuad(RenderGroup, WorldOffset, XAxis, YAxis);
        Result->Texture = Bitmap;
    }
    else
    {
        LoadAsset(RenderGroup->Assets, AssetID);
    }
    return Result;
}

inline render_entry_process_ui*
PushProcessUI(render_group* RenderGroup, fv3 WorldOffset, fv2 XAxis, fv2 YAxis,
              ui_context* UIContext, entity_id EntityID)
{
    
    
    render_entry_header* Header = PushStruct(&RenderGroup->Buffer, render_entry_header);
    render_entry_process_ui* NewEntry = PushStruct(&RenderGroup->Buffer, render_entry_process_ui);
    
    Header->Type = RenderGroupEntry_ProcessUI;
    
    ZeroStruct(*NewEntry);
    NewEntry->ProjPos = WorldToProj(RenderGroup->WorldOrigin + WorldOffset);
    NewEntry->XAxis = XAxis;
    NewEntry->YAxis = YAxis;
    NewEntry->UIContext = UIContext;
    NewEntry->EntityID = EntityID;
    
    return NewEntry;
}

inline void
PushClearBitmap(render_group* RenderGroup)
{
    render_entry_header* Header = PushStruct(&RenderGroup->Buffer, render_entry_header);
    render_entry_clear* NewEntry = PushStruct(&RenderGroup->Buffer, render_entry_clear);
    
    Header->Type = RenderGroupEntry_Clear;
}

inline render_entry_grid*
PushRenderGrid(render_group* RenderGroup)
{
    render_entry_header* Header = PushStruct(&RenderGroup->Buffer, render_entry_header);
    render_entry_grid* NewEntry = PushStruct(&RenderGroup->Buffer, render_entry_grid);
    
    Header->Type = RenderGroupEntry_Grid;
    
    return NewEntry;
}


internal void
RenderTile(bitmap* OutputTarget, render_group* RenderGroup, sv2Rectangle ClipRect, b32 Even)
{
    BEGIN_TIMED_BLOCK(ResolveRenderBuffer);
    
    arena* BufferArena = &RenderGroup->Buffer;
    u64 BufferIndex = 0;
    
    while (BufferIndex < BufferArena->Used)
    {
        render_entry_header* Header = (render_entry_header*)(BufferArena->Base + BufferIndex);
        BufferIndex += sizeof(render_entry_header);
        
        void* Entry = (void*)((u8*)Header + sizeof(render_entry_header));
        
        switch(Header->Type)
        {
            case RenderGroupEntry_Clear:
            {
                render_entry_clear* ClearEntry = (render_entry_clear*)Entry;
                sv2Rectangle OutputDim = GetBitmapDimensions(OutputTarget);
                DrawRectangle(OutputTarget, OutputDim, ClearEntry->Color, ClipRect, Even);
                BufferIndex += sizeof(render_entry_clear);
            } break;
            
            case RenderGroupEntry_Quad:
            {
                render_entry_quad* QuadEntry = (render_entry_quad*)Entry;
                
                if (QuadEntry->Texture)
                {
                    QuickDrawQuad(OutputTarget, RenderGroup->Camera, QuadEntry->ProjPos, QuadEntry->XAxis, QuadEntry->YAxis, QuadEntry->Color, QuadEntry->Texture, ClipRect, Even);
                }
                else
                {
                    DrawQuad(OutputTarget, RenderGroup->Camera, 
                             QuadEntry->ProjPos, QuadEntry->XAxis, QuadEntry->YAxis,
                             QuadEntry->Color, QuadEntry->Texture, 
                             QuadEntry->NormalMap, QuadEntry->SurfaceNormal, QuadEntry->SurfaceUp,
                             QuadEntry->Top, QuadEntry->Middle, QuadEntry->Bottom);
                }
                /*
                                DrawQuad(OutputTarget, RenderGroup->Camera, 
                                         QuadEntry->ProjPos, QuadEntry->XAxis, QuadEntry->YAxis,
                                         QuadEntry->Color, QuadEntry->Texture, 
                                         QuadEntry->NormalMap, QuadEntry->SurfaceNormal, QuadEntry->SurfaceUp,
                                         QuadEntry->Top, QuadEntry->Middle, QuadEntry->Bottom);
                */
                BufferIndex += sizeof(render_entry_quad);
                
            } break;
            
            case RenderGroupEntry_ProcessUI:
            {
                render_entry_process_ui* UI_Entry = (render_entry_process_ui*)Entry;
                
                // TODO: depth checking
                
                if (MouseOverBitmap(RenderGroup->Camera, UI_Entry->UIContext->MousePos,
                                    UI_Entry->ProjPos, UI_Entry->XAxis, UI_Entry->YAxis,
                                    UI_Entry->Hitmap))
                {
                    UI_Entry->UIContext->Hot = UI_Entry->EntityID;
                }
                
                BufferIndex += sizeof(render_entry_process_ui);
                
            } break;
            
            case RenderGroupEntry_Grid:
            {
                DrawGridlines(OutputTarget, RenderGroup);
                BufferIndex += sizeof(render_entry_grid);
            } break;
            
            
            InvalidDefaultCase;
        }
    }
    
    END_TIMED_BLOCK(ResolveRenderBuffer);
}

struct tile_render_work
{
    render_group* RenderGroup;
    bitmap* OutputTarget;
    sv2Rectangle ClipRect;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(DoTileRenderWork)
{
    tile_render_work* Work = (tile_render_work*)Data;
    
    RenderTile(Work->OutputTarget, Work->RenderGroup, Work->ClipRect, true);
    RenderTile(Work->OutputTarget, Work->RenderGroup, Work->ClipRect, false);
}


internal void
ResolveRenderBuffer(platform_work_queue* RenderQueue, bitmap* OutputTarget, render_group* RenderGroup)
{
    int const TileCountX = 4;
    int const TileCountY = 4;
    tile_render_work WorkArray[TileCountX * TileCountY];
    
    Assert(((uintptr)OutputTarget->Memory&15) == 0);
    int TileWidth = OutputTarget->Width / TileCountX;
    int TileHeight = OutputTarget->Height / TileCountY;
    
    TileWidth = ((TileWidth + 3) / 4)*4;
    
    int WorkIndex = 0;
    for (int TileY = 0;
         TileY < TileCountY;
         ++TileY)
    {
        for (int TileX = 0;
             TileX < TileCountX;
             ++TileX)
        {
            tile_render_work* Work = WorkArray + WorkIndex++;
            
            sv2Rectangle ClipRect = {
                TileX * TileWidth,
                TileY * TileHeight,
                ClipRect.MinX + TileWidth,
                ClipRect.MinY + TileHeight
            };
            
            if (TileX == TileCountX - 1)
            {
                ClipRect.MaxX = OutputTarget->Width;
            }
            if (TileY == TileCountY - 1)
            {
                ClipRect.MaxY = OutputTarget->Height;
            }
            
            Work->RenderGroup = RenderGroup;
            Work->OutputTarget = OutputTarget;
            Work->ClipRect = ClipRect;
            
#if 1
            // NOTE: multi-threaded path
            PlatformAddEntry(RenderQueue, DoTileRenderWork, Work);
#else
            DoTileRenderWork(RenderQueue, Work);
#endif
        }
    }
    PlatformCompleteAllWork(RenderQueue);
    ArenaReset(&RenderGroup->Buffer);
}
