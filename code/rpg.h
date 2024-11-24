#if !defined(BOUNCER_H)

/* RPG_INTERNAL:
*   0 - Build for public release
*   1 - Build for developer only
* RPG_SLOW:
*   0 - no slow code allowed
*   1 - Slow code allowed
*/

#define introspect(params)
#define INCLUDE_DIRECTORY(directory)

#include "types.h"
#include "rpg_intrinsics.h"
#include "rpg_math.h"
#include "random.h"
#include "arena.cpp"
#include "render.h"
#include "attack.h"
#include "modifier.h"
#include "spell.h"
#include "entity.h"
#include "think.h"
#include "generated.h"
#include "rpg_platform.h"
#include "pathfinding.cpp"
#include "projectile.h"




INCLUDE_DIRECTORY(heroes)

struct ui_context
{
    entity_id Hot;
    entity_id Active;
    
    fv2 MousePos;
    fv2 LastMousePos;
};

enum asset_state
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded
};
struct asset
{
    asset_state State;
    bitmap* Bitmap;
};

enum game_asset_id
{
    GAI_SphereDiffuse,
    GAI_SphereNormal,
    GAI_Hero,
    GAI_Tree,
    
    GAI_Count
};

struct asset_tag
{
    u32 Tag;
    f32 Value;
};
struct asset_bitmap_info
{
    fv2 Align;
    
    s32 Width;
    s32 Height;
    s32 TagCount;
};


struct game_assets
{
    // TODO: remove
    game_state* GameState;
    
    arena Arena;
    debug_platform_read_entire_file* ReadEntireFile;
    asset Bitmaps[GAI_Count];
};

inline bitmap* GetBitmap(game_assets* Assets, game_asset_id ID)
{
    bitmap* Result = Assets->Bitmaps[ID].Bitmap;
    return Result;
}

struct task_with_memory
{
    bool32 BeingUsed;
    arena Arena;
    
    memory_index PopIndex;
};



struct game_state
{
    arena GameArena;
    arena TransientArena;
    arena FrameArena;
    platform_work_queue* HighPriorityQueue;
    platform_work_queue* LowPriorityQueue;
    
    task_with_memory Tasks[4];
    
    spell_info SpellInfo;
    arena AttackRecords;
    arena ProjectileRecords;
    
    f32 Time;
    
    //global_event_handler GlobalEventHandler;
    
    ui_context UIContext;
    
    camera Camera;
    render_group MainRender;
    think_group MainThink;
    
    // TODO: put this in world chunks, then simulate all entities in all world chunks
#define MAX_ALIVE 512
    u32 AliveCount;
    entity_id AliveEntities[MAX_ALIVE];
    
    u32 EntityIndex;
    entity Entities[512];
    
    // Modifier Event callbacks (defined at startup and does not change)
    callback_lookup* ModifierCallbacks;
    
#define MAX_OBSTACLES 512
    sv2 Obstacles[MAX_OBSTACLES];
    s32 ObstacleCount;
    
    entity_id Hero;
    
    s32 EnvMapWidth;
    s32 EnvMapHeight;
    // 0 is bottom, 2 is top
    environment_map EnvMaps[3];
    
    game_assets Assets;
};


global_variable platform_add_entry* PlatformAddEntry;
global_variable platform_complete_all_work* PlatformCompleteAllWork;

internal void LoadAsset(game_assets* Assets, game_asset_id ID);

#define BOUNCER_H
#endif
