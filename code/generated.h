#include "heroes\ghost\types.h"

struct ghost_spectral_dagger
{
    char* Name;
    int TargetType;
    int TargetTeam;
    int TargetUnitType;
    int DamageType;
    int DispelType;
    f32 ProjectileSpeed;
    f32 Damage[4];
    f32 ManaCost[4];
    f32 MoveSpeedChangePct[4];
    f32 Cooldown[4];
};

struct ghost_dispersion
{
    char* Name;
    f32 Duration;
    f32 DamageReduction[4];
};

struct spell_info
{
    ghost_spectral_dagger GhostSpectralDagger;
    ghost_dispersion GhostDispersion;
};

internal void
InitSpellInfo(spell_info* SpellInfo)
{
    SpellInfo->GhostSpectralDagger.Name = "Spectral Dagger";
    SpellInfo->GhostSpectralDagger.TargetType = TARGET_TYPE_UNIT;
    SpellInfo->GhostSpectralDagger.TargetTeam = TARGET_TEAM_ENEMY;
    SpellInfo->GhostSpectralDagger.TargetUnitType = UNIT_TYPE_BASIC;
    SpellInfo->GhostSpectralDagger.DamageType = DAMAGE_TYPE_MAGICAL;
    SpellInfo->GhostSpectralDagger.DispelType = DISPEL_TYPE_WEAK;
    SpellInfo->GhostSpectralDagger.ProjectileSpeed = 800.000000;
    SpellInfo->GhostSpectralDagger.Damage[0] = 70.000000;
    SpellInfo->GhostSpectralDagger.Damage[1] = 120.000000;
    SpellInfo->GhostSpectralDagger.Damage[2] = 170.000000;
    SpellInfo->GhostSpectralDagger.Damage[3] = 220.000000;
    SpellInfo->GhostSpectralDagger.ManaCost[0] = 70.000000;
    SpellInfo->GhostSpectralDagger.ManaCost[1] = 120.000000;
    SpellInfo->GhostSpectralDagger.ManaCost[2] = 170.000000;
    SpellInfo->GhostSpectralDagger.ManaCost[3] = 220.000000;
    SpellInfo->GhostSpectralDagger.MoveSpeedChangePct[0] = 70.000000;
    SpellInfo->GhostSpectralDagger.MoveSpeedChangePct[1] = 120.000000;
    SpellInfo->GhostSpectralDagger.MoveSpeedChangePct[2] = 170.000000;
    SpellInfo->GhostSpectralDagger.MoveSpeedChangePct[3] = 220.000000;
    SpellInfo->GhostSpectralDagger.Cooldown[0] = 70.000000;
    SpellInfo->GhostSpectralDagger.Cooldown[1] = 120.000000;
    SpellInfo->GhostSpectralDagger.Cooldown[2] = 170.000000;
    SpellInfo->GhostSpectralDagger.Cooldown[3] = 220.000000;
    SpellInfo->GhostDispersion.Name = "Dispersion";
    SpellInfo->GhostDispersion.Duration = 10.000000;
    SpellInfo->GhostDispersion.DamageReduction[0] = 5.000000;
    SpellInfo->GhostDispersion.DamageReduction[1] = 10.000000;
    SpellInfo->GhostDispersion.DamageReduction[2] = 15.000000;
    SpellInfo->GhostDispersion.DamageReduction[3] = 20.000000;
}

enum modifier_type
{
    ModifierType_modifier_spectral_form,
    ModifierType_modifier_spectral_form_damage_record,
    ModifierType_modifier_spectral_dagger_buff,
    ModifierType_modifier_spectral_dagger_debuff,
    ModifierType_modifier_haunt_illusion_illu,
    ModifierType_modifier_haunt_illusion_backpointer,
    ModifierType_modifier_test,
    
ModifierType_Count
};

struct modifier
{
    union
    {
        modifier_spectral_form ModifierSpectralForm;
        modifier_spectral_form_damage_record ModifierSpectralFormDamageRecord;
        modifier_spectral_dagger_buff ModifierSpectralDaggerBuff;
        modifier_spectral_dagger_debuff ModifierSpectralDaggerDebuff;
        modifier_haunt_illusion_illu ModifierHauntIllusionIllu;
        modifier_haunt_illusion_backpointer ModifierHauntIllusionBackpointer;
        modifier_test ModifierTest;
    };
    modifier_type Type;
    modifier_id ID;
    modifier* NextInHash;
};

        