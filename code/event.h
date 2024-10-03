/* date = June 5th 2024 8:19 pm */

#ifndef EVENT_H
#define EVENT_H

// Header
enum event_type
{
    EVENT_ON_CAST,
    EVENT_ON_ATTACK
};

struct event_header
{
    event_type Type;
};

// Events 
struct event_on_cast
{
    event_header Header;
};

struct event_on_attack
{
    event_header Header;
    
    entity_id Attacker;
    entity_id Target;
};

// Event Handler

#define EVENT_LISTENER(name) void name(event_header*)
typedef EVENT_LISTENER(EventListener);

#define MAX_LISTENERS 100
struct event_handler
{
    arena Buffer;
    
    // TODO: growable?
    EventListener* OnAttackListeners[MAX_LISTENERS];
    u32 OnAttackListenerCount;
};

#endif //EVENT_H
