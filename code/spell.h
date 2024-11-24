/* date = October 13th 2024 9:54 am */

#ifndef SPELL_H
#define SPELL_H

struct game_state;
struct spell;

#define NO_TARGET_CAST(name) void name(game_state* GameState, entity* Caster, spell* Spell)
typedef NO_TARGET_CAST(no_target_cast);

#define UNIT_TARGET_CAST(name) void name(game_state* GameState, entity* Caster, entity* Target, spell* Spell)
typedef UNIT_TARGET_CAST(unit_target_cast);

#define GROUND_TARGET_CAST(name) void name(game_state* GameState, entity* Caster, fv3 Target, spell* Spell)
typedef GROUND_TARGET_CAST(ground_target_cast);

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


enum dispel_type
{
    DISPEL_TYPE_WEAK,
    DISPEL_TYPE_STRONG,
    DISPEL_TYPE_UNIVERSAL
};


/* TODO: does the spell need to contain information specific to the spell?
targeting rules
targeting display
 modifier listeners OnLevelUp or OnSpellCast
*/

struct spell
{
    int Level;
    
    spell_target_type TargetType;
    union
    {
        no_target_cast* NoTargetCast;
        unit_target_cast* UnitTargetCast;
        ground_target_cast* GroundTargetCast;
    };
    
    ll_modifier_event_listener* OnLevelUp;
    ll_modifier_event_listener* OnCast;
};

struct spell_table
{
    //arena SpellInstanceArena;
    
    union
    {
        struct
        {
            spell Ability0;
            spell Ability1;
            spell Ability2;
            spell Ability3;
            spell Ability4;
            spell Ability5;
        };
        spell Abilities[6];
    };
    
    union
    {
        struct
        {
            spell Item0;
            spell Item1;
            spell Item2;
            spell Item3;
            spell Item4;
            spell Item5;
        };
        spell Inventory[6];
    };
    
    union
    {
        struct
        {
            spell Backpack0;
            spell Backpack1;
            spell Backpack2;
            spell Backpack3;
            spell Backpack4;
            spell Backpack5;
            spell Backpack6;
            spell Backpack7;
        };
        spell Backpack[8];
    };
};

#endif //SPELL_H
