/* date = October 13th 2024 1:31 am */

#ifndef GHOST_H
#define GHOST_H

/*
TODO:
find modifier of a specific type
add modifier of a specific type

*/


// -------------------------------------------------------------
// Spectral Dagger
// -------------------------------------------------------------


struct spell_instance_spectral_dagger
{
    spell_instance_header Header;
};

struct modifier_spectral_dagger_buff
{
    int Level;
};

struct modifier_spectral_dagger_debuff
{
    int Level;
};



internal void
CastSpectralDagger(game_state* GameState, entity_id Caster, entity_id Target, spell* Spell_)
{
    spell_instance_spectral_dagger* Spell = (spell_instance_spectral_dagger*)Spell_;
    //CreateTrackingProjectile(GameState, Caster, Target, Spell->SpellInfo->ProjectileSpeed, false, 0, (void*)Spell);
}

internal void
ModifierSpectralDaggerBuff_OnCreated(modifier_spectral_dagger_buff* Modifier, 
                                     spell_instance_spectral_dagger* Spell)
{
    //Modifier->Level = Spell->Level;
}

internal void
ModifierSpectralDaggerDebuff_OnCreated(modifier_spectral_dagger_buff* Modifier, 
                                       spell_instance_spectral_dagger* Spell)
{
    //Modifier->Level = Spell->Level;
}

#if 0
internal void
GetModifierFlags(arena ModifierArena)
{
    while (NextSpellInfo)
    {
        // TODO: how the fuck do I do this?
        if (NextSpellInfo->BuffModifierFlags)
        {
        }
    }
}
#endif

// -------------------------------------------------------------
// Shriek
// -------------------------------------------------------------

struct spell_instance_shriek
{
    int Level;
};

internal void CastShriek(entity* Caster, spell_instance_shriek* Spell)
{
    
}

// -------------------------------------------------------------
// Spectral Form
// -------------------------------------------------------------

struct modifier_spectral_form
{
};

struct modifier_spectral_form_damage_record
{
    damage_type Type;
    f32 Damage;
};

struct spell_instance_spectral_form
{
    modifier_spectral_form* PassiveModifier;
    int Level;
};

internal MODIFIER_EVENT(Dispersion, OnTakeDamage)
{
    spell_instance_spectral_form* Spell = (spell_instance_spectral_form*)Spell_;
    modifier_spectral_form* Modifier = (modifier_spectral_form*)Modifier_;
    modifier_event_damage* DamageEvent = (modifier_event_damage*)Event_;
    
    // DamageEvent->Damage *= (1.0f - Spell->SpellInfo->DamageReduction[Spell->Level]);
}

// -------------------------------------------------------------
// Haunt
// -------------------------------------------------------------

struct spell_info_haunt
{
    spell_target_type TargetType;
    UnitTargetSpell* Cast;
    f32 IlluHealthMult[3];
    f32 IlluDamagePct;
    f32 Duration[3];
    modifier_event_listener* OnDeath;
};

struct spell_instance_haunt
{
    spell_info_haunt* SpellInfo;
    int Level;
};

struct modifier_haunt_illusion_illu
{
    entity_id HauntIllusion;
};

struct modifier_haunt_illusion_backpointer
{
    entity_id Hero;
    //ModifierEventOnDeath
};


internal void 
CastHaunt(game_state* GameState, entity_id Caster, entity_id Target, spell* Spell)
{
    //CreateIllusion(game_state* GameState, entity* Original, fv3 WorldPosition);
}

struct spell_instance_torment;

struct spell_info_torment
{
    spell_target_type TargetType;
    void (*Cast)(game_state*, entity_id, fv3, spell_instance_torment*);
    f32 CastFrontswing;
    f32 CastBackswing;
};

struct spell_instance_torment
{
    spell_info_torment* SpellInfo;
    int Level;
};

internal void 
CastTorment(game_state* GameState, entity_id Caster, fv3 Target, spell_instance_torment* Spell)
{
    // TODO: get multiple modifiers
    //entity* Illusion = GetModifiers(Caster, "modifier_haunt_illusion_pointer"); 
}

enum meta_spell_instance_type
{
    MetaSpellInstance_spell_instance_spectral_dagger
};

struct meta_size_entry
{
    meta_spell_instance_type Type;
    size_t Size;
};
meta_size_entry GhostSpellInstanceSize[]= 
{
    { MetaSpellInstance_spell_instance_spectral_dagger, sizeof(spell_instance_spectral_dagger) },
};

internal spell_instance_header*
AddSpellInstance_(entity* Entity, meta_spell_instance_type SpellInstanceType)
{
    arena* Arena = &Entity->SpellTable.SpellInstanceArena;
    // find struct that matches string
    // push the struct onto the arena then get a void*
    // pass void* to oncreated function which fills in spell instance starting values
    // get the corresponding spell_info
    // cast void* to header and fill in header values
    spell_instance_header* ret = PushStruct(Arena, spell_instance_header);
}
#define AddSpellInstance(Entity, SpellInstanceType) AddSpellInstance_(Entity, MetaSpellInstance_##SpellInstanceType)

internal void
InitGhost(game_state* GameState, entity* Hero)
{
    // create spell instances
    arena* SpellArena = &Hero->SpellTable.SpellInstanceArena;
    spell_instance_spectral_dagger* SpectralDaggerInst = PushZeroStruct(SpellArena, spell_instance_spectral_dagger);
    spell_instance_shriek* ShriekInst                  = PushZeroStruct(SpellArena, spell_instance_shriek);
    spell_instance_spectral_form* SpectralFormInst     = PushZeroStruct(SpellArena, spell_instance_spectral_form);
    spell_instance_haunt* HauntInst                    = PushZeroStruct(SpellArena, spell_instance_haunt);
    spell_instance_torment* TormentInst                = PushZeroStruct(SpellArena, spell_instance_torment);
    
#if 0
    modifier_spectral_form* SpectralFormPassiveModifier = AddModifier(Hero, modifier_spectral_form);
    SpectralFormInst->PassiveModifier = SpectralFormPassiveModifier;
#endif
    
    // put spell instances into correct slots
    Hero->SpellTable.Ability0 = (spell_instance_header*)SpectralDaggerInst;
    Hero->SpellTable.Ability1 = (spell_instance_header*)ShriekInst;
    Hero->SpellTable.Ability2 = (spell_instance_header*)SpectralFormInst;
    Hero->SpellTable.Ability3 = (spell_instance_header*)HauntInst;
    Hero->SpellTable.Ability4 = (spell_instance_header*)TormentInst;
    
}





#endif //GHOST_H
