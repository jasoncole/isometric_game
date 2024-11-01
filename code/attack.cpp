
internal projectile_record*
CreateTrackingProjectile(game_state* GameState, entity_id Source, entity_id Target, f32 ProjectileSpeed, b32 IsAttack, bitmap* Texture, void* ProjectileInfo, projectile_think* Think = 0)
{
    entity* Sender = GetEntity(GameState, Source);
    // TODO: circular buffer
    projectile_record* Result = PushStruct(&GameState->ProjectileRecords, projectile_record);
    
    Result->Position = Sender->Pos;
    Result->Speed = ProjectileSpeed;
    Result->Type = PROJECTILE_TYPE_TRACKING;
    Result->Source = Source;
    Result->Target = Target;
    Result->Team = Sender->Team;
    Result->IsAttack = IsAttack;
    Result->Texture = Texture;
    if (IsAttack)
    {
        Result->AttackRecord = (attack_record*)ProjectileInfo;
    }
    else
    {
        Result->Spell = (spell*)ProjectileInfo;
    }
    
    if (Think == 0)
    {
        Think = &GenericTrackingProjectile;
    }
    SetProjectileThink(GameState, Result, Think);
    
    return Result;
}

internal projectile_record*
CreateLinearProjectile(game_state* GameState, entity_id Source, f32 ProjectileSpeed, bitmap* Texture, spell* SpellInstance, projectile_think* Think = 0)
{
    entity* Sender = GetEntity(GameState, Source);
    // TODO: circular buffer
    projectile_record* Result = PushStruct(&GameState->ProjectileRecords, projectile_record);
    
    Result->Position = Sender->Pos;
    Result->Speed = ProjectileSpeed;
    Result->Type = PROJECTILE_TYPE_LINEAR;
    Result->Source = Source;
    Result->Team = Sender->Team;
    Result->Texture = Texture;
    Result->IsAttack = false;
    Result->Spell = SpellInstance;
    if (Think == 0)
    {
        Think = &GenericLinearProjectile;
    }
    SetProjectileThink(GameState, Result, Think);
    
    return Result;
}

// NOTE: projectile records will be in a circular buffer so don't need to free the memory
// just need to stop the projectile from updating
internal void
DestroyProjectileRecord(game_state* GameState, projectile_record* ProjectileRecord)
{
    RemoveThinker(&GameState->MainThink, ProjectileRecord->ThinkEntry);
}

internal attack_record*
CreateAttackRecord(game_state* GameState, entity_id Attacker, entity_id Target, f32 Damage)
{
    // TODO: circular arenas?
    attack_record* Result = PushStruct(&GameState->AttackRecords, attack_record);
    Result->Damage = Damage;
    Result->Attacker = Attacker;
    Result->Target = Target;
    
    return Result; 
}

internal f32
RollAttackDamage(entity* Attacker)
{
    f32 Variance = RandomBetween(&Attacker->Attributes.AttackVarianceSeed,
                                 -Attacker->Attributes.AttackVariance,
                                 Attacker->Attributes.AttackVariance);
    f32 Result = Attacker->Attributes.BaseAttackDamage + Attacker->Attributes.BonusAttackDamage + Variance;
    return Result;
};

internal void 
ApplyDamage(f32 Damage, damage_type DamageType, entity* Attacker, entity* Target)
{
    modifier_event_damage DamageEvent;
    
    DamageEvent.Damage = Damage;
    DamageEvent.Type = DamageType;
    DamageEvent.Attacker = Attacker;
    DamageEvent.Target = Target;
    
    if (DamageEvent.Type == DAMAGE_TYPE_MAGICAL)
    {
        DamageEvent.Damage *= (1.0f + Attacker->Attributes.SpellAmp);
        DamageEvent.Damage *= (1.0f - Target->Attributes.MagicResist);
    }
    else if (DamageEvent.Type == DAMAGE_TYPE_PHYSICAL)
    {
#define ARMOR_POWER 0.01f
        f32 ArmorAmount = Target->Attributes.Armor;
        f32 ArmorMult = 1 - ((ARMOR_POWER * ArmorAmount) / (1 + ARMOR_POWER * ABS(ArmorAmount)));
        DamageEvent.Damage *= ArmorMult;
    }
    
    // trigger on take damage events
    TriggerModifierListeners(Target->OnTakeDamage, (event*)&DamageEvent);
    
    Target->Attributes.Health -= DamageEvent.Damage;
}

internal void
PerformAttack(game_state* GameState, attack_record* AttackRecord)
{
    modifier_event_attack AttackEvent;
    AttackEvent.AttackRecord = AttackRecord;
    
    // do on hit stuff here
    entity* Attacker = GetEntity(GameState, AttackRecord->Attacker);
    TriggerModifierListeners(Attacker->OnHit, (event*)&AttackEvent);
    
    ApplyDamage(AttackRecord->Damage, DAMAGE_TYPE_PHYSICAL, Attacker, GetEntity(GameState, AttackRecord->Target));
}

internal void BeginAttack(game_state*, entity*, entity*, f32);


internal f32
EnableAttack(game_state* GameState, entity* Attacker, f32 dt)
{
    Attacker->AttackDisabled = false;
    if (Attacker->AttackAggroEntity)
    {
        BeginAttack(GameState, Attacker, GetEntity(GameState, Attacker->AttackAggroEntity), 0);
    }
    return -1;
}

internal f32
ResolveAttackPoint(game_state* GameState, attack_record* Attack, f32 dt)
{
    entity* Attacker = GetEntity(GameState, Attack->Attacker);
    if (Attacker->IsRanged)
    {
        CreateTrackingProjectile(GameState, Attack->Attacker, Attack->Target, Attacker->AttackProjectileSpeed, true, 0, (void*)Attack);
    }
    else
    {
        PerformAttack(GameState, Attack);
    }
    
    // PlayBackswingAnimation();
    Attacker->AttackDisabled = true;
    Attacker->PreAttackThink = 0;
    SetEntityThink(GameState, Attacker->ID, &EnableAttack, Attacker->AttackBackswing);
    
    return -1;
}

// TODO: any way of beginning the attack on the subtick time when the button was actually pressed?
internal void
BeginAttack(game_state* GameState, entity* Attacker, entity* Target, f32 dt) // dt is <= 0
{
    Attacker->AttackAggroEntity = Target->ID;
    
    if (Attacker->AttackDisabled)
    {
        return;
    }
    
    if (Attacker->PreAttackThink)
    {
        if (Attacker->PreAttackThink->AttackEntry.Attack->Target == Target->ID)
        {
            return;
        }
        RemoveThinker(&GameState->MainThink, Attacker->PreAttackThink);
    }
    f32 Damage = RollAttackDamage(Attacker);
    attack_record* AttackRecord  = CreateAttackRecord(GameState, Attacker->ID, Target->ID, Damage);
    Attacker->PreAttackThink = SetAttackThink(GameState, AttackRecord, &ResolveAttackPoint, Attacker->AttackFrontswing);
}

