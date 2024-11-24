#include "heroes\ghost\abilities.cpp"

modifier_event_callback CallbacksFor_modifier_spectral_dagger_buff[] = 
{
{ ModifierEvent_OnCreated, modifier_spectral_dagger_buff__OnCreated },
};

modifier_event_callback CallbacksFor_modifier_spectral_form[] = 
{
{ ModifierEvent_OnCreated, modifier_spectral_form__OnCreated },
};

internal void
InitModifierCallbackLUT(callback_lookup* ModifierCallbackLUT)
{
ModifierCallbackLUT[ModifierType_modifier_spectral_dagger_buff] = InitCallbackLookup(&CallbacksFor_modifier_spectral_dagger_buff[0], ArrayCount(CallbacksFor_modifier_spectral_dagger_buff));
ModifierCallbackLUT[ModifierType_modifier_spectral_form] = InitCallbackLookup(&CallbacksFor_modifier_spectral_form[0], ArrayCount(CallbacksFor_modifier_spectral_form));
}

