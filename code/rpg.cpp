#include "rpg.h"
#include "render.cpp"
#include "entity.cpp"
#include "event.cpp"

// TODO:

// -----------------------
// GRAPHICS
// -----------------------
// normal maps
// optimization
// render buffer sorting
// combine DrawRectangle and DrawBitmap into DrawQuad
// raylib

// -----------------------
// DEBUG
// -----------------------
// Live code editing
// Loop
// Inspect entities

// -----------------------
// STRUCTURAL
// -----------------------
// metaprogram modifiers
// hero ability / modifier organization
// metaprogram autoinclude hero files

// -----------------------
// MAP
// -----------------------
// editor
// save changes to files

// -----------------------
// ASSETS
// -----------------------
// packed files
// asset processing
// preload assets



#pragma pack(push, 1)
struct bitmap_header
{
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size_;
    s32 Width;
    s32 Height;
    u16 Planes;
    u16 BitsPerPixel;
    u32 Compression;
    u32 SizeOfBitmap;
    s32 HorzResolution;
    s32 VertResolution;
    u32 ColorsUsed;
    u32 ColorsImportant;
    
    u32 RedMask;
    u32 GreenMask;
    u32 BlueMask;
    u32 AlphaMask;
};
#pragma pack(pop)

internal bitmap
DEBUGLoadBMP(thread_context* Thread, debug_platform_read_entire_file* ReadEntireFile, char* FileName)
{
    
    // NOTE: byte order is BGRA, bottom up
    
    bitmap Result = {};
    
    debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);
    if (ReadResult.ContentSize != 0)
    {
        bitmap_header* Header = (bitmap_header*)ReadResult.Content;
        u32* Pixels = (u32*)((u8*)ReadResult.Content + Header->BitmapOffset);
        Result.Memory = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        
        u32 RedMask = Header->RedMask;
        u32 GreenMask = Header->GreenMask;
        u32 BlueMask = Header->BlueMask;
        u32 AlphaMask = Header->AlphaMask;
        
        bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);
        
        Assert(RedScan.Found);
        Assert(GreenScan.Found);
        Assert(BlueScan.Found);
        Assert(AlphaScan.Found);
        
        s32 RedShift = 16 - (s32)RedScan.Index;
        s32 GreenShift = 8 - (s32)GreenScan.Index;
        s32 BlueShift = 0 - (s32)BlueScan.Index;
        s32 AlphaShift = 24 - (s32)AlphaScan.Index;
        
        u32* SourcePtr = Pixels;
        for (int Y = 0;
             Y < Header->Height;
             ++Y)
        {
            for (int X = 0;
                 X < Header->Width;
                 ++X)
            {
                u32 C = *SourcePtr;
                
                u32 Texel32 = (RotateLeft(C & RedMask, RedShift) |
                               RotateLeft(C & GreenMask, GreenShift) |
                               RotateLeft(C & BlueMask, BlueShift) |
                               RotateLeft(C & AlphaMask, AlphaShift));
                
                fv4 Texel = u32_to_fv4Color(Texel32);
                Texel = SRGBToLinear(Texel);
                Texel.rgb *= Texel.a;
                Texel = LinearToSRGB(Texel);
                
                *SourcePtr++ = fv4_to_u32Color(Texel);
            }
        }
    }
    
    Result.Pitch = -Result.Width*BITMAP_BYTES_PER_PIXEL;
    Result.Memory = (uint8 *)Result.Memory - Result.Pitch*(Result.Height - 1);
    
    return Result;
}


#if 0
internal void
DEBUGOutlineRectangle(game_offscreen_buffer* Buffer, fv2 center, fv2 dimensions, u32 Color)
{
    f32 MinX = center.x - dimensions.x / 2.0f;
    f32 MinY = center.y - dimensions.y / 2.0f;
    f32 MaxX = center.x + dimensions.x / 2.0f;
    f32 MaxY = center.y + dimensions.y / 2.0f;
    
    DEBUGDrawLine(Buffer, {MinX, MinY}, {MaxX, MinY}, Color);
    DEBUGDrawLine(Buffer, {MinX, MinY}, {MinX, MaxY}, Color);
    DEBUGDrawLine(Buffer, {MinX, MaxY}, {MaxX, MaxY}, Color);
    DEBUGDrawLine(Buffer, {MaxX, MinY}, {MaxX, MaxY}, Color);
}
#endif


inline sv2
GetTileAtPosition(fv2 IsoPosition)
{
    return {
        (int)(IsoPosition.x < 0 ? IsoPosition.x - 1 : IsoPosition.x),
        (int)(IsoPosition.y < 0 ? IsoPosition.y - 1 : IsoPosition.y)
    };
}

internal void
MoveEntity(game_state* GameState, entity_id Entity, f32 dtForFrame)
{
    entity* Mover = GetEntity(GameState, Entity);
    
    // generate a path if there isn't one already
    sv2 MoverTile = GetTileAtPosition(Mover->Pos);
    
    if (Mover->PathArena.bytes_used == 0)
    {
        arena ScratchArena = NestArena(&GameState->GameArena, sizeof(path_node) * MAX_NODES + sizeof(path_node*) * MAX_NODES * 2);
        GeneratePath(&ScratchArena, &Mover->PathArena, 
                     MoverTile, Mover->MoveTarget,
                     GameState->Obstacles, GameState->ObstacleCount);
        ArenaPop(&GameState->GameArena, ScratchArena.base);
    }
    
    // move
    f32 RemainingDistance = Mover->MoveSpeed * dtForFrame;
    while (Mover->PathArena.bytes_used > 0 &&
           RemainingDistance > 0)
    {
        sv2 NodeLocation = *(sv2*)(Mover->PathArena.base + Mover->PathArena.bytes_used - sizeof(sv2));
        
        if (FindObstacle(NodeLocation, GameState->Obstacles, GameState->ObstacleCount) >= 0)
        {
            break;
        }
        
        fv2 NodeCenter = {
            (f32)NodeLocation.x + 0.5f,
            (f32)NodeLocation.y + 0.5f
        };
        
        f32 DistanceToNode = fv2Length(NodeCenter - Mover->Pos);
        if (RemainingDistance < DistanceToNode)
        {
            Mover->Pos += RemainingDistance * fv2Normalize(NodeCenter - Mover->Pos);
            RemainingDistance = 0;
        }
        else
        {
            Mover->Pos.x = NodeCenter.x;
            Mover->Pos.y = NodeCenter.y;
            Mover->PathArena.bytes_used -= sizeof(sv2);
            RemainingDistance -= DistanceToNode;
            
            if (MoverTile == Mover->MoveTarget)
            {
                Mover->IsMoving = false;
                return;
            }
        }
    }
}

inline bitmap
MakeEmptyBitmap(arena* Arena, int Width, int Height, b32 ClearToZero)
{
    bitmap ret;
    ret.Width = Width;
    ret.Height = Height;
    ret.Pitch = Width * BITMAP_BYTES_PER_PIXEL;
    int TotalBitmapSize = ret.Pitch * ret.Height;
    ret.Memory = ArenaPush(Arena, TotalBitmapSize);
    
    if (ClearToZero)
    {
        ZeroSize(TotalBitmapSize, ret.Memory);
    }
    
    return ret;
}

internal void
MakeSphereNormalMap(bitmap* Output, f32 Roughness)
{
    real32 InvWidth = 1.0f / (real32)(Output->Width - 1);
    real32 InvHeight = 1.0f / (real32)(Output->Height - 1);
    
    u8* Row = (u8*)Output->Memory;
    for (int Y = 0;
         Y < Output->Height;
         ++Y)
    {
        u32* Pixel = (u32*)Row;
        for(int X = 0;
            X < Output->Width;
            ++X)
        {
            fv2 BitmapUV = {
                InvWidth * real32(X),
                InvHeight * real32(Y)
            };
            
            f32 Nx = 2.0f*BitmapUV.x - 1.0f;
            f32 Ny = 2.0f*BitmapUV.y - 1.0f;
            
            f32 RootTerm = 1.0f - Square(Nx) - Square(Ny);
            fv3 Normal = {0.0f, 0.0f, 1.0f};
            if (RootTerm >= 0.0f)
            {
                f32 Nz = SquareRoot(RootTerm);
                Normal = {Nx, Ny, Nz};
            }
            
            fv4 Color = {
                0.5f * (Normal.x + 1.0f),
                0.5f * (Normal.y + 1.0f),
                0.5f * (Normal.z + 1.0f),
                Roughness
            };
            
            *Pixel++ = fv4_to_u32Color(Color);
        }
        Row += Output->Pitch;
    }
}


extern "C" GAME_INITIALIZE_MEMORY(GameInitializeMemory)
{
    game_state* GameState = (game_state*)Memory->Base;
    
    char* Filename = __FILE__;
    
    u64 TransientStorageSize = Megabytes(64);
    GameState->GameArena = MakeArena((void*)((u8*)Memory->Base + sizeof(game_state)), Memory->Size - sizeof(game_state));
    GameState->FrameArena = NestArena(&GameState->GameArena, TransientStorageSize);
    GameState->PathArenas = NestArena(&GameState->GameArena, PATH_ARENA_SIZE * MAX_ENTITIES);
    GameState->ModifierArenas = 
        NestArena(&GameState->GameArena, MODIFIER_ARENA_SIZE * MAX_ENTITIES);
    
    GameState->WhiteSquare = MakeEmptyBitmap(&GameState->GameArena, 100, 100, false);
    {
        bitmap* WhiteSquare = &GameState->WhiteSquare;
        u8* Row = (u8*)WhiteSquare->Memory;
        for (int Y = 0;
             Y < 100;
             ++Y)
        {
            u32* Pixel = (u32*)Row;
            for (int X = 0;
                 X < 100;
                 ++X)
            {
                *Pixel++ = fv4_to_u32Color({1.0f, 1.0f, 1.0f, 1.0f});
            }
            Row += WhiteSquare->Pitch;
        }
    }
    
    GameState->TestNormal = MakeEmptyBitmap(&GameState->GameArena, 100, 100, false);
    MakeSphereNormalMap(&GameState->TestNormal, 0.0f);
    
    GameState->Camera.Pos = {0.0f, 0.0f};
    GameState->Camera.Dim = {16.0f, 9.0f};
    GameState->MainRender.x_transform = {1.0f, 0.5f};
    GameState->MainRender.y_transform = {-1.0f, 0.5f};
    GameState->MainRender.Camera = &GameState->Camera;
    GameState->MainRender.Buffer = NestArena(&GameState->GameArena, Megabytes(4));
    
    GameState->Entities = PushArray(&GameState->GameArena, entity, MAX_ENTITIES);
    
    GameState->Hero = CreateEntity(GameState);
    entity* HeroEntity = GetEntity(GameState, GameState->Hero);
    HeroEntity->Pos = {0.0f, 0.0f};
    HeroEntity->MoveSpeed = 10.0f;
    HeroEntity->Bitmap = DEBUGLoadBMP(0, Memory->DEBUGPlatformReadEntireFile, "W:\\rpg\\data\\Hero.bmp");
    
    entity_id Enemy_ID = CreateEntity(GameState);
    entity* Enemy = GetEntity(GameState, Enemy_ID);
    Enemy->Pos = {10.5f, 10.5f};
    Enemy->Bitmap = HeroEntity->Bitmap;
    
    GameState->UIContext.Hot = 0;
    GameState->UIContext.Active = 0;
    
    // test
    // TODO: #define this?
    RegisterListener(GameState->EventHandler.OnAttackListeners, GameState->EventHandler.OnAttackListenerCount, AlchemistOnAttack);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{   
    Assert((&Input->Controllers[0].Terminator - Input->Controllers[0].Buttons) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->Size);
    
    game_state* GameState = (game_state*)Memory->Base;
    
    bitmap DrawBuffer_ = {};
    bitmap* DrawBuffer = &DrawBuffer_;
    DrawBuffer->Width = Buffer->Width;
    DrawBuffer->Height = Buffer->Height;
    DrawBuffer->Pitch = Buffer->Pitch;
    DrawBuffer->Memory = Buffer->Memory;
    
    DEBUGClearToBlack(DrawBuffer);
    
    camera* GameCamera = &GameState->Camera;
    
    game_controller_input* PlayerOneController = GetController(Input, 0);
    
    // camera grip
    if (IsDown(PlayerOneController->CameraGrip))
    {
        if (!(PlayerOneController->CameraGrip.HalfTransitionCount > 0))
        {
            fv2 DeltaMousePos = Input->MousePos - GameState->UIContext.LastMousePos;
            
            GameCamera->Pos -= { DeltaMousePos.x * GameCamera->Dim.x, DeltaMousePos.y * GameCamera->Dim.y };
        }
        GameState->UIContext.LastMousePos = Input->MousePos;
    }
    
    // place obstacles
    if (WasPressed(PlayerOneController->Select))
    {
        fv2 IsoMousePos = NormScreenToWorld(&GameState->MainRender, Input->MousePos);
        sv2 TilePosition = GetTileAtPosition(IsoMousePos);
        
        s32 ObstaclePos = FindObstacle(TilePosition, GameState->Obstacles, GameState->ObstacleCount);
        if (ObstaclePos >= 0)
        {
            for (s32 ObstacleIter = ObstaclePos;
                 ObstacleIter < (GameState->ObstacleCount - 1);
                 ++ObstacleIter)
            {
                GameState->Obstacles[ObstacleIter].x = GameState->Obstacles[ObstacleIter + 1].x;
                GameState->Obstacles[ObstacleIter].y = GameState->Obstacles[ObstacleIter + 1].y;
                
            }
            GameState->ObstacleCount--;
        }
        else 
        {
            sv2* NewObstacle = GameState->Obstacles + GameState->ObstacleCount%MAX_ENTITIES;
            NewObstacle->x = TilePosition.x;
            NewObstacle->y = TilePosition.y;
            
            GameState->ObstacleCount++;
        }
    }
    
    // TODO: sorting
    PushRenderGrid(&GameState->MainRender);
    
    
    entity* HeroEntity = GetEntity(GameState, GameState->Hero);
    
    // TODO: set move targets for entities in selection group
    // set move target
    if (IsDown(PlayerOneController->Move))
    {
        fv2 IsoMousePos = NormScreenToWorld(&GameState->MainRender, Input->MousePos);
        
        HeroEntity->MoveTarget = GetTileAtPosition(IsoMousePos);
        HeroEntity->IsMoving = true;
        
        ArenaReset(&HeroEntity->PathArena); // re-generate path
    }
    
    // TODO: entity update order?
    // update entities
    for (entity_id EntityID = 1;
         EntityID <= MAX_ENTITIES;
         ++EntityID)
    {
        if (EntityExists(GameState, EntityID))
        {
            entity* Entity = GetEntity(GameState, EntityID);
            if (Entity->IsMoving)
            {
                MoveEntity(GameState, EntityID, Input->dtForFrame);
            }
            
            fv4 EntityColor = {0.5f, 0.1f, 1.0f, 1.0f};
            
            PushRenderQuad(&GameState->MainRender, Entity->Pos, 
                           2*GameState->MainRender.x_transform, 
                           2*GameState->MainRender.y_transform,
                           EntityColor,
                           &Entity->Bitmap, 0,
                           0, 0, 0);
            PushProcessUI(&GameState->MainRender, Entity->Pos,
                          2*GameState->MainRender.x_transform, 
                          2*GameState->MainRender.y_transform,
                          &Entity->Bitmap,
                          &GameState->UIContext, EntityID, Input->MousePos);
        }
    }
    for (int ObstacleIndex = 0;
         ObstacleIndex < GameState->ObstacleCount;
         ++ObstacleIndex)
    {
        fv3 ObstaclePos = {
            (f32)GameState->Obstacles[ObstacleIndex].x,
            (f32)GameState->Obstacles[ObstacleIndex].y,
            0
        };
        
        PushRenderQuad(&GameState->MainRender, ObstaclePos, 
                       GameState->MainRender.x_transform, 
                       GameState->MainRender.y_transform,
                       {0.5f, 0.5f, 0.5f, 1.0f},
                       0, 0,
                       0, 0, 0);
    }
    
    GameState->UIContext.Hot = 0;
    
    PushRenderQuad(&GameState->MainRender, {0.0f, 0.0f}, {2.0f, 0.0f}, {0.0f, 2.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, &GameState->WhiteSquare, &GameState->TestNormal, 0, 0, 0);
    ResolveRenderBuffer(DrawBuffer, &GameState->MainRender);
    
    if (WasPressed(PlayerOneController->Attack))
    {
        if (GameState->UIContext.Hot)
        {
            entity* HotEntity = GetEntity(GameState, GameState->UIContext.Hot);
        }
        else
        {
            fv2 IsoMousePos = NormScreenToWorld(&GameState->MainRender, Input->MousePos);
            // attack move to IsoMousePos
        }
    }
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state* GameState = (game_state*)Memory->Base;
    //GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}

