/* date = November 19th 2024 11:07 pm */

// -------------------------------------------------------------
// Spectral Dagger
// -------------------------------------------------------------

GROUND_TARGET_CAST(CastSpectralDagger)
{
    ghost_spectral_dagger* SpellInfoSpectralDagger = &GameState->SpellInfo.GhostSpectralDagger;
    fv2 Direction = GetNormalizedProjDirection(Caster->Pos, Target);
    CreateLinearProjectile(GameState, Caster, Direction, SpellInfoSpectralDagger->ProjectileSpeed, 0, Spell, 0);
}

MODIFIER_EVENT(modifier_spectral_dagger_buff, OnCreated)
{
    //Modifier->Level = Spell->Header.Level;
}

MODIFIER_EVENT(modifier_spectral_dagger_debuff, OnCreated)
{
    //Modifier->Level = Spell->Header.Level;
}


// -------------------------------------------------------------
// Spectral Form
// -------------------------------------------------------------



// to make OnCreated function for modifiers:
// option 1: use metaprogramming to make a large switch statement that checks the type of the modifier and uses parse information to register listeners
// option 2: auto-generate an OnCreated function using parse information for each modifier


// when the modifier is created,
// register this function as listener to modifier owner's OnTakeDamage event
// when the event is fired, check if modifier still exists
// if modifier still exists, call the listener
// if modifier does not exist, remove the listener
MODIFIER_EVENT(modifier_spectral_form, OnTakeDamage)
{
    modifier_spectral_form* ModifierSpectralForm = &Modifier->ModifierSpectralForm;
    //modifier_event_damage* DamageEvent = (modifier_event_damage*)Event_;
    
    //DamageEvent->Damage *= (1.0f - SpellInfo->GhostDispersion.DamageReduction[Spell->Level]);
}

MODIFIER_EVENT(modifier_spectral_form, OnCreated)
{
    modifier_spectral_form* ModifierSpectralForm = &Modifier->ModifierSpectralForm;
    
}

// -------------------------------------------------------------
// Shriek
// -------------------------------------------------------------

NO_TARGET_CAST(CastShriek)
{
    
}

// -------------------------------------------------------------
// Haunt
// -------------------------------------------------------------

NO_TARGET_CAST(CastHaunt)
{
    //CreateIllusion(game_state* GameState, entity* Original, fv3 WorldPosition);
}

GROUND_TARGET_CAST(CastTorment)
{
    // TODO: get multiple modifiers
    //entity* Illusion = GetModifiers(Caster, "modifier_haunt_illusion_pointer"); 
}


internal void
InitGhost(game_state* GameState, entity* Hero)
{
    /*
    // create spell instances
    arena* SpellArena = &Hero->SpellTable.SpellInstanceArena;
    spell_instance_spectral_dagger* SpectralDaggerInst = PushZeroStruct(SpellArena, spell_instance_spectral_dagger);
    spell_instance_shriek* ShriekInst                  = PushZeroStruct(SpellArena, spell_instance_shriek);
    spell_instance_spectral_form* SpectralFormInst     = PushZeroStruct(SpellArena, spell_instance_spectral_form);
    spell_instance_haunt* HauntInst                    = PushZeroStruct(SpellArena, spell_instance_haunt);
    spell_instance_torment* TormentInst                = PushZeroStruct(SpellArena, spell_instance_torment);
    
    modifier_spectral_form* SpectralFormPassiveModifier = AddAndGetModifier(Hero, modifier_spectral_form);
    SpectralFormInst->PassiveModifier = SpectralFormPassiveModifier;
    
    // put spell instances into correct slots
    Hero->SpellTable.Ability0 = (spell_instance_header*)SpectralDaggerInst;
    Hero->SpellTable.Ability1 = (spell_instance_header*)ShriekInst;
    Hero->SpellTable.Ability2 = (spell_instance_header*)SpectralFormInst;
    Hero->SpellTable.Ability3 = (spell_instance_header*)HauntInst;
    Hero->SpellTable.Ability4 = (spell_instance_header*)TormentInst;
    */
}

