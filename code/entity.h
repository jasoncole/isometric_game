/* date = June 12th 2024 0:33 pm */

#ifndef ENTITY_H
#define ENTITY_H

typedef u32 entity_id;


// TODO: handle different entity types
struct entity
{
    fv3 Pos;
    f32 MoveSpeed;
    
    b32 IsMoving;
    sv2 MoveTarget;
    entity_id EntityMoveTarget;
    arena PathArena;
    arena ModifierArena;
    
    bitmap Bitmap;
    arena Modifiers;
};


#endif //ENTITY_H
