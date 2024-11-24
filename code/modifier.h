/* date = September 28th 2024 7:59 pm */

#ifndef MODIFIER_H
#define MODIFIER_H

typedef u32 modifier_id;
typedef u32 entity_id;

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

struct entity;
struct modifier;
struct spell_info;
#define MODIFIER_EVENT_LISTENER(name) void name(spell_info* SpellInfo, modifier* Modifier, entity* Owner, event* Event_)
typedef MODIFIER_EVENT_LISTENER(modifier_event_listener);

#define MODIFIER_EVENT(m, m_event) internal MODIFIER_EVENT_LISTENER(m##__##m_event)

enum modifier_event_name
{
    ModifierEvent_OnCreated,
    ModifierEvent_OnDestroy,
    ModifierEvent_OnTakeDamage,
    ModifierEvent_OnCastSpell,
    ModifierEvent_OnAttackStart,
    ModifierEvent_OnHit,
    ModifierEvent_OnDeath,
    
    ModifierEvent_Count
};
struct modifier_event_callback
{
    modifier_event_name EventName;
    modifier_event_listener* ListenerFunc;
};
struct callback_lookup
{
    modifier_event_callback* Start;
    int Count;
};
inline callback_lookup InitCallbackLookup(modifier_event_callback* Start, int Count)
{
    callback_lookup Result;
    Result.Start = Start;
    Result.Count = Count;
    return Result;
}

struct ll_modifier_event_listener
{
    entity_id Owner;
    modifier_id ModifierID;
    modifier_event_listener* Listener;
    ll_modifier_event_listener* Prev;
    ll_modifier_event_listener* Next;
};




#endif //MODIFIER_H
