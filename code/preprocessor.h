/* date = October 26th 2024 9:59 pm */

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <windows.h>

typedef size_t memory_index;


#define Kilobytes(Value) ((Value)* 1024LL)
#define Megabytes(Value) (Kilobytes(Value)* 1024LL)
#define Gigabytes(Value) (Megabytes(Value)* 1024LL)
#define Terabytes(Value) (Gigabytes(Value)* 1024LL)



struct arena
{
    char* Base;
    memory_index Used;
    memory_index Size;
};

static arena Arena;

#define PushArray(type, count) (type *)PushSize(sizeof(type)*(count))
#define PushStruct(type) PushArray(type, 1)
#define ArenaTop (Arena.Base + Arena.Used)

static void*
PushSize(memory_index Bytes)
{
    void* Result = 0;
    if (Arena.Used + Bytes <= Arena.Size)
    {
        Result = Arena.Base + Arena.Used;
        Arena.Used += Bytes;
    }
    else
    {
        fprintf(stderr, "ERROR: Ran out of memory!\n");
    }
    
    return Result;
}


enum token_type
{
    Token_Unknown,
    
    Token_OpenParen,
    Token_CloseParen,
    Token_Colon,
    Token_Semicolon,
    Token_Asterisk,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,
    Token_Hash,
    Token_Comma,
    
    Token_Identifier,
    Token_String,
    Token_Float,
    Token_Integer,
    
    Token_EndOfStream
};

struct stringinplace
{
    char* Text;
    size_t Length;
};

struct token
{
    token_type Type;
    size_t TextLength;
    char* Text;
};

// TODO: technically i should turn these into a discriminated union so I don't have to keep casting the pointer
// everything already written tho and its working so...
enum ast_type
{
    AST_Identifier,
    AST_Float,
    AST_Integer,
    AST_Array,
    AST_String,
    AST_KV,
};
struct ast_header
{
    ast_type Type;
};

struct ast_identifier
{
    ast_header Header;
    size_t TextLength;
    char* Text;
};

struct ast_string
{
    ast_header Header;
    size_t TextLength;
    char* Text;
};

struct ast_float
{
    ast_header Header;
    float Value;
    ast_float* Next;
};

struct ast_integer
{
    ast_header Header;
    int Value;
    ast_integer* Next;
};

struct ast_kv
{
    ast_header Header;
    ast_identifier* Key;
    ast_header* Value;
    ast_kv* Next;
};

struct ast_array
{
    ast_header Header;
    ast_header* First;
    int Count;
};

struct tokenizer
{
    char* At;
};

struct ll_string
{
    size_t Length;
    char* Text;
    
    ll_string* Next;
};

struct modifier_event_name_info
{
    stringinplace ModifierName;
    stringinplace EventName;
    modifier_event_name_info* Next;
};

struct parse_info
{
    //FILE* HFile;
    FILE* CFile;
    
    char** FileNames;
    int FileNameCount;
    ll_string* ModifierRoot;
    ll_string* ModifierEnd;
    modifier_event_name_info* ModifierEventNameInfoRoot;
    modifier_event_name_info* ModifierEventNameInfoEnd;
    
};

#endif //PREPROCESSOR_H
