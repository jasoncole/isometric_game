/* date = September 18th 2024 11:04 am */

#ifndef RENDER_H
#define RENDER_H

struct camera
{
    // TODO: change everything to proj?
    fv2 Pos; // NOTE: This is in proj space
    fv2 Dim;
    f32 inv_zoom; // multiply by dimension
};

struct bitmap
{
    fv2 Align;
    
    s32 Width;
    s32 Height;
    s32 Pitch;
    void* Memory;
};

struct environment_map
{
    bitmap LOD[4];
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
    fv3 SurfaceNormal;
    fv3 SurfaceUp;
    
    environment_map* Top;
    environment_map* Middle;
    environment_map* Bottom;
};

// TODO: move this?
struct ui_context;
typedef u32 entity_id;

struct render_entry_process_ui
{
    bitmap* Hitmap; // TODO: produce hitmaps from assets
    fv2 ProjPos;
    fv2 XAxis;
    fv2 YAxis;
    
    ui_context* UIContext;
    entity_id EntityID;
};

struct render_entry_grid
{
    fv4 Color;
};

struct render_group
{
    union 
    {
        fv3 WorldOrigin;
        fv2 NormScreenOrigin;
    };
    camera* Camera;
    arena Buffer;
    struct game_assets* Assets;
};

#endif //RENDER_H
