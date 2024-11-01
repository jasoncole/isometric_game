/* date = June 5th 2024 8:19 pm */

#ifndef EVENT_H
#define EVENT_H


#if 0
// Global Events
enum global_event_type
{
    EVENT_ON_CAST,
    EVENT_ON_ATTACK
};

// TODO: what's the point of a header?
// are we ever going to process events of unknown type?
struct global_event_header
{
    global_event_type Type;
};

#define GLOBAL_EVENT_LISTENER(name) void name(global_event_header* Header)
typedef GLOBAL_EVENT_LISTENER(GlobalEventListener);


#define MAX_LISTENERS 100
struct global_event_handler
{
    arena Buffer;
    
    // TODO: growable?
    GlobalEventListener* GlobalEvents[MAX_LISTENERS];
    u32 OnAttackListenerCount;
};

#endif



#endif //EVENT_H
