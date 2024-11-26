#include "rpg.h"
#include "render.cpp"
#include "entity.cpp"
#include "modifier.cpp"
#include "think.cpp"
#include "attack.cpp"
#include "generated.cpp"

/*
TODO:

Make modifiers entities?
modifiers update and some of them are rendered

modifiers
spells
dynamic memory allocator
probabilistic movement
cleanup

asset builder
fonts
debug tools

-----------------------
BUGS
-----------------------
walking speed affected by framerate - hard to replicate bug
drawquad is not drawing overdrawing correctly - we don't plan on using drawquad in the long run so just ignore this
walking to a directly adjacent tile is not handled correctly

-----------------------
RENDER
-----------------------
render buffer sorting
raylib

-----------------------
DEBUG
-----------------------
Inspect entities

-----------------------
ENGINE
-----------------------
dynamic memory allocator
read the spell info values from a text file - or - load the structs directly form an asset file
chunk-based entity storage (to cut down on aoe search time)
metaprogram autoinclude hero files
make think work on subtick timing

-----------------------
MAP EDITOR
-----------------------
place objects
save changes to files

-----------------------
ASSETS
-----------------------
asset tagging
packed files
asset processing


*/

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
DEBUGLoadBMP(thread_context* Thread, debug_platform_read_entire_file* ReadEntireFile, char* FileName, 
             fv2 Align = {})
{
    // NOTE: pass align in pixels based on origin in top left because that's how Paint.net shows it
    
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
        Result.Pitch = Result.Width * BITMAP_BYTES_PER_PIXEL;
        Result.Align = FV2(Align.x / Result.Width, 1.0f - Align.y / Result.Height);
        
        Assert(Result.Height >= 0);
        Assert(Header->Compression == 3);
        
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
    
#if 0
    Result.Memory = (uint8 *)Result.Memory * Result.Pitch*(Result.Height - 1);
    Result.Pitch = -Result.Pitch;
#endif
    
    return Result;
}


internal void
DEBUGDrawRectangle(bitmap* Buffer, sv2 Min, sv2 Max, fv4 Color)
{
    u32 Color32 = fv4_to_u32Color(Color);
    
    Min.x = s32Clamp(0, Buffer->Width, Min.x);
    Min.y = s32Clamp(0, Buffer->Height, Min.y);
    Max.x = s32Clamp(0, Buffer->Width, Max.x);
    Max.y = s32Clamp(0, Buffer->Height, Max.y);
    
    u8* Row = (u8*)Buffer->Memory + 
        Min.y * Buffer->Pitch + 
        Min.x * sizeof(u32);
    for (int Y = Min.y;
         Y < Max.y;
         ++Y)
    {
        u32* Pixel = (u32*)Row;
        for (int X = Min.x;
             X < Max.x;
             ++X)
        {
            *Pixel++ = Color32;
        }
        Row += Buffer->Pitch;
    }
}


inline sv2
GetTileAtPosition(fv3 IsoPos)
{
    // TODO: take into account Z
    sv2 ret = {
        (int)(IsoPos.x < 0 ? IsoPos.x - 1 : IsoPos.x),
        (int)(IsoPos.y < 0 ? IsoPos.y - 1 : IsoPos.y)
    };
    return ret;
}

internal f32 
MoveEntity(game_state* GameState, entity* Mover, f32 dtForFrame)
{
    //entity* Mover = GetEntity(GameState, Entity);
    
    // generate a path if there isn't one already
    fv3 MoverPosition_ = WorldToIso(Mover->Pos);
    fv2 MoverPosition = MoverPosition_.xy;
    sv2 MoverTile = GetTileAtPosition(MoverPosition_);
    
    if (Mover->PathArena.Used == 0)
    {
        arena ScratchArena = NestArena(&GameState->GameArena, sizeof(path_node) * MAX_NODES + sizeof(path_node*) * MAX_NODES * 2);
        GeneratePath(&ScratchArena, &Mover->PathArena, 
                     MoverTile, Mover->MoveTarget,
                     GameState->Obstacles, GameState->ObstacleCount);
        ArenaPop(&GameState->GameArena, ScratchArena.Base);
    }
    
    // move
    f32 RemainingDistance = Mover->MoveSpeed * dtForFrame;
    while (Mover->PathArena.Used > 0 &&
           RemainingDistance > 0)
    {
        sv2 NodeLocation = *(sv2*)(Mover->PathArena.Base + Mover->PathArena.Used - sizeof(sv2));
        
        if (FindObstacle(NodeLocation, GameState->Obstacles, GameState->ObstacleCount) >= 0)
        {
            break;
        }
        
        fv2 NodeCenter = {
            (f32)NodeLocation.x + 0.5f,
            (f32)NodeLocation.y + 0.5f
        };
        
        f32 DistanceToNode = fv2Length(NodeCenter - MoverPosition);
        if (RemainingDistance < DistanceToNode)
        {
            Mover->Pos += IsoToWorld(fv2tofv3(RemainingDistance * fv2Normalize(NodeCenter - MoverPosition), 0.0f));
            RemainingDistance = 0;
        }
        else
        {
            Mover->Pos = IsoToWorld(fv2tofv3(NodeCenter, 0));
            Mover->PathArena.Used -= sizeof(sv2);
            RemainingDistance -= DistanceToNode;
            
            if (MoverTile == Mover->MoveTarget)
            {
                Mover->MoveThink = 0;
                return -1;
            }
        }
    }
    return 0;
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

internal void
MakeSphereDiffuseMap(bitmap* Output, fv4 BaseColor)
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
            
            real32 Alpha = 0;
            f32 RootTerm = 1.0f - Square(Nx) - Square(Ny);
            if (RootTerm >= 0.0f)
            {
                Alpha = BaseColor.a;
            }
            
            fv4 Color = {
                Alpha * BaseColor.x,
                Alpha * BaseColor.y,
                Alpha * BaseColor.z,
                Alpha
            };
            
            *Pixel++ = fv4_to_u32Color(Color);
        }
        Row += Output->Pitch;
    }
}

internal task_with_memory *
BeginTaskWithMemory(game_state *GameState)
{
    task_with_memory* FoundTask = 0;
    
    for(uint32 TaskIndex = 0;
        TaskIndex < ArrayCount(GameState->Tasks);
        ++TaskIndex)
    {
        task_with_memory *Task = GameState->Tasks + TaskIndex;
        if(!Task->BeingUsed)
        {
            FoundTask = Task;
            Task->BeingUsed = true;
            Task->PopIndex = Task->Arena.Used;
            break;
        }
    }
    
    return(FoundTask);
}

inline void
EndTaskWithMemory(task_with_memory *Task)
{
    ArenaPopIndex(&Task->Arena, Task->PopIndex);
    
    CompletePreviousWritesBeforeFutureWrites;
    
    Task->BeingUsed = false;
}


struct load_asset_work
{
    game_assets* Assets;
    char* FileName;
    game_asset_id ID;
    bitmap* Bitmap;
    fv2 Align;
    task_with_memory* Task;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork)
{
    load_asset_work* Work = (load_asset_work*)Data;
    *Work->Bitmap = DEBUGLoadBMP(0, Work->Assets->ReadEntireFile, Work->FileName, Work->Align);
    
    CompletePreviousWritesBeforeFutureWrites;
    
    Work->Assets->Bitmaps[Work->ID].Bitmap = Work->Bitmap;
    Work->Assets->Bitmaps[Work->ID].State = AssetState_Loaded;
    
    EndTaskWithMemory(Work->Task);
    
}

internal void
LoadAsset(game_assets* Assets, game_asset_id ID)
{
    if (AtomicCompareExchangeUInt32((uint32*)&Assets->Bitmaps[ID].State, AssetState_Unloaded, AssetState_Queued) ==
        AssetState_Unloaded)
    {
        task_with_memory* Task = BeginTaskWithMemory(Assets->GameState);
        if (Task)
        {
            load_asset_work* Work = PushStruct(&Task->Arena, load_asset_work);
            
            Work->Assets = Assets;
            Work->FileName = "";
            Work->ID = ID;
            Work->Task = Task;
            Work->Bitmap = PushStruct(&Assets->Arena, bitmap);
            Work->Align = FV2(0,0);
            
            switch(ID)
            {
                case GAI_Hero:
                {
                    Work->FileName = "W:\\rpg\\data\\hero.bmp";
                    Work->Align = FV2(50, 116);
                } break;
                InvalidDefaultCase;
            };
            
            PlatformAddEntry(Assets->GameState->HighPriorityQueue, LoadAssetWork, Work);
        }
    }
}

#if 0
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

internal void
MakeGlyphTest()
{
    debug_read_file_result TTFFile = Platform.ReadEntireFile("c:/Windows/Fonts/arial.ttf");
    
    stbtt_fontinfo Font;
    stbtt_InitFont(&Font, (u8*)TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8*)TTFFile.Contents,0));
    
    int Width, Height, XOffset, YOffset;
    u8* MonoBitmap = stbtt_GetCodepointBitmap(&Font, 0,stbtt_ScaleForPixelHeight(&Font, 128.0f), 'N', &Width, &Height, &XOffset, &YOffset);
    stbtt_FreeBitmap(MonoBitmap, 0);
}
#endif 

extern "C" GAME_INITIALIZE_MEMORY(GameInitializeMemory)
{
    PlatformAddEntry = Memory->PlatformAddEntry;
    PlatformCompleteAllWork = Memory->PlatformCompleteAllWork;
    
    game_state* GameState = (game_state*)Memory->Base;
    
    char* Filename = __FILE__;
    
    GameState->HighPriorityQueue = Memory->HighPriorityQueue;
    GameState->LowPriorityQueue = Memory->LowPriorityQueue;
    
    u64 TransientStorageSize = Megabytes(64);
    GameState->GameArena = MakeArena((void*)((u8*)Memory->Base + sizeof(game_state)), Memory->Size - sizeof(game_state));
    GameState->FrameArena = NestArena(&GameState->GameArena, TransientStorageSize);
    GameState->AttackRecords = NestArena(&GameState->GameArena, Kilobytes(64));
    GameState->ProjectileRecords = NestArena(&GameState->GameArena, Kilobytes(64));
    GameState->MainThink.Buffer = NestArena(&GameState->GameArena, Kilobytes(64));
    GameState->MainThink.Thinkers = PushArray(&GameState->GameArena, think_entry, MAX_THINKERS);
    
    InitSpellInfo(&GameState->SpellInfo);
    GameState->ModifierCallbacks = PushArray(&GameState->GameArena, callback_lookup, ModifierType_Count);
    InitModifierCallbackLUT(GameState->ModifierCallbacks);
    
    for (u32 TaskIndex = 0;
         TaskIndex < ArrayCount(GameState->Tasks);
         ++TaskIndex)
    {
        task_with_memory* Task = &GameState->Tasks[TaskIndex];
        
        Task->BeingUsed = false;
        Task->Arena = NestArena(&GameState->GameArena, Megabytes(1));
    }
    
    GameState->Assets.GameState = GameState;
    GameState->Assets.Arena = NestArena(&GameState->GameArena, Megabytes(64));
    GameState->Assets.ReadEntireFile = Memory->DEBUGPlatformReadEntireFile;
    //LoadAsset(&GameState->Assets, GAI_Hero);
    
    GameState->Assets.Bitmaps[GAI_SphereDiffuse].Bitmap = PushStruct(&GameState->GameArena, bitmap);
    *GetBitmap(&GameState->Assets, GAI_SphereDiffuse) = MakeEmptyBitmap(&GameState->GameArena, 100, 100, false);
    MakeSphereDiffuseMap(GetBitmap(&GameState->Assets, GAI_SphereDiffuse), FV4(0.0f, 0.0f, 0.0f, 1.0f));
    GameState->Assets.Bitmaps[GAI_SphereDiffuse].State = AssetState_Loaded;
    
    GameState->Assets.Bitmaps[GAI_SphereNormal].Bitmap = PushStruct(&GameState->GameArena, bitmap);
    *GetBitmap(&GameState->Assets, GAI_SphereNormal) = MakeEmptyBitmap(&GameState->GameArena, 100, 100, false);
    MakeSphereNormalMap(GetBitmap(&GameState->Assets, GAI_SphereNormal), 0.0f);
    GameState->Assets.Bitmaps[GAI_SphereNormal].State = AssetState_Loaded;
    
    GameState->Camera.Pos = {-8.0f, -4.5f};
    GameState->Camera.Dim = {16.0f, 9.0f};
    GameState->MainRender.Camera = &GameState->Camera;
    GameState->MainRender.Buffer = NestArena(&GameState->GameArena, Megabytes(4));
    GameState->MainRender.Assets = &GameState->Assets;
    
    GameState->EnvMapWidth = 512;
    GameState->EnvMapHeight= 256;
    
    for (u32 MapIndex = 0;
         MapIndex < ArrayCount(GameState->EnvMaps);
         ++MapIndex)
    {
        environment_map* Map = GameState->EnvMaps + MapIndex;
        u32 Width = GameState->EnvMapWidth;
        u32 Height = GameState->EnvMapHeight;
        for (u32 LODIndex = 0;
             LODIndex < ArrayCount(Map->LOD);
             ++LODIndex)
        {
            Map->LOD[LODIndex] = MakeEmptyBitmap(&GameState->GameArena, Width, Height, false);
            Width >>= 1;
            Height >>= 1;
        }
    }
    
    LoadAsset(&GameState->Assets, GAI_Hero);
    bitmap* HeroBitmap = GetBitmap(&GameState->Assets, GAI_Hero);
    
    entity_result EntityResult = SpawnEntity(GameState);
    GameState->Hero = EntityResult.EntityID; 
    entity* HeroEntity = EntityResult.EntityPtr;
    HeroEntity->Pos = {0.0f, 0.0f};
    HeroEntity->MoveSpeed = 10.0f;
    HeroEntity->Bitmap = HeroBitmap;
    HeroEntity->Attributes.Health = 10;
    HeroEntity->Attributes.MaxHealth = 20;
    HeroEntity->IsRanged = true;
    HeroEntity->AttackProjectileSpeed = 10;
    HeroEntity->AttackFrontswing = 0.5f;
    HeroEntity->AttackBackswing = 0.5f;
    HeroEntity->Attributes.BaseAttackDamage = 5.0f;
    
    entity* Enemy = SpawnEntity(GameState).EntityPtr;
    Enemy->Pos = {2.0f, 2.0f};
    Enemy->Bitmap = HeroEntity->Bitmap;
    Enemy->Team = 1;
    Enemy->Attributes.Health = 20;
    Enemy->Attributes.MaxHealth = 20;
    
    GameState->UIContext.Hot = 0;
    GameState->UIContext.Active = 0;
    
    // test
    // TODO: #define this?
    //RegisterListener(GameState->EventHandler.OnAttackListeners, GameState->EventHandler.OnAttackListenerCount, AlchemistOnAttack);
}

#if RPG_INTERNAL
game_memory* DebugGlobalMemory;
#endif
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{   
    PlatformAddEntry = Memory->PlatformAddEntry;
    PlatformCompleteAllWork = Memory->PlatformCompleteAllWork;
    
#if RPG_INTERNAL
    DebugGlobalMemory = Memory;
#endif
    BEGIN_TIMED_BLOCK(GameUpdateAndRender);
    
    
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
    
    sv2Rectangle DrawBufferDimensions = GetBitmapDimensions(DrawBuffer);
    DrawRectangle(DrawBuffer, DrawBufferDimensions, FV4(0.5f,0.5f,0.5f,0), DrawBufferDimensions, true);
    DrawRectangle(DrawBuffer, DrawBufferDimensions, FV4(0.5f,0.5f,0.5f,0), DrawBufferDimensions, false);
    
    GameState->Time += Input->dtForFrame;
    real32 Angle = GameState->Time;
    real32 Disp = 2*Cos(Angle);
    real32 Disp_2 = 2*Cos(Angle + 3.14f*0.5f);
    
    camera* MainCamera = &GameState->Camera;
    ui_context* UIContext = &GameState->UIContext;
    
    UIContext->MousePos = UnNormalizeScreenSpace(MainCamera, Input->MousePos);
    
    
    game_controller_input* PlayerOneController = GetController(Input, 0);
    
    // camera grip
    if (IsDown(PlayerOneController->CameraGrip))
    {
        if (!WasPressed(PlayerOneController->CameraGrip))
        {
            fv2 DeltaMousePos = UIContext->MousePos - UIContext->LastMousePos;
            
            MainCamera->Pos -= DeltaMousePos;
        }
        UIContext->LastMousePos = UIContext->MousePos;
    }
    
#if 0
    render_entry_quad* CurrentMouse = PushRenderOnScreenQuad(&GameState->MainRender, NormalizeScreenSpace(MainCamera, UIContext->MousePos), FV2(0.02f,0), FV2(0,0.02f));
    CurrentMouse->Color = FV4(1,0,0,0.5f);
    
    render_entry_quad* LastMouse = PushRenderOnScreenQuad(&GameState->MainRender, NormalizeScreenSpace(MainCamera, UIContext->LastMousePos), FV2(0.02f,0), FV2(0,0.02f));
    LastMouse->Color = FV4(0,0,1,0.5f);
#endif
    
    // place obstacles
    if (WasPressed(PlayerOneController->Select))
    {
        fv3 IsoMousePos = WorldToIso(ProjToWorld(UIContext->MousePos + MainCamera->Pos));
        sv2 TilePosition = GetTileAtPosition(IsoMousePos);
        
        s32 ObstacleFoundAt = FindObstacle(TilePosition, GameState->Obstacles, GameState->ObstacleCount);
        if (ObstacleFoundAt >= 0)
        {
            for (s32 ObstacleIter = ObstacleFoundAt;
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
            sv2* NewObstacle = GameState->Obstacles + GameState->ObstacleCount%MAX_OBSTACLES;
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
        fv3 IsoMousePos = WorldToIso(ProjToWorld(UIContext->MousePos + MainCamera->Pos));
        
        HeroEntity->MoveTarget = GetTileAtPosition(IsoMousePos);
        
        ArenaReset(&HeroEntity->PathArena); // re-generate path
        if (HeroEntity->MoveThink == 0)
        {
            HeroEntity->MoveThink = SetEntityThink(GameState, GameState->Hero, &MoveEntity, 0);
        }
    }
    
    PushBufferToThinkers(&GameState->MainThink);
    ThinkAll(GameState, &GameState->MainThink, Input->dtForFrame);
    
#if 1
    // Draw entities here
    for (u32 AliveIndex = 0;
         AliveIndex <= MAX_ALIVE;
         ++AliveIndex)
    {
        entity_id EntityID = GameState->AliveEntities[AliveIndex];
        entity* Entity = GetEntity(GameState, EntityID);
        if (Entity)
        {
            bitmap* EntityBitmap = GetBitmap(&GameState->Assets, GAI_Hero);
            
            GameState->MainRender.WorldOrigin = Entity->Pos;
            fv2 XAxis = FV2(2,0);
            fv2 YAxis = FV2(0,2);
            fv3 HeroAlign = CalculateBitmapAlign(XAxis, YAxis, EntityBitmap);
            GameState->MainRender.WorldOrigin += HeroAlign;
            render_entry_quad* EntityQuad = PushRenderQuad(&GameState->MainRender, GAI_Hero, FV3(0,0,0), 
                                                           XAxis, YAxis);
            if (EntityQuad)
            {
                EntityQuad->Color = FV4(1.0f, 0.1f, 1.0f, 1.0f);
            }
            
            render_entry_process_ui* UIEntry = PushProcessUI(&GameState->MainRender, FV3(0,0,0),
                                                             FV2(2,0), FV2(0,2),
                                                             &GameState->UIContext, EntityID);
            UIEntry->Hitmap = EntityBitmap;
            
            // Draw Healthbar
            f32 HealthBarPercent = Entity->Attributes.Health / Entity->Attributes.MaxHealth;
            
            f32 HealthBarOffset = 1.1f;
            fv3 HealthBarAlign = fv2tofv3(HealthBarOffset * YAxis, 0);
            GameState->MainRender.WorldOrigin += HealthBarAlign;
            
            render_entry_quad* HealthBarBack = PushRenderQuad(&GameState->MainRender, FV3(0,0,0), FV2(2,0), FV2(0,0.5));
            HealthBarBack->Color = FV4(0.75f,0.75f,0.75f,1);
            
            render_entry_quad* HealthBarFill = PushRenderQuad(&GameState->MainRender, FV3(0,0.05,0),HealthBarPercent*FV2(2,0), FV2(0,0.4));
            HealthBarFill->Color = FV4(1,0,0,1);
            
            
            
        }
    }
    GameState->MainRender.WorldOrigin = {0,0,0};
    
    
    for (int ObstacleIndex = 0;
         ObstacleIndex < GameState->ObstacleCount;
         ++ObstacleIndex)
    {
        fv3 ObstaclePos = {};
        ObstaclePos.xy = sv2tofv2(GameState->Obstacles[ObstacleIndex]);
        ObstaclePos = IsoToWorld(ObstaclePos);
        
        render_entry_quad* QuadEntry = PushRenderQuad(&GameState->MainRender, ObstaclePos, 
                                                      ISO_X_AXIS, ISO_Y_AXIS);
        QuadEntry->Color = FV4(0.5f, 0.5f, 0.5f, 1.0f);
        QuadEntry->Texture = GetBitmap(&GameState->Assets, GAI_SphereDiffuse);
    }
#endif
    
#if 0
    render_entry_quad* QuadEntry = PushRenderQuad(&GameState->MainRender, IsoToWorld(fv2tofv3(sv2tofv2(HeroEntity->MoveTarget),0)), FV2(1,0), FV2(0,1));
    QuadEntry->Color = FV4(1,1,1,1);
#endif
    
    GameState->UIContext.Hot = 0;
    
    fv3 MapColor[3] = 
    {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
    };
    s32 CheckerWidth= 16;
    s32 CheckerHeight = 16;
    
    for (u32 MapIndex = 0;
         MapIndex < ArrayCount(GameState->EnvMaps);
         ++MapIndex)
    {
        environment_map* Map = GameState->EnvMaps + MapIndex;
        bitmap* LOD = Map->LOD + 0;
        b32 RowCheckerOn = false;
        for (s32 Y = 0;
             Y < LOD->Height;
             Y += CheckerHeight)
        {
            b32 CheckerOn = RowCheckerOn;
            for (s32 X = 0;
                 X < LOD->Width;
                 X += CheckerWidth)
            {
                fv4 Color = CheckerOn ? fv3tofv4(MapColor[MapIndex], 1.0f) : FV4(0.0f, 0.0f, 0.0f, 1.0f);
                sv2 MinP = SV2(X, Y);
                sv2 MaxP = MinP + SV2(CheckerWidth, CheckerHeight);
                DEBUGDrawRectangle(LOD, MinP, MaxP, Color);
                CheckerOn = !CheckerOn;
            }
            RowCheckerOn = !RowCheckerOn;
        }
    }
    
    // NOTE: surface normal is in projected coordinates for now
    // TODO: change when finalizing coordinate systems
    // Sphere Normal map test
    render_entry_quad* SphereEntry = PushRenderQuad(&GameState->MainRender, FV3(Disp, Disp_2, 0.0f), FV2(2, 0), FV2(0, 2));
    SphereEntry->Color = FV4(1.0f, 1.0f, 1.0f, 1.0);
    SphereEntry->Texture = GetBitmap(&GameState->Assets, GAI_SphereDiffuse);
    SphereEntry->NormalMap = GetBitmap(&GameState->Assets, GAI_SphereNormal);
    SphereEntry->SurfaceNormal = FV3(0.0f, 1.0f, 0.0f);
    SphereEntry->SurfaceUp = FV3(0.0f, 0.0f, 1.0f);
    SphereEntry->Top = GameState->EnvMaps + 2;
    SphereEntry->Middle = GameState->EnvMaps + 1;
    SphereEntry->Bottom = GameState->EnvMaps + 0;
    
#if 0
    fv2 MapP = FV2(0,0);
    
    for (u32 MapIndex = 0;
         MapIndex < ArrayCount(GameState->EnvMaps);
         ++MapIndex)
    {
        environment_map* Map = GameState->EnvMaps + MapIndex;
        bitmap* LOD = Map->LOD + 0;
        fv2 XAxis = 0.5f* FV2((f32)LOD->Width / (f32)Buffer->Width, 0);
        fv2 YAxis = 0.5f* FV2(0, (f32)LOD->Height / (f32)Buffer->Height);
        
        render_entry_quad* DebugEnvironmentMap = PushRenderOnScreenQuad(&GameState->MainRender, MapP, XAxis, YAxis);
        DebugEnvironmentMap->Color = FV4(1.0f, 1.0f, 1.0f, 1.0f);
        DebugEnvironmentMap->Texture = LOD;
        
        MapP += YAxis*1.05f;
    }
#endif
    
    ResolveRenderBuffer(GameState->HighPriorityQueue, DrawBuffer, &GameState->MainRender);
    
    if (WasPressed(PlayerOneController->Attack))
    {
        if (GameState->UIContext.Hot)
        {
            entity* HotEntity = GetEntity(GameState, GameState->UIContext.Hot);
            if (HotEntity->Team != HeroEntity->Team)
            {
                BeginAttack(GameState, HeroEntity, HotEntity, 0);
            }
        }
        else
        {
            fv3 IsoMousePos = WorldToIso(ProjToWorld(UIContext->MousePos + MainCamera->Pos));
            // attack move to IsoMousePos
        }
    }
    
    END_TIMED_BLOCK(GameUpdateAndRender);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state* GameState = (game_state*)Memory->Base;
    //GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}

//test

