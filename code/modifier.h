/* date = September 28th 2024 7:59 pm */

#ifndef MODIFIER_H
#define MODIFIER_H

#define MODIFIER_ARENA_SIZE Kilobytes(64)

#define MODIFIER_STATE_COUNT 5
enum unit_state
{
    MODIFIER_STATE_STUNNED = 1 << 0,
    MODIFIER_STATE_SILENCED = 1 << 1,
    MODIFIER_STATE_MUTED = 1 << 2,
    MODIFIER_STATE_MAGIC_IMMUNE = 1 << 3,
    MODIFIER_STATE_INVISIBLE = 1 << 4,
    MODIFIER_STATE_FREE_PATHING = 1 << 5
};

typedef void spell_instance;

// Modifier Events 
typedef void event;

struct modifier_event_cast
{
    entity_id Caster;
};

struct modifier_event_attack
{
    attack_record* AttackRecord;
};

/*
struct modifier_event_calculate_attributes
{
    attributes* Attributes;
};
*/


struct entity;

struct modifier_event_damage
{
    f32 Damage;
    damage_type Type;
    entity* Attacker;
    entity* Target;
};

#define MODIFIER_EVENT_LISTENER(name) void name(spell* Spell_, modifier* Modifier_, event* Event_)
typedef MODIFIER_EVENT_LISTENER(modifier_event_listener);

#define MODIFIER_EVENT(m, m_event) MODIFIER_EVENT_LISTENER(m##m_event)

struct ll_modifier_event_listener
{
    ll_modifier_event_listener* Prev;
    ll_modifier_event_listener* Next;
    spell* Spell;
    modifier* Modifier;
    modifier_event_listener* Listener;
};

// NOTE: this function may write to the event.
// make sure that each modifier value change is order-independent (multiplicative)
internal void
TriggerModifierListeners(ll_modifier_event_listener* First, event* Event)
{
    ll_modifier_event_listener* ListenerNode = First;
    while (ListenerNode)
    {
        ListenerNode->Listener(ListenerNode->Spell, ListenerNode->Modifier, Event);
        ListenerNode = ListenerNode->Next;
    }
}

#if 0
internal modifier*
AddModifier_(entity* Target)
{
    return 0;
}

#define AddModifier(Target, ModifierType) (ModifierType*)AddModifier_(Target)
#endif

#endif //MODIFIER_H
