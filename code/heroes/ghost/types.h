/* date = November 20th 2024 0:07 am */

#ifndef BASE_H
#define BASE_H

// Spectral Form
struct modifier_spectral_form
{
    int Level;
};

struct modifier_spectral_form_damage_record
{
    damage_type Type;
    f32 Damage;
};

// Spectral Dagger
struct modifier_spectral_dagger_buff
{
    int Level;
};

struct modifier_spectral_dagger_debuff
{
    int Level;
};

// Haunt
struct modifier_haunt_illusion_illu
{
    entity_id HauntIllusion;
};

struct modifier_haunt_illusion_backpointer
{
    entity_id Hero;
    //ModifierEventOnDeath
};

struct modifier_test
{
    modifier_id Modifier;
};


#endif //BASE_H
