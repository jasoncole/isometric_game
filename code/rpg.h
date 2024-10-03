#if !defined(BOUNCER_H)

/* RPG_INTERNAL:
*   0 - Build for public release
*   1 - Build for developer only
* RPG_SLOW:
*   0 - no slow code allowed
*   1 - Slow code allowed
*/

#include "types.h"
#include "rpg_math.h"
#include "rpg_intrinsics.h"
#include "random.h"
#include "arena.cpp"
#include "stb_truetype.h"
#include "render.h"
#include "modifier.h"
#include "entity.h"
#include "rpg_platform.h"
#include "pathfinding.cpp"
#include "event.h"

struct ui_context
{
    entity_id Hot;
    entity_id Active;
    
    fv2 LastMousePos;
};

struct game_state
{
    arena GameArena;
    arena FrameArena;
    arena PathArenas;
    arena ModifierArenas;
    
    ui_context UIContext;
    camera Camera;
    
    render_group MainRender;
    
    bitmap WhiteSquare;
    bitmap TestNormal;
    
#define MAX_ENTITIES 512
    u32 EntityFreeList[DIV32(MAX_ENTITIES)];
    u32 EntityCount;
    entity* Entities;
    
    entity_id Hero;
    
    sv2 Obstacles[MAX_ENTITIES];
    s32 ObstacleCount;
    
    event_handler EventHandler;
};

struct transient_state
{
    
};

#define BOUNCER_H
#endif
