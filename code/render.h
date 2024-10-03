/* date = September 18th 2024 11:04 am */

#ifndef RENDER_H
#define RENDER_H

struct camera
{
    fv2 Pos;
    fv2 Dim;
    f32 inv_zoom; // multiply by dimension
};

struct bitmap
{
    s32 Width;
    s32 Height;
    s32 Pitch;
    void* Memory;
};

struct environment_map
{
    bitmap* LOD[4];
};

enum render_entry_type
{
    RenderGroupEntry_Clear,
    RenderGroupEntry_Quad,
    RenderGroupEntry_ProcessUI,
    RenderGroupEntry_Grid,
};

struct render_entry_header
{
    render_entry_type Type;
};

struct render_entry_clear
{
    fv4 Color;
};

struct render_entry_quad
{
    fv2 ProjPos;
    fv2 XAxis;
    fv2 YAxis;
    
    fv4 Color;
    bitmap* Texture;
    bitmap* NormalMap;
    
    environment_map* Top;
    environment_map* Middle;
    environment_map* Bottom;
};

// TODO: move this?
struct ui_context;
typedef u32 entity_id;

struct render_entry_process_ui
{
    bitmap* Texture; // for alpha testing
    fv2 ProjPos;
    fv2 XAxis;
    fv2 YAxis;
    
    // TODO: move mouse pos to ui context?
    ui_context* UIContext;
    entity_id EntityID;
    fv2 MousePos;
};


struct render_entry_grid
{
    fv4 Color;
};


struct render_group
{
    f32 Z_to_Y;
    
    fv2 x_transform;
    fv2 y_transform;
    
    camera* Camera;
    arena Buffer;
};

#endif //RENDER_H
