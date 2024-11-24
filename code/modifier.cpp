


internal ll_modifier_event_listener*
RegisterModifierEventListener(arena* ListenerArena, ll_modifier_event_listener* Start,
                              modifier_id ModifierID, modifier_event_listener* ListenerFunc)
{
    // TODO: circular buffer
    ll_modifier_event_listener* ListenerNode = PushStruct(ListenerArena, ll_modifier_event_listener);
    Start->Prev = ListenerNode;
    ListenerNode->Next = Start;
    ListenerNode->ModifierID = ModifierID;
    ListenerNode->Listener = ListenerFunc;
    return ListenerNode;
}

// TODO: how to free up the memory?
internal void
RemoveModifierEventListener(ll_modifier_event_listener* Node)
{
    if (Node->Prev)
    {
        Node->Prev->Next = Node->Next;
    }
    if (Node->Next)
    {
        Node->Next->Prev = Node->Prev;
    }
}

internal modifier*
GetModifier(entity* Entity, modifier_id ModifierID)
{
    if (ModifierID == 0)
    {
        return 0;
    }
    u32 HashSlot = HashU32(ModifierID, Entity->ModifierHashTableSize);
    
    modifier* Modifier = (modifier*)Entity->ModifierArena.Base + HashSlot;
    do
    {
        if (Modifier->ID == ModifierID)
        {
            // NOTE: early return
            return Modifier;
        }
        Modifier = Modifier->NextInHash;
    } while (Entity);
    
    return 0;
}

// NOTE: this function may write to the event.
// make sure that each modifier value change is order-independent (multiplicative)
internal void
TriggerModifierListeners(game_state* GameState, ll_modifier_event_listener* First, event* Event)
{
    ll_modifier_event_listener* ListenerNode = First;
    while (ListenerNode)
    {
        entity* Entity = GetEntity(GameState, ListenerNode->Owner);
        modifier* Modifier = GetModifier(Entity, ListenerNode->ModifierID);
        if (Entity && Modifier)
        {
            ListenerNode->Listener(&GameState->SpellInfo, Modifier, Entity, Event);
            ListenerNode = ListenerNode->Next;
        }
        else
        {
            RemoveModifierEventListener(ListenerNode);
        }
    }
}


// NOTE: this function just reserves space for the modifier in memory
struct get_modifier_result
{
    modifier_id ModifierID;
    modifier* ModifierPtr;
};
internal get_modifier_result
CreateModifier(entity* Entity, modifier_type Type)
{
    Assert(Entity->ModifierIndex < U32Maximum);
    get_modifier_result Result;
    Result.ModifierID = ++Entity->ModifierIndex;
    u32 HashSlot = HashU32(Result.ModifierID, Entity->ModifierHashTableSize);
    
    modifier* Modifier = (modifier*)Entity->ModifierArena.Base + HashSlot;
    if (Modifier->ID)
    {
        while (Modifier->NextInHash)
        {
            Modifier = Modifier->NextInHash;
        }
        Modifier->NextInHash = PushStruct(&Entity->ModifierArena, modifier);
        Modifier = Modifier->NextInHash;
    }
    
    Modifier->Type = Type;
    Modifier->ID = Result.ModifierID;
    Result.ModifierPtr = Modifier;
    
    return Result;
}

inline callback_lookup
GetModifierCallbackLookup(game_state* GameState, modifier_type Type)
{
    callback_lookup Result = GameState->ModifierCallbacks[Type];
    return Result;
}

// NOTE: this function calls CreateModifier and runs modifier creation code
internal modifier*
AddModifier_(game_state* GameState, entity* Entity, modifier_type Type)
{
    get_modifier_result Result = CreateModifier(Entity, Type);
    
    //if (Result) TODO: handle not enough space for modifier?
    // probably not cause it should be a growable arena
    {
        callback_lookup ModifierLookup = GetModifierCallbackLookup(GameState, Type);
        for (int LookupIndex = 0;
             LookupIndex < ModifierLookup.Count;
             ++LookupIndex)
        {
            modifier_event_callback* EventCallback = ModifierLookup.Start + LookupIndex;
            // TODO: call OnCreated here?
            
            ll_modifier_event_listener* Root = GetModifierListenerRoot(Entity, EventCallback->EventName);
            RegisterModifierEventListener(&Entity->ModifierEventListenerArena, Root, Result.ModifierID, EventCallback->ListenerFunc);
        }
    }
    
    return Result.ModifierPtr;
}

enum modifier_type;
struct modifier;
internal modifier* AddModifier_(entity*, modifier_type);
#define AddModifier(e, m) AddModifier_((e), ModifierType_##m)
#define AddAndGetModifier(e, m) (m*)AddModifier_((e), ModifierType_##m)

inline void
DeleteModifier(entity* Entity, modifier_id ModifierID)
{
    modifier* Modifier = GetModifier(Entity, ModifierID);
    Modifier->ID = 0;
    // TODO: remove old modifiers from memory
}
