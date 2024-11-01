/* date = June 12th 2024 0:33 pm */

#ifndef ENTITY_H
#define ENTITY_H

typedef u32 entity_id;

// TODO: order independent metaprogramming
struct ll_modifier_event_listener;

// NOTE: at the start of each frame, calculate attribute struct for each entity
// for the remainder of the frame, attribute struct is read-only
struct attributes
{
    f32 Str;
    f32 Int;
    f32 Agi;
    
    f32 BaseAttackDamage;
    random_series AttackVarianceSeed;
    f32 AttackVariance; // add +/- AttackVariance
    f32 BonusAttackDamage;
    f32 BaseAttackTime;
    f32 AttackSpeed;
    
    f32 Health;
    f32 MaxHealth;
    f32 Mana;
    f32 MaxMana;
    f32 HealthRegen;
    f32 ManaRegen;
    f32 Armor;
    f32 MagicResist;
    
    f32 SpellAmp;
    f32 Evasion;
};


// TODO: isolate the information that is only allowed to change during the think pass
// stuff like world position
struct think_context
{
};

struct entity
{
    entity_id ID;
    
    fv3 Pos;
    f32 MoveSpeed;
    u32 Team;
    
    bitmap* Bitmap;
    
    attributes Attributes;
    b32 IsRanged;
    f32 AttackProjectileSpeed;
    f32 AttackFrontswing;
    f32 AttackBackswing;
    
    arena PathArena;
    spell_table SpellTable;
    arena ModifierListeners;
    
    ll_modifier_event_listener* OnTakeDamage;
    ll_modifier_event_listener* OnCastSpell;
    ll_modifier_event_listener* OnAttackStart;
    ll_modifier_event_listener* OnHit;
    
    b32 AttackDisabled;
    struct think_entry* PreAttackThink;
    entity_id AttackAggroEntity;
    
    think_entry* MoveThink;
    sv2 MoveTarget;
    entity_id EntityMoveTarget;
    
    entity* NextInHash;
};

#endif //ENTITY_H
