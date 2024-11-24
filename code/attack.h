/* date = October 13th 2024 9:00 pm */

#ifndef ATTACK_H
#define ATTACK_H

enum damage_type
{
    DAMAGE_TYPE_PHYSICAL,
    DAMAGE_TYPE_MAGICAL,
    DAMAGE_TYPE_PURE
};

struct attack_record
{
    entity_id Attacker;
    entity_id Target;
    
    f32 Damage;
};

struct damage_event
{
    f32 Damage;
    damage_type Type;
    struct entity* Attacker;
    entity* Target;
};




#endif //ATTACK_H
