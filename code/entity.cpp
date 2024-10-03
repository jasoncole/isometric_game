
#define GetEntityIndex(EntityID) ((EntityID) - 1)
#define EntityIDCheckBounds(EntityID) Assert(((EntityID) > 0) && ((EntityID) <= MAX_ENTITIES))

// TODO: put these in a more reasonable place
#define MAX_NODES 100
#define PATH_ARENA_SIZE (sizeof(sv2)*MAX_NODES)

inline b32
EntityExists(game_state* GameState, entity_id EntitySlot)
{
    b32 ret = TestBit(GameState->EntityFreeList, EntitySlot - 1);
    return ret;
}

inline entity*
GetEntity(game_state* GameState, entity_id EntityID)
{
    entity* ret = &GameState->Entities[GetEntityIndex(EntityID)];
    return ret;
}

internal entity_id
CreateEntity(game_state* GameState)
{
    if (GameState->EntityCount < MAX_ENTITIES)
    {
        for (u32 EntityID = 1;
             EntityID <= MAX_ENTITIES;
             ++EntityID)
        {
            if (!EntityExists(GameState, EntityID))
            {
                GameState->EntityCount++;
                SetBit(GameState->EntityFreeList, GetEntityIndex(EntityID));
                
                entity* Entity = GetEntity(GameState, EntityID);
                ZeroStruct(*Entity);
                Entity->PathArena = MakeArena((void*)(GameState->PathArenas.base + (GetEntityIndex(EntityID) * PATH_ARENA_SIZE)), PATH_ARENA_SIZE);
                Entity->ModifierArena = MakeArena((void*)(GameState->ModifierArenas.base + (GetEntityIndex(EntityID) * MODIFIER_ARENA_SIZE)), MODIFIER_ARENA_SIZE);
                return EntityID;
            }
        }
    }
    return 0;
}

inline void
DeleteEntity(game_state* GameState, entity_id EntityID)
{
    EntityIDCheckBounds(EntityID);
    ClearBit(GameState->EntityFreeList, GetEntityIndex(EntityID));
    // TODO: zero entity?
    GameState->EntityCount--;
}

