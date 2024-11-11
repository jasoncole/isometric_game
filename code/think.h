/* date = October 18th 2024 0:15 am */

#ifndef THINK_H
#define THINK_H

struct think_entry;
enum projectile_type
{
    PROJECTILE_TYPE_LINEAR,
    PROJECTILE_TYPE_TRACKING
};

struct projectile_record
{
    fv3 Position;
    f32 Speed;
    projectile_type Type;
    b32 IsAttack;
    union
    {
        attack_record* AttackRecord;
        spell* Spell;
    };
    think_entry* ThinkEntry;
    
    bitmap* Texture;
    entity_id Source;
    entity_id Target;
    u32 Team;
};

struct modifier;

#define ENTITY_THINK(name) f32 name(game_state* GameState, entity* Entity, f32 dt)
typedef ENTITY_THINK(entity_think);
#define MODIFIER_THINK(name) f32 name(game_state* GameState, modifier* Modifier, f32 dt)
typedef MODIFIER_THINK(modifier_think);
#define PROJECTILE_THINK(name) f32 name(game_state* GameState, projectile_record* Projectile, f32 dt)
typedef PROJECTILE_THINK(projectile_think);
#define ATTACK_THINK(name) f32 name(game_state* GameState, attack_record* Attack, f32 dt)
typedef ATTACK_THINK(attack_think);

enum think_entry_type
{
    ThinkEntry_Entity,
    ThinkEntry_Modifier,
    ThinkEntry_Projectile,
    ThinkEntry_Attack
};

struct think_entry_entity
{
    entity_think* Think;
    entity_id EntityID;
};

struct think_entry_modifier
{
    modifier_think* Think;
    modifier* Modifier;
    // TODO: think about how to ensure that modifiers don't think after they are destroyed
};

struct think_entry_projectile
{
    projectile_think* Think;
    projectile_record* Projectile; 
    // TODO: think about how to ensure that projectiles don't think after they are destroyed
};

struct think_entry_attack
{
    attack_think* Think;
    attack_record* Attack;
};

// TODO: thinkers that loop
struct think_entry
{
    think_entry_type Type;
    f32 StartTime;
    f32 RemainingTime;
    union
    {
        think_entry_entity EntityEntry;
        think_entry_modifier ModifierEntry;
        think_entry_projectile ProjectileEntry;
        think_entry_attack AttackEntry;
    };
};


#define MAX_THINKERS 512

// TODO: should entities also have think groups?
struct think_group
{
    u32 ThinkFreeList[DIV32(MAX_THINKERS)];
    think_entry* Thinkers;
    
    arena Buffer;
};

// TODO: order independence
internal void PerformAttack(game_state*, attack_record*);

#endif //THINK_H
