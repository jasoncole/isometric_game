
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

inline fv3
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

inline fv3
SampleEnvironmentMap(fv3 Normal, f32 Roughness, environment_map* Map)
{
    fv3 ret = Normal;
    
    u32 LODIndex = (u32)(Roughness*(real32)(ArrayCount(Map->LOD)-1) + 0.5f);
    Assert(LODIndex < ArrayCount(Map->LOD));
    
    bitmap* LOD = Map->LOD[LODIndex];
    
    bilinear_sample Sample = BilinearSample(LOD, MapX, MapY);
    fv3 Color = SRGBBilinearBlend(Sample, fx, fy).xyz;
    return ret;
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


internal inline void
FillPixel(u32* Pixel, u32 Color)
{
    if ((Color >> 24) >= (*Pixel >> 24))
    {
        *Pixel = Color;
    }
}

void
DEBUGClearToBlack(bitmap* Buffer)
{
    u8* Row = ((u8*)Buffer->Memory);
    for (int Y = 0;
         Y < Buffer->Height;
         ++Y)
    {
        uint32* Pixel = (uint32*)Row;
        for (int X = 0;
             X < Buffer->Width;
             ++X)
        {
            *Pixel++ = 0x00000000;
        }
        Row += Buffer->Pitch;
    }
}

// NOTE: coordinate systems
// projected space: non-isometric 2D coordinates relative to world
// screen space: projected space relative to camera
// normalized screen space: screen space normalized as a percentage of screen dimensions
// world space: 3D isometric coordinates relative to world

inline fv2
NormalizeScreenSpace(camera* Camera, fv2 ScreenSpace)
{
    if (Camera->Dim.x == 0.0f || Camera->Dim.y == 0.0f) 
    {
        InvalidCodePath;
        return {};
    }
    
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
    
    fv2 InvXAxis= {InvDet*YAxis.y, -InvDet*XAxis.y};
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

internal fv3
ProjToWorld(render_group* RenderGroup, fv2 Proj, f32 z = 0)
{
    // TODO: get Z value from the world
    // z = GetZValueAtPos(Proj)
    
    Proj.y -= RenderGroup->Z_to_Y * z;
    
    fv3 WorldPos = CanonicalToBasis(Proj, RenderGroup->x_transform, RenderGroup->y_transform);
    WorldPos.z = z;
    
    return WorldPos;
}

inline fv2
WorldToProj(render_group* RenderGroup, fv3 WorldPos)
{
    fv2 ret = BasisToCanonical(WorldPos, RenderGroup->x_transform, RenderGroup->y_transform);
    ret.y += RenderGroup->Z_to_Y * WorldPos.z;
    return ret;
}

inline fv2
ProjToNormScreen(camera* Camera, fv3 Proj)
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

inline fv2
NormScreenToWorld(render_group* RenderGroup, fv2 NormScreen)
{
    fv2 ret = UnNormalizeScreenSpace(RenderGroup->Camera, NormScreen) + RenderGroup->Camera->Pos;
    ret = ProjToWorld(RenderGroup, ret, 0);
    return ret;
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


// TODO: doesn't work with non-standard transformation matrix
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
        Corners[CornerIndex] = ProjToWorld(RenderGroup, Corners[CornerIndex], 0);
    }
    
    fv2 Min = fv2FindMin(Corners, 4);
    fv2 Max = fv2FindMax(Corners, 4);
    
    int MinX = s32Floor(Min.x);
    int MinY = s32Floor(Min.y);
    int MaxX = s32Ceiling(Max.x);
    int MaxY = s32Ceiling(Max.y);
    
    f32 OrthXSlope = RenderGroup->y_transform.y / RenderGroup->y_transform.x;
    f32 OrthYSlope = RenderGroup->x_transform.y / RenderGroup->x_transform.x;
    
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


inline b32 
InsideEdge(fv2 V1, fv2 V2, fv2 P) 
{ 
    return ((P.x - V1.x) * (V2.y - V1.y) - (P.y - V1.y) * (V2.x - V1.x) + EPSILON >= 0); 
}


internal void
DrawQuad(bitmap* Buffer, camera* Camera,
         fv2 Origin, fv2 X_Axis, fv2 Y_Axis,
         fv4 Color,
         bitmap* Texture, bitmap* NormalMap,
         environment_map* Top,
         environment_map* Middle,
         environment_map* Bottom)
{
    Color.rgb *= Color.a;
    
    int WidthMax = Buffer->Width - 1;
    int HeightMax = Buffer->Height - 1;
    
    // BoundingRect
    fv2 Corners[4] = {
        Origin, 
        Origin + X_Axis, 
        Origin + X_Axis + Y_Axis, 
        Origin + Y_Axis
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
                    
                    // TODO: this projection is in projected coordinate space
                    // it should be in the texture coordinate space
                    // normalized texture space
                    
                    fv2 TextureSpaceP = CanonicalToBasis(P, 
                                                         Corners[1] - Corners[0],
                                                         Corners[3] - Corners[0]);
                    
#if 0
                    // TODO(casey): SSE clamping.
                    Assert((TextureSpaceP.x >= 0.0f) && (TextureSpaceP.x <= 1.0f));
                    Assert((TextureSpaceP.y >= 0.0f) && (TextureSpaceP.y <= 1.0f));
#endif
                    
                    // TODO: unjank this
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
                        
                        Normal = BiasNormal(Normal);
                        
#if 1
                        environment_map* FarMap = 0;
                        f32 tEnvMap =  Normal.y;
                        f32 tFarMap = 0.0f;
                        if (tEnvMap < -0.5f)
                        {
                            FarMap = Bottom;
                            tFarMap = 2.0f * (tEnvMap + 1.0f);
                        }
                        else if (tEnvMap > 0.5f)
                        {
                            FarMap = Top;
                            tFarMap = 2.0f * (tEnvMap - 0.5f);
                        }
                        else
                        {
                        }
                        
                        fv3 LightColor = SampleEnvironmentMap(Normal.xyz, Normal.w, Middle);
                        if (FarMap)
                        {
                            fv3 FarMapColor = SampleEnvironmentMap(Normal.xyz, Normal.w, FarMap);
                            LightColor = fv3Lerp(LightColor, tFarMap, FarMapColor);
                        }
                        
                        Texel.rgb = Texel.rgb + Texel.a*LightColor;
#else
                        Texel.rgb = fv3{0.5f, 0.5f, 0.5f} + 0.5f * Normal.rgb;
                        Texel.a = 1.0f;
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
                bitmap* Texture)
{
    fv2 ProjMousePos = NormScreenToProj(Camera, MousePos);
    fv2 TextureSpaceP = CanonicalToBasis(ProjMousePos - Origin, X_Axis, Y_Axis);
    
    if ((TextureSpaceP.x < 0 || TextureSpaceP.x > 1.0f) ||
        (TextureSpaceP.y < 0 || TextureSpaceP.y > 1.0f))
    {
        return false;
    }
    
    // TODO: unjank this
    real32 tX = TextureSpaceP.x * (real32)(Texture->Width - 2);
    real32 tY = TextureSpaceP.y * (real32)(Texture->Height - 2);
    
    int TexelX = RoundFloatToInt(tX);
    int TexelY = RoundFloatToInt(tY);
    
    u8* TexelPtr = ((u8*)Texture->Memory + 
                    (Texture->Height - TexelY - 1) * Texture->Pitch + 
                    TexelX * BITMAP_BYTES_PER_PIXEL);
    
    fv4 Texel = u32_to_fv4Color(*(u32*)TexelPtr);
    
    f32 Threshold = 0.5f;
    return Texel.a > Threshold;
}

inline void
PushRenderQuad(render_group* RenderGroup,
               fv3 WorldPos, fv2 XAxis, fv2 YAxis,
               fv4 Color,
               bitmap* Texture, bitmap* NormalMap,
               environment_map* Top, environment_map* Middle, environment_map* Bottom)
{
    render_entry_header* Header = PushStruct(&RenderGroup->Buffer, render_entry_header);
    render_entry_quad* NewEntry = PushStruct(&RenderGroup->Buffer, render_entry_quad);
    
    Header->Type = RenderGroupEntry_Quad;
    NewEntry->Texture = Texture;
    NewEntry->NormalMap = NormalMap;
    NewEntry->Color = Color;
    
    NewEntry->ProjPos = WorldToProj(RenderGroup, WorldPos);
    NewEntry->XAxis = XAxis;
    NewEntry->YAxis = YAxis;
    
    NewEntry->Top = Top;
    NewEntry->Middle = Middle;
    NewEntry->Bottom = Bottom;
}


// TODO: I don't want to have to change this every time I change PushRenderQuad
inline void
PushProcessUI(render_group* RenderGroup,
              fv3 WorldPos, fv2 XAxis, fv2 YAxis,
              bitmap* Texture, 
              ui_context* UIContext, entity_id EntityID,
              fv2 MousePos)
{
    render_entry_header* Header = PushStruct(&RenderGroup->Buffer, render_entry_header);
    render_entry_process_ui* NewEntry = PushStruct(&RenderGroup->Buffer, render_entry_process_ui);
    
    Header->Type = RenderGroupEntry_ProcessUI;
    NewEntry->Texture = Texture;
    NewEntry->ProjPos = WorldToProj(RenderGroup, WorldPos);
    NewEntry->XAxis = XAxis;
    NewEntry->YAxis = YAxis;
    NewEntry->UIContext = UIContext;
    NewEntry->EntityID = EntityID;
    NewEntry->MousePos = MousePos;
}

inline void
PushRenderGrid(render_group* RenderGroup)
{
    render_entry_header* Header = PushStruct(&RenderGroup->Buffer, render_entry_header);
    render_entry_grid* NewEntry = PushStruct(&RenderGroup->Buffer, render_entry_grid);
    
    Header->Type = RenderGroupEntry_Grid;
}

void
ResolveRenderBuffer(bitmap* Buffer, render_group* RenderGroup)
{
    arena* BufferArena = &RenderGroup->Buffer;
    u64 BufferIndex = 0;
    
    while (BufferIndex < BufferArena->bytes_used)
    {
        render_entry_header* Header = (render_entry_header*)(BufferArena->base + BufferIndex);
        BufferIndex += sizeof(render_entry_header);
        
        void* Entry = (void*)((u8*)Header + sizeof(render_entry_header));
        
        switch(Header->Type)
        {
            case RenderGroupEntry_Clear:
            {
                DEBUGClearToBlack(Buffer);
                BufferIndex += sizeof(render_entry_clear);
            } break;
            
            
            case RenderGroupEntry_Quad:
            {
                render_entry_quad* QuadEntry = (render_entry_quad*)Entry;
                DrawQuad(Buffer, RenderGroup->Camera, 
                         QuadEntry->ProjPos, QuadEntry->XAxis, QuadEntry->YAxis,
                         QuadEntry->Color,
                         QuadEntry->Texture, QuadEntry->NormalMap, QuadEntry->Top, QuadEntry->Middle, QuadEntry->Bottom);
                BufferIndex += sizeof(render_entry_quad);
                
            } break;
            
            case RenderGroupEntry_ProcessUI:
            {
                render_entry_process_ui* UI_Entry = (render_entry_process_ui*)Entry;
                
                // TODO: depth checking
                if (MouseOverBitmap(RenderGroup->Camera, UI_Entry->MousePos,
                                    UI_Entry->ProjPos, UI_Entry->XAxis, UI_Entry->YAxis,
                                    UI_Entry->Texture))
                {
                    UI_Entry->UIContext->Hot = UI_Entry->EntityID;
                }
                
                BufferIndex += sizeof(render_entry_process_ui);
                
            } break;
            
            case RenderGroupEntry_Grid:
            {
                DrawGridlines(Buffer, RenderGroup);
                BufferIndex += sizeof(render_entry_grid);
            } break;
            
            
            InvalidDefaultCase;
        }
    }
    
    ArenaReset(BufferArena);
}
