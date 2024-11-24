
// TODO: put these in a more reasonable place
#define MAX_NODES 100
#define PATH_ARENA_SIZE (sizeof(sv2)*MAX_NODES)

// NOTE: HashTableSize must be even
inline u32
HashU32(u32 ID, u32 HashTableSize)
{
    u32 HashValue = ID;
    HashValue = ((HashValue >> 0) & 0xFF) + 
    ((HashValue >> 8) & 0xFF) + 
    ((HashValue >> 16) & 0xFF) + 
    ((HashValue >> 24) & 0xFF);
    HashValue = HashValue * ID;
    u32 HashSlot = HashValue & (HashTableSize - 1);
    Assert(HashSlot < HashTableSize);
    
    return HashSlot;
}

internal entity*
GetEntity(game_state* GameState, entity_id EntityID)
{
    if (EntityID == 0)
    {
        return 0;
    }
    u32 HashSlot = HashU32(EntityID, ArrayCount(GameState->Entities));
    
    entity* Entity = &GameState->Entities[HashSlot];
    do
    {
        if (Entity->ID == EntityID)
        {
            return Entity;
        }
        Entity = Entity->NextInHash;
    } while (Entity);
    
    return 0;
}

struct entity_result
{
    entity_id EntityID;
    entity* EntityPtr;
};

internal entity_result
CreateEntity(game_state* GameState)
{
    Assert(GameState->EntityIndex < 0xFFFFFFFF);
    
    entity_result Result;
    Result.EntityID = ++GameState->EntityIndex;
    u32 HashSlot = HashU32(Result.EntityID, ArrayCount(GameState->Entities));
    
    entity* Entity = &GameState->Entities[HashSlot];
    if (Entity->ID)
    {
        while (Entity->NextInHash)
        {
            Entity = Entity->NextInHash;
        }
        Entity->NextInHash = PushStruct(&GameState->GameArena, entity);
        Entity = Entity->NextInHash;
    }
    Entity->ID = Result.EntityID;
    Entity->PathArena = NestArena(&GameState->GameArena, PATH_ARENA_SIZE);
    Result.EntityPtr = Entity;
    
    return Result;
}

inline entity_result
SpawnEntity(game_state* GameState)
{
    Assert(GameState->AliveCount< MAX_ALIVE);
    entity_result EntityResult = CreateEntity(GameState);
    GameState->AliveEntities[GameState->AliveCount++] = EntityResult.EntityID;
    return EntityResult;
}

internal void
KillEntity(game_state* GameState, entity_id EntityID)
{
    for (u32 AliveIndex = 0;
         AliveIndex < GameState->AliveCount;
         ++AliveIndex)
    {
        if (GameState->AliveEntities[AliveIndex] == EntityID)
        {
            --GameState->AliveCount;
            for (u32 CopyBack = AliveIndex;
                 AliveIndex < GameState->AliveCount;
                 ++CopyBack)
            {
                GameState->AliveEntities[CopyBack] = GameState->AliveEntities[CopyBack+1];
            }
            return;
        }
    }
}

inline void
DeleteEntity(game_state* GameState, entity_id EntityID)
{
    entity* Entity = GetEntity(GameState, EntityID);
    KillEntity(GameState, Entity->ID);
    Entity->ID = 0;
    // TODO: remove old entities from memory
}

inline ll_modifier_event_listener* GetModifierListenerRoot(entity* Entity, modifier_event_name EventType)
{
    ll_modifier_event_listener* Result = Entity->ModifierListenerRoots[EventType];
    return Result;
}


