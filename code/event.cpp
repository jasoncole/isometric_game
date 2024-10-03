
// TODO: meta-programming?

EVENT_LISTENER(AlchemistOnAttack)
{
    return;
}


internal void
RegisterListener(EventListener** ListenerArray, u32 Count, EventListener* ListenerFunc)
{
    ListenerArray[Count] = ListenerFunc;
}

inline void
ResolveListeners(EventListener** ListenerFuncs, u32 Count,
                 event_header* Header)
{
    for (u32 ListenerIndex = 0;
         ListenerIndex < Count;
         ++ListenerIndex)
    {
        (*ListenerFuncs[ListenerIndex])(Header);
    }
}

internal void
PushEventOnCast(event_handler* Handler)
{
    event_on_cast* NewEvent = PushStruct(&Handler->Buffer, event_on_cast);
    
    NewEvent->Header.Type = EVENT_ON_CAST;
}

internal void
PushEventOnAttack(event_handler* Handler, entity_id Attacker, entity_id Target)
{
    event_on_attack* NewEvent = PushStruct(&Handler->Buffer, event_on_attack);
    
    NewEvent->Header.Type = EVENT_ON_ATTACK;
    NewEvent->Attacker = Attacker;
    NewEvent->Target = Target;
    
}

internal void
ResolveEventBuffer(event_handler* Handler)
{
    arena* BufferArena = &Handler->Buffer;
    u64 BufferIndex = 0;
    
    while (BufferIndex < BufferArena->bytes_used)
    {
        event_header* Header = (event_header*)(BufferArena->base + BufferIndex);
        switch(Header->Type)
        {
            case EVENT_ON_CAST:
            {
                BufferIndex += sizeof(event_on_cast);
            } break;
            
            case EVENT_ON_ATTACK:
            {
                ResolveListeners(Handler->OnAttackListeners, Handler->OnAttackListenerCount, Header);
                BufferIndex += sizeof(event_on_attack);
            } break;
            
            InvalidDefaultCase;
        }
    }
    ArenaReset(BufferArena);
}

