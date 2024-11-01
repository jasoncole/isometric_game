/* date = October 13th 2024 9:54 am */

#ifndef SPELL_H
#define SPELL_H

typedef void spell;
struct game_state;

#define NO_TARGET_CAST(name) void name(game_state* GameState, entity_id Caster, spell* Spell)
typedef NO_TARGET_CAST(NoTargetSpell);

#define UNIT_TARGET_CAST(name) void name(game_state* GameState, entity_id Caster, entity_id Target, spell* Spell)
typedef UNIT_TARGET_CAST(UnitTargetSpell);

#define GROUND_TARGET_CAST(name) void name(game_state* GameState, entity_id Caster, fv3 Target, spell* Spell)
typedef GROUND_TARGET_CAST(GroundTargetSpell);

enum spell_target_type
{
    TARGET_TYPE_NONE,
    TARGET_TYPE_UNIT,
    TARGET_TYPE_GROUND
};

enum spell_target_team
{
    TARGET_TEAM_ALL,
    TARGET_TEAM_ALLY,
    TARGET_TEAM_ENEMY
};

enum unit_type
{
    UNIT_TYPE_CREEP = 1 < 0,
    UNIT_TYPE_HERO = 1 < 1,
    UNIT_TYPE_BUILDING = 1 < 2,
    UNIT_TYPE_TREE = 1 < 3
};
#define UNIT_TYPE_BASIC (UNIT_TYPE_CREEP | UNIT_TYPE_HERO)

enum damage_type
{
    DAMAGE_TYPE_PHYSICAL,
    DAMAGE_TYPE_MAGICAL,
    DAMAGE_TYPE_PURE
};

enum dispel_type
{
    DISPEL_TYPE_WEAK,
    DISPEL_TYPE_STRONG,
    DISPEL_TYPE_UNDISPELLABLE
};



// NOTE: directly after spell_info_header, there is a struct with information specific to the individual spell
struct spell_info_header
{
    spell_target_type TargetType;
    union
    {
        NoTargetSpell* NoTargetCast;
        UnitTargetSpell* UnitTargetCast;
        GroundTargetSpell* GroundTargetCast;
    };
};

struct spell_instance_header
{
    spell_info_header* SpellInfo;
    int Level;
    // put pointers to any modifiers in the spell struct
};

struct spell_table
{
    arena SpellInstanceArena;
    
    union
    {
        struct
        {
            spell_instance_header* Ability0;
            spell_instance_header* Ability1;
            spell_instance_header* Ability2;
            spell_instance_header* Ability3;
            spell_instance_header* Ability4;
            spell_instance_header* Ability5;
        };
        spell_instance_header* Abilities[6];
    };
    
    union
    {
        struct
        {
            spell_instance_header* Item0;
            spell_instance_header* Item1;
            spell_instance_header* Item2;
            spell_instance_header* Item3;
            spell_instance_header* Item4;
            spell_instance_header* Item5;
        };
        spell_instance_header* Inventory[6];
    };
    
    union
    {
        struct
        {
            spell_instance_header* Backpack0;
            spell_instance_header* Backpack1;
            spell_instance_header* Backpack2;
            spell_instance_header* Backpack3;
            spell_instance_header* Backpack4;
            spell_instance_header* Backpack5;
            spell_instance_header* Backpack6;
            spell_instance_header* Backpack7;
        };
        spell_instance_header* Backpack[8];
    };
};

#endif //SPELL_H
