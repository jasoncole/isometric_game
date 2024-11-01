
// TODO: bug - thinkers are never getting removed so think group overflows


internal think_entry*
ReserveThinkEntry(think_group* ThinkGroup)
{
    for (u32 ThinkIndex = 0;
         ThinkIndex < MAX_THINKERS;
         ++ThinkIndex)
    {
        if (!TestBit(ThinkGroup->ThinkFreeList, ThinkIndex))
        {
            SetBit(ThinkGroup->ThinkFreeList, ThinkIndex);
            think_entry* ThinkEntry = ThinkGroup->Thinkers + ThinkIndex;
            return ThinkEntry;
        }
    }
    return 0;
};

internal think_entry*
SetEntityThink(game_state* GameState, entity_id EntityID, entity_think* Think, f32 Delay)
{
    think_entry* ThinkEntry = PushStruct(&GameState->MainThink.Buffer, think_entry);
    if (ThinkEntry)
    {
        ThinkEntry->Type = ThinkEntry_Entity;
        ThinkEntry->StartTime = GameState->Time;
        ThinkEntry->RemainingTime = Delay;
        ThinkEntry->EntityEntry.Think = Think;
        ThinkEntry->EntityEntry.EntityID = EntityID;
    }
    return ThinkEntry;
}

internal f32
GenericLinearProjectile(game_state* GameState, projectile_record* Projectile, f32 dt)
{
    return -1;
}

internal f32
GenericTrackingProjectile(game_state* GameState, projectile_record* Projectile, f32 dt)
{
    //entity* Source = GetEntity(GameState, Projectile->Source);
    entity* Target = GetEntity(GameState, Projectile->Target);
    fv3 Direction = fv3Normalize(Target->Pos - Projectile->Position);
    Projectile->Position += Direction*dt*Projectile->Speed;
    fv3 NewDirection = fv3Normalize(Target->Pos - Projectile->Position);
    
    if (fv3Dot(Direction, NewDirection) < 0) // projectile passed target
    {
        // draw impact animation
        if (Projectile->IsAttack)
        {
            PerformAttack(GameState, Projectile->AttackRecord);
        }
        return -1;
    }
    else
    {
        fv2 YAxis = WorldToProj(NewDirection);
        fv2 XAxis = fv2Perp(YAxis);
        render_entry_quad* QuadEntry = PushRenderQuad(&GameState->MainRender, Projectile->Position + CalculateAlign(XAxis, YAxis, FV2(0.5f, 0)), 
                                                      XAxis, YAxis);
        QuadEntry->Color = FV4(1,1,1,1);
        QuadEntry->Texture = Projectile->Texture;
    }
    return 0;
}

internal think_entry*
SetProjectileThink(game_state* GameState, projectile_record* Projectile, projectile_think* Think)
{
    think_entry* ThinkEntry = PushStruct(&GameState->MainThink.Buffer, think_entry);
    if (ThinkEntry)
    {
        ThinkEntry->Type = ThinkEntry_Projectile;
        ThinkEntry->StartTime = GameState->Time;
        ThinkEntry->RemainingTime = 0;
        ThinkEntry->ProjectileEntry.Projectile = Projectile;
        ThinkEntry->ProjectileEntry.Think = Think;
    }
    
    Projectile->ThinkEntry = ThinkEntry;
    return ThinkEntry;
}

internal think_entry*
SetAttackThink(game_state* GameState, attack_record* Attack, attack_think* Think, f32 Delay)
{
    think_entry* ThinkEntry = PushStruct(&GameState->MainThink.Buffer, think_entry);
    if (ThinkEntry)
    {
        ThinkEntry->Type = ThinkEntry_Attack;
        ThinkEntry->StartTime = GameState->Time;
        ThinkEntry->RemainingTime = Delay;
        ThinkEntry->AttackEntry.Think = Think;
        ThinkEntry->AttackEntry.Attack = Attack;
    }
    return ThinkEntry;
}

inline void
RemoveThinker(think_group* ThinkGroup, think_entry* Thinker)
{
    u32 ThinkID = (u32)(Thinker - ThinkGroup->Thinkers);
    ClearBit(ThinkGroup->ThinkFreeList, ThinkID);
}

internal void 
PushBufferToThinkers(think_group* ThinkGroup)
{
    u64 BufferIndex = 0;
    while(BufferIndex < ThinkGroup->Buffer.Used)
    {
        think_entry* BufferEntry = (think_entry*)(ThinkGroup->Buffer.Base + BufferIndex);
        think_entry* DestEntry = ReserveThinkEntry(ThinkGroup);
        if (DestEntry)
        {
            CopySize(sizeof(think_entry), (void*)BufferEntry, (void*)DestEntry);
        }
        else
        {
            InvalidCodePath;
        }
        BufferIndex += sizeof(think_entry);
    }
    ArenaReset(&ThinkGroup->Buffer);
}

internal void
ThinkAll(game_state* GameState, think_group* ThinkGroup, f32 dt)
{
    for (u32 ThinkIndex = 0;
         ThinkIndex < MAX_THINKERS;
         ++ThinkIndex)
    {
        if (TestBit(ThinkGroup->ThinkFreeList, ThinkIndex))
        {
            think_entry* Entry = ThinkGroup->Thinkers + ThinkIndex;
            Entry->RemainingTime -= dt;
            if (Entry->RemainingTime <= 0)
            {
                switch(Entry->Type)
                {
                    case ThinkEntry_Entity:
                    {
                        think_entry_entity* EntityEntry = &Entry->EntityEntry;
                        entity* Entity = GetEntity(GameState, EntityEntry->EntityID);
                        if (Entity)
                        {
                            Entry->RemainingTime = (*EntityEntry->Think)(GameState, Entity, dt);
                        }
                    } break;
                    case ThinkEntry_Modifier:
                    {
                    } break;
                    case ThinkEntry_Projectile:
                    {
                        think_entry_projectile* ProjectileEntry = &Entry->ProjectileEntry;
                        Entry->RemainingTime = (*ProjectileEntry->Think)(GameState, ProjectileEntry->Projectile, dt);
                    } break;
                    case ThinkEntry_Attack:
                    {
                        think_entry_attack* AttackEntry = &Entry->AttackEntry;
                        Entry->RemainingTime = (*AttackEntry->Think)(GameState, AttackEntry->Attack, dt);
                    } break;
                    InvalidDefaultCase;
                }
            }
            if (Entry->RemainingTime < 0)
            {
                RemoveThinker(ThinkGroup, Entry);
            }
        }
    }
}