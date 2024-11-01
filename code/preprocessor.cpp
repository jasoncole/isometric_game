
/*
TODO:
 search for INCLUDE_DIRECTORY(directory) macro and make function that automatically writes include for all files in said directory
make a recursive function that starts at rpg.cpp and looks for all #include statements and records all included files
function should also look through the included files and see if they include any files
*/


#include "preprocessor.h"

static char* 
ReadEntireFileIntoMemoryAndNullTerminate(char* FileName)
{
    char* Result = 0;
    
    FILE* File = fopen(FileName, "r");
    if (File)
    {
        fseek(File, 0, SEEK_END);
        size_t FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);
        
        Result = (char*)malloc(FileSize + 1);
        fread(Result, FileSize, 1, File);
        Result[FileSize] = 0;
        
        fclose(File);
    }
    
    return Result;
}

// TODO: allocate memroy for filename here so I don't allocate if the file already exists
static int
AddFileName(bool AutoInclude, file_name* FileNames, int Count, char* Name, memory_index Length = 0)
{
    int FilesAdded = 0;
    for (NameIndex = 0;
         NameIndex < Count;
         ++NameIndex)
    {
        if (StringEquals(FileNames[NameIndex].Name, Name))
        {
        }
        else
        {
            char* NamePointer = ArenaTop;
            if (Length)
            {
                Arena.Used += sprintf(NamePointer, "%.*s", (int)Length, Name);
            }
            else
            {
                Arena.Used += sprintf(NamePointer, "%s", Name);
            }
            FileNames[Count] = InitFileName(NamePointer, AutoInclude);
            FilesAdded++;
        }
    }
    return FilesAdded;
}

static int
GetFilesInDirectory(file_name* FileNames, char* Directory)
{
    WIN32_FIND_DATA Data;
    HANDLE hFind;
    
    int FileNameCount= 0;
    
    char DirectoryPath[256];
    char* At = &DirectoryPath[0];
    At += sprintf(At, "%s", Directory);
    sprintf(At, "\\*");
    
    hFind = FindFirstFileA(DirectoryPath, &Data);
    
    do
    {
        if (!(Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            FileNameCount += AddFileName(true, FileNames, FileNameCount, Data.cFileName);
        }
    }
    while (FindNextFile(hFind, &Data) != 0);
    
    return FileNameCount;
}


static bool 
FileNameHasExtension(char* FileName, char* FileExtension)
{
    char* At = FileName;
    while (*At)
    {
        if (*At++ == '.')
        {
            char* Match = FileExtension;
            bool Parsing = true;
            while (Parsing)
            {
                if (*At++ != *Match++)
                {
                    return false;
                }
                if (*At == 0)
                {
                    Parsing = false;
                }
            }
        }
    }
    return true;
}


inline bool
IsEndOfLine(char C)
{
    bool Result = ((C == '\n') ||
                   (C == '\r'));
    return Result;
}

inline bool
IsWhitespace(char C)
{
    bool Result = ((C == ' ') ||
                   (C == '\t') ||
                   (C == '\v') ||
                   (C == '\f') ||
                   IsEndOfLine(C));
    
    return Result;
}

inline bool
IsAlpha(char C)
{
    bool Result = (((C >= 'a') && (C <= 'z')) ||
                   ((C >= 'A') && (C <= 'Z')));
    return Result;
}

inline bool
IsNumber(char C)
{
    // TODO: fix this
    bool Result = ((C >= '0') && (C <= '9'));
    return Result;
}

#define NodeEquals(N, S) StringEquals((N).Text, (N).TextLength, (S))
#define TokenEquals(T, S) StringEquals((T).Text, (T).TextLength, (S))

inline bool
StringEquals(char* str, char* Match, memory_index Length = -1)
{
    Bool Result = false;
    
    memory_index Index = 0;
    bool Scanning = true;
    while (Scanning)
    {
        if ((Index == Length) ||
            (*str == 0))
        {
            Scanning = false;
            if (*Match == 0)
            {
                Result = true;
            }
        }
        else if ((*Match == 0) ||
                 (*str != *At))
        {
            Scanning = false;
            Result = false;
        }
        else
        {
            Match++;
            str++;
            Index++;
        }
    }
    
    return Result;
}

inline bool
TokenHasPrefix(token Token, char* Prefix)
{
    char* At = Prefix;
    int Index = 0;
    while (*At)
    {
        if (Token.Text[Index++] != *At++)
        {
            return false;
        }
    }
    
    return true;
}

static void
EatAllWhitespace(tokenizer* Tokenizer)
{
    for (;;)
    {
        if (IsWhitespace(Tokenizer->At[0]))
        {
            ++Tokenizer->At;
        }
        else if ((Tokenizer->At[0] == '/') &&
                 (Tokenizer->At[1] == '/'))
        {
            Tokenizer->At += 2;
            while (Tokenizer->At[0] && !IsEndOfLine(Tokenizer->At[0]))
            {
                ++Tokenizer->At;
            }
        }
        else if ((Tokenizer->At[0] == '/') &&
                 (Tokenizer->At[1] = '*'))
        {
            Tokenizer->At += 2;
            while (Tokenizer->At[0] && 
                   !((Tokenizer->At[0] == '*') &&
                     (Tokenizer->At[1] == '/')))
            {
                ++Tokenizer->At;
            }
            if (Tokenizer->At[0] == '*')
            {
                Tokenizer->At += 2;
            }
        }
        else
        {
            break;
        }
    }
}

static token
GetToken(tokenizer* Tokenizer)
{
    EatAllWhitespace(Tokenizer);
    
    token Token = {};
    Token.TextLength = 1;
    Token.Text = Tokenizer->At;
    char C = Tokenizer->At[0];
    ++Tokenizer->At;
    switch(C)
    {
        case '\0': { Token.Type = Token_EndOfStream; } break;
        
        case '(': { Token.Type = Token_OpenParen; } break;
        case ')': { Token.Type = Token_CloseParen; } break;
        case ':': { Token.Type = Token_Colon; } break;
        case ';': { Token.Type = Token_Semicolon; } break;
        case '*': { Token.Type = Token_Asterisk; } break;
        case '[': { Token.Type = Token_OpenBracket; } break;
        case ']': { Token.Type = Token_CloseBracket; } break;
        case '{': { Token.Type = Token_OpenBrace; } break;
        case '}': { Token.Type = Token_CloseBrace; } break;
        case '#': { Token.Type = Token_Hash; } break;
        
        case '"': 
        {
            Token.Type = Token_String;
            
            Token.Text = Tokenizer->At;
            while (Tokenizer->At[0] &&
                   Tokenizer->At[0] != '"')
            {
                if ((Tokenizer->At[0] == '\\') &&
                    Tokenizer->At[1])
                {
                    ++Tokenizer->At;
                }
                ++Tokenizer->At;
            }
            Token.Type = Token_String;
            Token.TextLength = Tokenizer->At - Token.Text;
            
            if (Tokenizer->At[0] == '"')
            {
                ++Tokenizer->At;
            }
        } break;
        
        default:
        {
            if (IsAlpha(C))
            {
                Token.Type = Token_Identifier;
                
                while (IsAlpha(Tokenizer->At[0]) ||
                       IsNumber(Tokenizer->At[0]) ||
                       Tokenizer->At[0] == '_')
                {
                    ++Tokenizer->At;
                }
                Token.TextLength = Tokenizer->At - Token.Text;
            }
            
            else if (IsNumber(C))
            {
                bool SeenDecimal = false;
                for (;;)
                {
                    if (Tokenizer->At[0] == '.')
                    {
                        if (SeenDecimal)
                        {
                            break;
                        }
                        else
                        {
                            SeenDecimal = true;
                        }
                    }
                    else if (!IsNumber(Tokenizer->At[0]))
                    {
                        break;
                    }
                    ++Tokenizer->At;
                }
                Token.Type = SeenDecimal ? Token_Float : Token_Integer;
                Token.TextLength = Tokenizer->At - Token.Text;
                
            }
            
            else
            {
                Token.Type = Token_Unknown;
            }
        } break;
    }
    return Token;
}

static token
PeekToken(tokenizer* Tokenizer, int Index = 0)
{
    char* OriginalAt = Tokenizer->At;
    Tokenizer->At += Index;
    token Result = GetToken(Tokenizer);
    Tokenizer->At = OriginalAt;
    return Result;
}

static bool
RequireToken(tokenizer* Tokenizer, token_type DesiredType)
{
    token Token = GetToken(Tokenizer);
    bool ret = (Token.Type == DesiredType);
    return ret;
}

static void
ParseIntrospectionParams(tokenizer* Tokenizer)
{
    for (;;)
    {
        token Token = GetToken(Tokenizer);
        if ((Token.Type == Token_CloseParen) ||
            (Token.Type == Token_EndOfStream))
        {
            break;
        }
    }
}

// TODO: this doesn't work
static void
ParseMember(tokenizer* Tokenizer, token MemberTypeToken)
{
    token MemberVar = GetToken(Tokenizer);
    switch(MemberVar.Type)
    {
        case Token_Asterisk:
        {
            ParseMember(Tokenizer, MemberVar);
        } break;
        case Token_Identifier:
        {
            printf("%d: %.*s\n", MemberVar.Type, (int)MemberVar.TextLength, MemberVar.Text);
            token Token = GetToken(Tokenizer);
            while ((Token.Type != Token_Semicolon) &&
                   (Token.Type != Token_EndOfStream))
            {
                Token = GetToken(Tokenizer);
            }
        } break;
    }
}

static void
ParseStruct(tokenizer* Tokenizer)
{
    token NameToken = GetToken(Tokenizer);
    if (RequireToken(Tokenizer, Token_OpenBrace))
    {
        for (;;)
        {
            token MemberToken = GetToken(Tokenizer);
            if (MemberToken.Type == Token_CloseBrace)
            {
                break;
            }
            else
            {
                ParseMember(Tokenizer, MemberToken);
            }
        }
    }
}

static void
ParseIntrospectable(tokenizer* Tokenizer)
{
    if (RequireToken(Tokenizer, Token_OpenParen))
    {
        ParseIntrospectionParams(Tokenizer);
        
        token TypeToken = GetToken(Tokenizer);
        if (TokenEquals(TypeToken, "struct"))
        {
            ParseStruct(Tokenizer);
        }
        else
        {
            fprintf(stderr, "ERROR: Introspection only supported for structs right now.\n");
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Missing parentheses.\n");
    }
}

static ast_header* ParseValue(tokenizer*, ast_array* = 0);

static ast_kv*
ParseKey(tokenizer* Tokenizer, ast_array* Array = 0)
{
    ast_kv* Result = 0;
    
    token Token = GetToken(Tokenizer);
    switch(Token.Type)
    {
        case Token_Identifier:
        {
            ast_identifier* KeyIdentifier = PushStruct(ast_identifier);
            ast_kv* KV = PushStruct(ast_kv);
            KeyIdentifier->Header.Type = AST_Identifier;
            KeyIdentifier->TextLength = Token.TextLength;
            KeyIdentifier->Text = Token.Text;
            
            if (Array)
            {
                Array->Count++;
            }
            
            KV->Header.Type = AST_KV;
            KV->Key = KeyIdentifier;
            KV->Value = ParseValue(Tokenizer);
            KV->Next = ParseKey(Tokenizer, Array);
            Result = KV;
        } break;
        
        case Token_CloseBrace:
        {
        } break;
        
        case Token_EndOfStream:
        {
        } break;
        
        default:
        {
            fprintf(stderr, "ERROR: Key not found.\n");
            Result = ParseKey(Tokenizer);
        } break;
    }
    return Result;
}

// TODO: this kinda jank because I had to make arrays accept number values or kvs
static ast_header*
ParseValue(tokenizer* Tokenizer, ast_array* Array)
{
    token Token = GetToken(Tokenizer);
    
    ast_header* Result = 0;
    switch(Token.Type)
    {
        case Token_OpenBrace:
        {
            ast_array* NodeArray = PushStruct(ast_array);
            NodeArray->Header.Type = AST_Array;
            NodeArray->Count = 0;
            
            token LookAhead = PeekToken(Tokenizer);
            if (LookAhead.Type == Token_Identifier)
            {
                NodeArray->First = (ast_header*)ParseKey(Tokenizer, NodeArray);
            }
            else
            {
                NodeArray->First = ParseValue(Tokenizer, NodeArray);
            }
            
            Result = (ast_header*)NodeArray;
        } break;
        
        case Token_CloseBrace:
        {
        } break;
        
        case Token_Float:
        {
            ast_float* NodeFloat = PushStruct(ast_float);
            NodeFloat->Header.Type = AST_Float;
            NodeFloat->Value = (float)atof(Token.Text);
            NodeFloat->Next = 0;
            
            if (Array)
            {
                Array->Count++;
                NodeFloat->Next = (ast_float*)ParseValue(Tokenizer, Array);
            }
            Result = (ast_header*)NodeFloat;
        } break;
        
        case Token_Integer:
        {
            ast_integer* NodeInteger = PushStruct(ast_integer);
            NodeInteger->Header.Type = AST_Integer;
            NodeInteger->Value = atoi(Token.Text);
            NodeInteger->Next = 0;
            
            if (Array)
            {
                Array->Count++;
                NodeInteger->Next = (ast_integer*)ParseValue(Tokenizer, Array);
            }
            Result = (ast_header*)NodeInteger;
        } break;
        
        case Token_String:
        {
            ast_string* NodeString = PushStruct(ast_string);
            NodeString->Header.Type = AST_String;
            NodeString->TextLength = Token.TextLength;
            NodeString->Text = Token.Text;
            
            Result = (ast_header*)NodeString;
        } break;
        
        case Token_Identifier:
        {
            ast_identifier* NodeIdentifier = PushStruct(ast_identifier);
            NodeIdentifier->Header.Type = AST_Identifier;
            NodeIdentifier->TextLength = Token.TextLength;
            NodeIdentifier->Text = Token.Text;
            
            Result = (ast_header*)NodeIdentifier;
        } break;
        
        case Token_EndOfStream:
        {
        } break;
        
        default:
        {
            fprintf(stderr, "ERROR: Value not found.\n");
            Result = ParseValue(Tokenizer, Array);
        } break;
        
    }
    
    return Result;
}

static void
PrintAST(ast_header* Node)
{
    if (Node)
    {
        switch(Node->Type)
        {
            case AST_Identifier:
            {
                ast_identifier* NodeIdentifier = (ast_identifier*)Node;
                printf("%.*s",  (int)NodeIdentifier->TextLength, NodeIdentifier->Text);
            } break;
            
            case AST_Float:
            {
                ast_float* NodeFloat = (ast_float*)Node;
                printf("%f", NodeFloat->Value);
            } break;
            
            case AST_Integer:
            {
                ast_integer* NodeInteger = (ast_integer*)Node;
                printf("%d", NodeInteger->Value);
                if (NodeInteger->Next)
                {
                    PrintAST((ast_header*)NodeInteger->Next);
                }
            } break;
            
            case AST_Array:
            {
                ast_array* NodeArray = (ast_array*)Node;
                printf("\n{\n");
                PrintAST((ast_header*)NodeArray->First);
                printf("\n}\n");
            } break;
            
            case AST_String:
            {
                ast_string* NodeString = (ast_string*)Node;
                printf("%.*s",  (int)NodeString->TextLength, NodeString->Text);
            } break;
            
            case AST_KV:
            {
                ast_kv* NodeKV = (ast_kv*)Node;
                printf("\n");
                PrintAST((ast_header*)NodeKV->Key);
                PrintAST(NodeKV->Value);
                PrintAST((ast_header*)NodeKV->Next);
                
            } break;
            
        }
    }
}

static bool
PrintNodeType(ast_header* Node)
{
    bool IsArray = false;
    switch(Node->Type)
    {
        case (AST_Identifier):
        case (AST_Integer):
        {
            printf("int ");
        } break;
        case (AST_String):
        {
            printf("char* ");
        } break;
        case (AST_Float):
        {
            printf("f32 ");
        } break;
        case (AST_Array):
        {
            ast_header* FirstElement = ((ast_array*)Node)->First;
            PrintNodeType(FirstElement);
            IsArray = true;
        } break;
        
        default:
        {
        } break;
    }
    return IsArray;
}

inline void
PrintTabToIndentationLevel(int IndentationLevel)
{
    for (int Index = 0;
         Index < IndentationLevel;
         ++Index)
    {
        printf("    ");
    }
}

static void
PrintKVMember(ast_kv* KV, int IndentationLevel)
{
    PrintTabToIndentationLevel(IndentationLevel);
    bool IsArray = PrintNodeType(KV->Value);
    
    ast_identifier* Key = (ast_identifier*)KV->Key;
    printf("%.*s", (int)Key->TextLength, Key->Text);
    if (IsArray)
    {
        printf("[");
        printf("%d", ((ast_array*)KV->Value)->Count);
        printf("]");
    }
    printf(";\n");
}

// NOTE: re-using the token struct to save struct name
static char*
PrintKVasStruct(ast_kv* KV, char* prefix = "")
{
    printf("struct ");
    
    ast_identifier* Key = (ast_identifier*)KV->Key;
    memory_index PrefixSize = printf(prefix);
    printf("%.*s", (int)Key->TextLength, Key->Text);
    
    char* ret = (char*)PushSize(Key->TextLength + PrefixSize + 1);
    sprintf(ret, "%s%.*s", prefix, (int)Key->TextLength, Key->Text);
    
    printf("\n");
    printf("{\n");
    if (KV->Value->Type == AST_Array)
    {
        ast_array* KVArray = (ast_array*)KV->Value;
        if (KVArray->First->Type == AST_KV)
        {
            for (ast_kv* Index = (ast_kv*)KVArray->First;
                 Index;
                 Index = Index->Next)
            {
                PrintKVMember(Index, 1);
            }
        }
    }
    printf("};\n");
    return ret;
}



static void
ProcessHeroes(ast_header* Node)
{
}

static void
ProcessDescriptions(ast_header* Node)
{
}

inline char
CapitalizeChar(char C)
{
    char UpperCaseDiff = 'A' - 'a';
    char ret = ((C >= 'a') && (C <= 'z')) ? C + UpperCaseDiff : C;
    return ret;
}

static char*
PrintCamelCase(char* Text)
{
    char* ArenaIndex = Arena.Base + Arena.Used;
    char* ret = ArenaIndex;
    bool FirstLetter = true;
    while (*Text)
    {
        if (*Text == '_')
        {
            FirstLetter = true;
        }
        else 
        {
            if (FirstLetter)
            {
                ArenaIndex += sprintf(ArenaIndex, "%c", CapitalizeChar(*Text));
            }
            else
            {
                ArenaIndex += sprintf(ArenaIndex, "%c", *Text);
            }
            FirstLetter = false;
        }
        Text++;
    }
    *ArenaIndex++ = '\0';
    Arena.Used += ArenaIndex - ret;
    return ret;
}

static void
nPrintCamelCase(memory_index Count, char* Text)
{
    bool FirstLetter = true;
    for (int Index = 0;
         Index < Count;
         ++Index)
    {
        if (Text[Index] == '_')
        {
            FirstLetter = true;
        }
        else 
        {
            if (FirstLetter)
            {
                printf("%c", CapitalizeChar(Text[Index]));
            }
            else
            {
                printf("%c", Text[Index]);
            }
            FirstLetter = false;
        }
    }
}

static void
PrintInitVar(ast_header* Node, char* Path = "", int IndentationLevel = 0)
{
    if (Node)
    {
        switch(Node->Type)
        {
            case AST_Identifier:
            {
                ast_identifier* NodeIdentifier = (ast_identifier*)Node;
                printf("%.*s",  (int)NodeIdentifier->TextLength, NodeIdentifier->Text);
            } break;
            
            case AST_Float:
            {
                ast_float* NodeFloat = (ast_float*)Node;
                printf("%f", NodeFloat->Value);
                // TODO: I can't actually assign arrays this way
            } break;
            
            case AST_Integer:
            {
                ast_integer* NodeInteger = (ast_integer*)Node;
                printf("%d", NodeInteger->Value);
            } break;
            
            case AST_Array:
            {
                ast_array* NodeArray = (ast_array*)Node;
                printf("{");
                PrintInitVar((ast_header*)NodeArray->First);
                printf("}");
            } break;
            
            case AST_String:
            {
                ast_string* NodeString = (ast_string*)Node;
                printf("\"%.*s\"",  (int)NodeString->TextLength, NodeString->Text);
                //printf("ArenaPrint(Arena,\"%.*s\")",  (int)NodeString->TextLength, NodeString->Text);
            } break;
            
            case AST_KV:
            {
                ast_kv* NodeKV = (ast_kv*)Node;
                ast_header* Value = NodeKV->Value;
                
                bool IsArray = false;
                if (Value->Type == AST_Array)
                {
                    IsArray = true;
                    Value = ((ast_array*)Value)->First;
                }
                
                int ArrayIndex = 0;
                while (Value)
                {
                    PrintTabToIndentationLevel(IndentationLevel);
                    printf(Path);
                    PrintInitVar((ast_header*)NodeKV->Key);
                    if (IsArray)
                    {
                        printf("[%d]", ArrayIndex++);
                    }
                    
                    printf(" = ");
                    PrintInitVar(Value);
                    printf(";\n");
                    
                    if (IsArray)
                    {
                        if (Value->Type == AST_Integer)
                        {
                            Value = (ast_header*)((ast_integer*)Value)->Next;
                        }
                        else if (Value->Type == AST_Float)
                        {
                            Value = (ast_header*)((ast_float*)Value)->Next;
                        }
                    }
                    else
                    {
                        Value = 0;
                    }
                }
            } break;
            
            default:
            {
            } break;
        }
    }
}

static void
PrintInitVariables(ast_kv* KV, char* StructName, int IndentationLevel)
{
    char* PathPrefix = ArenaTop;
    
    Arena.Used += sprintf(ArenaTop, "SpellInfo->");
    Arena.Used += sprintf(ArenaTop, StructName);
    Arena.Used += sprintf(ArenaTop, ".");
    while (KV)
    {
        PrintInitVar((ast_header*)KV, PathPrefix, IndentationLevel);
        KV = KV->Next;
    }
    Arena.Used -= (ArenaTop - PathPrefix);
}

static void
ProcessAST(ast_header* Node)
{
    if (Node && Node->Type == AST_KV)
    {
        ast_identifier* Key = ((ast_kv*)Node)->Key;
        if (NodeEquals(*Key, "Heroes"))
        {
            //ProcessHeroes();
        }
        else if (NodeEquals(*Key, "Abilities"))
        {
            ast_array* AbilityArray = (ast_array*)(((ast_kv*)Node)->Value);
            if (AbilityArray->First->Type == (AST_KV))
            {
                char** StructTypes = (char**)PushArray(char*, AbilityArray->Count);
                char** StructNames = (char**)PushArray(char*, AbilityArray->Count);
                int TypeCount = 0;
                
                // NOTE: Print struct definition
                for (ast_kv* AbilityIndex = (ast_kv*)AbilityArray->First;
                     AbilityIndex;
                     AbilityIndex = AbilityIndex->Next)
                {
                    StructTypes[TypeCount++] = PrintKVasStruct(AbilityIndex);
                    printf("\n");
                }
                
                // NOTE: print spell info struct
                printf("struct spell_info\n");
                printf("{\n");
                for (int TypeIndex = 0;
                     TypeIndex < TypeCount;
                     ++TypeIndex)
                {
                    PrintTabToIndentationLevel(1);
                    char* Type = StructTypes[TypeIndex];
                    printf(Type);
                    printf(" ");
                    StructNames[TypeIndex] = PrintCamelCase(Type);
                    printf(StructNames[TypeIndex]);
                    printf(";\n");
                }
                printf("};\n");
                
                // NOTE: print init function for spell info
                printf("\ninternal void\nInitSpellInfo(spell_info* SpellInfo)\n{\n");
                // print all the struct types
                
                int StructIndex = 0;
                for (ast_kv* Ability = (ast_kv*)AbilityArray->First;
                     Ability;
                     Ability = Ability->Next)
                {
                    char* StructName = StructNames[StructIndex++];
                    ast_header* FirstMember = ((ast_array*)Ability->Value)->First;
                    PrintInitVariables((ast_kv*)FirstMember, StructName, 1);
                }
                
                printf("}\n");
            }
        }
        else if (NodeEquals(*Key, "Descriptions"))
        {
        }
    }
    if (((ast_kv*)Node)->Next)
    {
        ProcessAST((ast_header*)((ast_kv*)Node)->Next);
    }
}

static ll_string*
ParseCFile(tokenizer* Tokenizer)
{
    
    bool Parsing = true;
    bool IsStructName = false;
    ll_string* LastString = 0;
    ll_string* Result = 0;
    while(Parsing)
    {
        token Token = GetToken(Tokenizer);
        switch(Token.Type)
        {
            
            case Token_EndOfStream:
            {
                Parsing = false;
            } break;
            
            case Token_Unknown:
            {
            } break;
            
            case Token_Identifier:
            {
                if (IsStructName)
                {
                    IsStructName = false;
                    
                    if (TokenHasPrefix(Token, "modifier_"))
                    {
                        ll_string* ModifierTypeName = PushStruct(ll_string);
                        ModifierTypeName->Length = Token.TextLength;
                        ModifierTypeName->Text = Token.Text;
                        ModifierTypeName->Next = 0;
                        
                        if (LastString)
                        {
                            LastString->Next = ModifierTypeName;
                        }
                        else
                        {
                            Result = ModifierTypeName;
                        }
                        LastString = ModifierTypeName;
                        
                    }
                    else if (TokenHasPrefix(Token, "spell_info_"))
                    {
                        //printf("%d: %.*s\n", Token.Type, (int)Token.TextLength, Token.Text);
                    }
                    else if (TokenHasPrefix(Token, "spell_instance_"))
                    {
                        //printf("%d: %.*s\n", Token.Type, (int)Token.TextLength, Token.Text);
                    }
                }
                else if (TokenEquals(Token, "struct"))
                {
                    IsStructName = true;
                }
            } break;
            
            default:
            {
            } break;
        }
    }
    return Result;
}


static int
FindIncludes(file_name* FileNames, char** FilePointers, tokenizer* Tokenizer)
{
    int FilesAdded = 0;
    
    bool Parsing = true;
    bool FoundHash = false;
    bool FoundInclude = false;
    bool ExpectDirectory = false;
    
    char* FirstFileName = ArenaTop;
    Arena.Used += sprintf(FirstFileName, "%s", FirstFile);
    FileNames[FilesAdded++] = InitFileName(FirstFileName, false);
    
    while(Parsing)
    {
        bool ResetInclude = true;
        bool ResetDirectory = true;
        
        token Token = GetToken(Tokenizer);
        switch(Token.Type)
        {
            case Token_EndOfStream:
            {
                Parsing = false;
            } break;
            
            case Token_Unknown:
            {
            } break;
            
            case Token_Identifier:
            {
                if (ExpectDirectory & (!TokenEquals(Token, "directory")))
                {
                    char DirectoryName[256];
                    sprintf(&DirectoryName, "%.*s", (int)Token.TextLength, Token.Text);
                    FilesAdded += GetFilesInDirectory(FileNames + FilesAdded, DirectoryName);
                }
            }
            else if (FoundHash)
            {
                if (TokenEquals(Token, "include"))
                {
                    FoundInclude = true;
                    ResetInclude = false;
                }
            }
            else if (TokenEquals(Token, "INCLUDE_DIRECTORY"))
            {
                ExpectDirectory = true;
                ResetDirectory = false;
            }
        } break;
        
        case Token_OpenParen:
        {
            ResetDirectory = false;
        } break;
        
        case Token_Hash:
        {
            FoundHash = true;
            ResetInclude = false;
        }
        
        case Token_String
        {
            if (FoundInclude)
            {
                FilesAdded += AddFileName(false, FileNames, FilesAdded, Token.Text, Token.TextLength);
            }
        };
        
        default:
        {
        } break;
    }
    if (ResetInclude)
    {
        FoundHash = false;
        FoundInclude = false;
    }
    if (ResetDirectory)
    {
        ExpectDirectory = false;
    }
    return FilesAdded;
}

int 
main(int ArgCount, char** Args)
{
    tokenizer Tokenizer = {};
    Arena.Used = 0;
    Arena.Size = Megabytes(64);
    Arena.Base = (char*)malloc(Arena.Size);
    
    char* MainFile = ReadEntireFileIntoMemoryAndNullTerminate("rpg.cpp");
    
    file_name* FileNames = PushArray(file_name, 1024);
    char** FilePointers = PushArray(char*, 1024);
    
    // TODO: exclude platform code
    int FileNameCount = 0;
    
    char* FirstFileName = ArenaTop;
    Arena.Used += sprintf(FirstFileName, "rpg.cpp") + 1;
    FileNames[FileNameCount++] = InitFileName(FirstFileName, 0);
    
    // TODO: should  findincludes also print all struct definitions?
    // It would have to save all function definitions to a buffer so it could write the function definitions after the struct definitions
    // Lastly, write include the files specified by INCLUDE_DIRECTORY macro
    for (int FileIndex = 0;
         FileIndex < FileCount
         ++FileIndex)
    {
        char* FileContents = ReadEntireFileIntoMemoryAndNullTerminate(FileNames[FileIndex]);
        FilePointers[FileIndex] = Tokenizer->At;
        Parsing = true;
        
        FileNameCount += FindIncludes(FileNames, FilePointers, &Tokenizer);
        
        
    }
    
    char* OrderIndependentBuffer = (char*)PushSize(Megabytes(4)); // null-terminated
    ll_string* ModifierTop = 0;
    ast_header* ASTTop = 0;
    for (int FileIndex = 0;
         FileIndex < FileNameCount;
         ++FileIndex)
    {
        file_name* FileName = FileNames + FileIndex;
        char* FileContents = FilePointers[FileIndex];
        if (FileContents)
        {
            Tokenizer.At = FileContents;
            if (FileNameHasExtension(FileName->Name, "cpp") || FileNameHasExtension(FileName->Name, "h"))
            {
                printf("#include \"%s\"\n", FileName);
                //ModifierTop = ParseCFile(&Tokenizer, &ModifierTop, Buffer);
            }
            else if (FileNameHasExtension(FileName->Name, "kv"))
            {
                // TODO: combine multiple kv files
                ASTTop = (ast_header*)ParseKey(&Tokenizer);
            }
        }
    }
    printf("\n");
    ProcessAST(ASTTop);
    
    printf("\nenum modifier_type\n{\n");
    for (ll_string* ModifierType = ModifierTop;
         ModifierType;
         ModifierType = ModifierType->Next)
    {
        PrintTabToIndentationLevel(1);
        printf("ModifierType_%.*s,\n", (int)ModifierType->Length, ModifierType->Text);
    }
    printf("};\n\n");
    printf("struct modifier\n{\n");
    PrintTabToIndentationLevel(1);
    printf("modifier_type Type;\n");
    PrintTabToIndentationLevel(1);
    printf("union\n");
    PrintTabToIndentationLevel(1);
    printf("{\n");
    for (ll_string* ModifierType = ModifierTop;
         ModifierType;
         ModifierType = ModifierType->Next)
    {
        PrintTabToIndentationLevel(2);
        printf("%.*s ", (int)ModifierType->Length, ModifierType->Text);
        nPrintCamelCase(ModifierType->Length, ModifierType->Text);
        printf(";\n");
    }
    PrintTabToIndentationLevel(1);
    printf("};\n};");
    
}