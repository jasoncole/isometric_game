
/*
TODO:
 search for INCLUDE_DIRECTORY(directory) macro and make function that automatically writes include for all files in said directory
make a recursive function that starts at rpg.cpp and looks for all #include statements and records all included files
function should also look through the included files and see if they include any files

*/


#include "preprocessor.h"

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
        size_t EndOfFile = fread(Result, 1, FileSize, File);
        Result[EndOfFile] = 0;
        
        fclose(File);
    }
    
    return Result;
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

#define NodeEquals(N, S) StringEquals((N).Text, (int)(N).TextLength, (S))
#define TokenEquals(T, S) StringEquals((T).Text, (int)(T).TextLength, (S))

inline bool
StringEquals(char* str, int Length, char* Match)
{
    bool Result = false;
    
    int Index = 0;
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
                 (*str != *Match))
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


// TODO: bug if there are two includes for different paths to same file
// get the actual file pointer?
static char*
RegisterFileName(char** FileNames, int Count, char* Name, int Length = -1)
{
    char* Result = 0;
    
    char* NamePointer = ArenaTop;
    bool AlreadyFound = false;
    for (int NameIndex = 0;
         NameIndex < Count;
         ++NameIndex)
    {
        bool IsMatch = StringEquals(Name, Length, FileNames[NameIndex]);
        
        if (IsMatch)
        {
            AlreadyFound = true;
            break;
        }
    }
    if (!AlreadyFound)
    {
        if (Length >= 0)
        {
            Arena.Used += sprintf(NamePointer, "%.*s", Length, Name);
        }
        else
        {
            Arena.Used += sprintf(NamePointer, "%s", Name);
        }
        Arena.Used++;
        FileNames[Count] = NamePointer;
        Result = NamePointer;
    }
    return Result;
}

static int
GetFilesInDirectory(char** FileNames, int Count, char* Directory, char* IncludeBuffer)
{
    WIN32_FIND_DATA Data;
    HANDLE hFind;
    
    int FilesAdded = 0;
    
    char DirectoryPath[256];
    char* At = &DirectoryPath[0];
    At += sprintf(At, "%s", Directory);
    At += sprintf(At, "\\");
    sprintf(At, "*");
    
    hFind = FindFirstFileA(DirectoryPath, &Data);
    
    do
    {
        if (!(Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            sprintf(At, Data.cFileName);
            char* FileName = RegisterFileName(FileNames, Count + FilesAdded, DirectoryPath);
            if (FileName)
            {
                FilesAdded++;
                
                if (FileNameHasExtension(FileName, "h"))
                {
                    IncludeBuffer += sprintf(IncludeBuffer, "#include \"%s\"\n", FileName);
                }
            }
        }
    }
    while (FindNextFile(hFind, &Data) != 0);
    
    return FilesAdded;
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
                 (Tokenizer->At[1] == '*'))
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

static void
SkipToNewLine(tokenizer* Tokenizer)
{
    while (Tokenizer->At[0] != '\n' &&
           !IsEndOfLine(Tokenizer->At[0]))
    {
        ++Tokenizer->At;
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
        case ',': { Token.Type = Token_Comma; } break;
        
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

static int
WriteFunctionParamsToBuffer(char* Buffer, tokenizer* Tokenizer)
{
    int CharCount = 0;
    char* TokenizerStart = Tokenizer->At;
    while (Tokenizer->At[0] != ')' &&
           !IsEndOfLine(Tokenizer->At[0]))
    {
        ++Tokenizer->At;
        ++CharCount;
    }
    int Result = sprintf(Buffer, "%.*s", CharCount, TokenizerStart);
    return Result;
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
        if (((ast_kv*)Node)->Next)
        {
            ProcessAST((ast_header*)((ast_kv*)Node)->Next);
        }
        
    }
}

#if 0
static ll_string*
//PrintCFile(tokenizer* Tokenizer)
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
#endif

/* NOTE:
1. scans files for #include and INCLUDE_DIRECTORY
2. prints struct definitions
3. prints function prototypes to a buffer
3. save filenames
4. return number of filenames found
*/

// TODO: make this function save modifier meta types like printcfile
// TODO: maybe when I write function prototypes I should just write everything inside the parenthesis
// there's stuff like default arguments that can make it hard to parse function parameters
static void
ParseCFile(char** FileNames, int* FileNameCount, tokenizer* Tokenizer, ll_string** ModifierRoot, ll_string** ModifierEnd, char* IncludeBuffer)
{
    int FilesAdded = 0;
    
    bool Parsing = true;
    int IsInclude = 0;
    bool ExpectDirectory = false;
    int IsStruct = 0;
    
    while(Parsing)
    {
        bool ResetInclude = true;
        bool ResetDirectory = true;
        bool ResetStruct = true;
        
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
                    sprintf(DirectoryName, "%.*s", (int)Token.TextLength, Token.Text);
                    FilesAdded += GetFilesInDirectory(FileNames, *FileNameCount + FilesAdded, DirectoryName, IncludeBuffer);
                }
                else if (IsInclude == 1)
                {
                    if (TokenEquals(Token, "include"))
                    {
                        IsInclude = 2;
                        ResetInclude = false;
                    }
                    else
                    {
                        SkipToNewLine(Tokenizer);
                    }
                }
                else if (IsStruct == 1)
                {
                    if (TokenHasPrefix(Token, "modifier_"))
                    {
                        ll_string* ModifierTypeName = PushStruct(ll_string);
                        ModifierTypeName->Length = Token.TextLength;
                        ModifierTypeName->Text = Token.Text;
                        ModifierTypeName->Next = 0;
                        
                        if (*ModifierEnd)
                        {
                            (*ModifierEnd)->Next = ModifierTypeName;
                            *ModifierEnd = ModifierTypeName;
                        }
                        else
                        {
                            *ModifierRoot = ModifierTypeName;
                            *ModifierEnd = *ModifierRoot;
                        }
                    }
                    
                }
                else if (TokenEquals(Token, "INCLUDE_DIRECTORY"))
                {
                    ExpectDirectory = true;
                    ResetDirectory = false;
                }
                else if (TokenEquals(Token, "struct"))
                {
                    IsStruct = 1;
                    ResetStruct = false;
                }
                
            } break;
            
            case Token_OpenParen:
            {
                if (ExpectDirectory)
                {
                    ResetDirectory = false;
                }
            } break;
            
            case Token_Hash:
            {
                IsInclude = 1;
                ResetInclude = false;
            } break;
            
            case Token_String:
            {
                if (IsInclude == 2)
                {
                    FilesAdded += ((RegisterFileName(FileNames, *FileNameCount + FilesAdded, Token.Text, (int)Token.TextLength)) ? 1 : 0);
                }
            } break;
            
            default:
            {
            } break;
        }
        if (ResetInclude)
        {
            IsInclude = 0;
        }
        if (ResetDirectory)
        {
            ExpectDirectory = false;
        }
        if (ResetStruct)
        {
            IsStruct = 0;
        }
    }
    *FileNameCount += FilesAdded;
}

int 
main(int ArgCount, char** Args)
{
    tokenizer Tokenizer = {};
    Arena.Used = 0;
    Arena.Size = Megabytes(64);
    Arena.Base = (char*)malloc(Arena.Size);
    
    char* MainFile = ReadEntireFileIntoMemoryAndNullTerminate("rpg.cpp");
    
#define MAX_FILE_COUNT 1024
    char** FileNames = PushArray(char*, MAX_FILE_COUNT);
    char** FilePointers = PushArray(char*, MAX_FILE_COUNT);
    char* IncludeBuffer = (char*)PushSize(Megabytes(5));
    IncludeBuffer[0] = '\0';
    
    // TODO: exclude platform code?
    int FileCount = 0;
    
    char* FirstFileName = ArenaTop;
    Arena.Used += sprintf(FirstFileName, "rpg.cpp") + 1;
    FileNames[FileCount++] = FirstFileName;
    
    ast_header* ASTTop = 0;
    ll_string* ModifierRoot = 0;
    ll_string* ModifierEnd = 0;
    for (int FileIndex = 0;
         FileIndex < FileCount;
         ++FileIndex)
    {
        char* FileName = FileNames[FileIndex];
        char* FileContents = ReadEntireFileIntoMemoryAndNullTerminate(FileName);
        FilePointers[FileIndex] = FileContents;
        Tokenizer.At = FileContents;
        
        if (FileContents)
        {
            if (FileNameHasExtension(FileName, "cpp") || FileNameHasExtension(FileName, "h"))
            {
                ParseCFile(FileNames, &FileCount, &Tokenizer, &ModifierRoot, &ModifierEnd, IncludeBuffer);
            }
            else if (FileNameHasExtension(FileName, "kv"))
            {
                ASTTop = (ast_header*)ParseKey(&Tokenizer);
            }
        }
    }
    
    printf("\n");
    ProcessAST(ASTTop);
    
    printf("\n");
    printf(IncludeBuffer);
    
    printf("\nenum modifier_type\n{\n");
    for (ll_string* ModifierType = ModifierRoot;
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
    for (ll_string* ModifierType = ModifierRoot;
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