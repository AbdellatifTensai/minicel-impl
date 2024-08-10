#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <regex.h>
#include <raylib.h>

typedef size_t usize;
typedef ssize_t ssize;
typedef float f32;
typedef double f64;
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t s32;
#define MAX(a, b) ((a)>(b)? (a): (b))

typedef struct{
        char *Data;
        s32 Length;
} string;

typedef struct{
        s32 Length;
        char Data[32];
} short_string;

string String(char *Data, u64 Length){ return (string){ .Data = Data, .Length = Length }; }
s32 StringComp(string a, string b){ return strncmp(a.Data, b.Data, MAX(a.Length, b.Length)); }

#define STR(str) (string){(str), sizeof((str))-1}

static const string EMPTY_STRING = {"", 0};

typedef enum { NODE_TYPE_EMPTY, NODE_TYPE_NUM, NODE_TYPE_EXPR, NODE_TYPE_UNI_OP, NODE_TYPE_BIN_OP, NODE_TYPE_COUNT } node_type;

typedef struct node{
        struct node *Left, *Right;
        node_type NodeType;
        union{
                string Op;
                f64 Num;
        };
} node;

static node EMPTY_NODE = { .Left =  &EMPTY_NODE, .Right = &EMPTY_NODE, .NodeType = NODE_TYPE_EMPTY };

typedef struct{ s32 Row, Column; } v2s;
v2s V2s(s32 Row, s32 Column){ return (v2s){Row, Column}; }

typedef struct{
        void *Mem;

        f64 *Nums;
        v2s *NumsPos;

        short_string *Strings;
        v2s *StringsPos;

        short_string *StringsToParse;
        v2s *StringsToParsePos;

        struct{ s32 Nums, Strings, StringsToParse; } *Count; // instead of: '(*Table.StringsCount)++' -> 'Table.Count->Strings++'
} cells;

static regex_t Regex;

u32 Tokenize(string Input, string *OutStrings){
        regmatch_t Matches;
        char *CurrentChar = Input.Data;
        u32 StringsCount = 0;
        for(; !regexec(&Regex, CurrentChar, 1, &Matches, 0); StringsCount++){
                OutStrings[StringsCount] = String(CurrentChar + Matches.rm_so, Matches.rm_eo - Matches.rm_so);
                CurrentChar += Matches.rm_eo;
        }
        return StringsCount;
}

void FillCells(cells Table, string Input, char Delim){
        bool IsToParse = false;

        for(s32 CharIndex = 0, Row = 1, Column = 0 ; CharIndex<Input.Length; CharIndex++){
                char CurrentChar = Input.Data[CharIndex];
                if(CurrentChar == '=')
                        IsToParse = true;

                if(CurrentChar == Delim){
                        if(IsToParse){
                                Table.StringsToParsePos[Table.Count->StringsToParse] = V2s(Row, ++Column);
                                Table.Count->StringsToParse++;
                                IsToParse = false;
                        }
                        else{
                                Table.StringsPos[Table.Count->Strings] = V2s(Row, ++Column);
                                Table.Count->Strings++;
                        }
                }
                else if(CurrentChar == '\n'){
                        if(IsToParse){
                                Table.StringsToParsePos[Table.Count->StringsToParse] = V2s(Row++, ++Column);
                                Table.Count->StringsToParse++;
                                Column = 0;
                                IsToParse = false;
                        }
                        else{
                                Table.StringsPos[Table.Count->Strings] = V2s(Row++, ++Column);
                                Table.Count->Strings++;
                                Column = 0;
                        }
                }
                else{
                        if(IsToParse){
                                short_string *Cell = &Table.StringsToParse[Table.Count->StringsToParse];
                                Cell->Data[Cell->Length++] = CurrentChar;
                        }
                        else{
                                short_string *Cell = &Table.Strings[Table.Count->Strings];
                                Cell->Data[Cell->Length++] = CurrentChar;
                        }
                }
        }
}

f64 StrToNum(const string Expr){
        f64 Num = 0;
        u32 Frac = 1;
        bool Point = false;
        for(s32 CharIndex=0; CharIndex<Expr.Length; CharIndex++){
                char CurrentChar = Expr.Data[CharIndex];
                if(CurrentChar == '.'){ Point = true; continue; }
                if(Point) Frac *= 10.0f;
                Num = Num * 10.f + (f64)(CurrentChar - '0');
        }
        return Num / Frac;
}

typedef enum{ EXPR_TYPE_NONE, EXPR_TYPE_NUM, EXPR_TYPE_PRIMARY_OP, EXPR_TYPE_SECONDARY_OP } expr_type;

bool IsNum(string Expr){
        for(s32 CharIndex=0; CharIndex<Expr.Length; CharIndex++){
                char CurrentChar = Expr.Data[CharIndex];
                if(!(('0' <= CurrentChar && CurrentChar <= '9') || CurrentChar == '.')) return false;
        }
        return true;
}

expr_type ExpressionTypeGet(string Expr){
        if(Expr.Data[0] == '*' || Expr.Data[0] == '/') return EXPR_TYPE_PRIMARY_OP;
        else if(Expr.Data[0] == '-' || Expr.Data[0] == '+') return EXPR_TYPE_SECONDARY_OP;
        else if(IsNum(Expr)) return EXPR_TYPE_NUM;

        return EXPR_TYPE_NONE;
}

void Factor(const string *Tokens, u32 *TokenIndex, node *Nodes, u32 *NodeIndex, u32 CurrentNodeIndex){
        string CurrentToken = Tokens[*TokenIndex];
        expr_type ExpressionType = ExpressionTypeGet(CurrentToken);
        switch(ExpressionType){

        case EXPR_TYPE_NUM:{
                (*TokenIndex)++;
                Nodes[CurrentNodeIndex] = (node){
                        .Left = &EMPTY_NODE,
                        .Right = &EMPTY_NODE,
                        .NodeType = NODE_TYPE_NUM,
                        .Num = StrToNum(CurrentToken)
                };
        } break;

        case EXPR_TYPE_SECONDARY_OP:{
                (*TokenIndex)++;

                u32 RightNodeIndex = (*NodeIndex)++;
                Factor(Tokens, TokenIndex, Nodes, NodeIndex, RightNodeIndex);

                Nodes[CurrentNodeIndex].Right = &Nodes[RightNodeIndex];
                Nodes[CurrentNodeIndex].NodeType = NODE_TYPE_UNI_OP;
                Nodes[CurrentNodeIndex].Op = CurrentToken;
        } break;

        case EXPR_TYPE_NONE:{
                Nodes[CurrentNodeIndex] = EMPTY_NODE;
        }break;
        }
}

void Term(const string *Tokens, u32 *TokenIndex, node *Nodes, u32 *NodeIndex, u32 CurrentNodeIndex){
        u32 LeftNodeIndex = (*NodeIndex)++;
        Factor(Tokens, TokenIndex, Nodes, NodeIndex, LeftNodeIndex);

        while(ExpressionTypeGet(Tokens[*TokenIndex]) == EXPR_TYPE_PRIMARY_OP){
                string CurrentToken = Tokens[*TokenIndex];
                (*TokenIndex)++;
                u32 RightNodeIndex = (*NodeIndex)++;
                Factor(Tokens, TokenIndex, Nodes, NodeIndex, RightNodeIndex);

                u32 OldLeftIndex = (*NodeIndex)++;
                Nodes[OldLeftIndex] = Nodes[LeftNodeIndex];

                Nodes[LeftNodeIndex] = (node){
                        .Left = &Nodes[OldLeftIndex],
                        .Right = &Nodes[RightNodeIndex],
                        .NodeType = NODE_TYPE_BIN_OP
                };
                Nodes[LeftNodeIndex].Op = CurrentToken;
        }

        Nodes[CurrentNodeIndex] = Nodes[LeftNodeIndex];
}

void Expression(const string *Tokens, u32 *TokenIndex, node *Nodes, u32 *NodeIndex, u32 CurrentNodeIndex){
        u32 LeftNodeIndex = (*NodeIndex)++;
        Term(Tokens, TokenIndex, Nodes, NodeIndex, LeftNodeIndex);

        while(ExpressionTypeGet(Tokens[*TokenIndex]) == EXPR_TYPE_SECONDARY_OP){
                string CurrentToken = Tokens[*TokenIndex];
                (*TokenIndex)++;
                u32 RightNodeIndex = (*NodeIndex)++;
                Term(Tokens, TokenIndex, Nodes, NodeIndex, RightNodeIndex);

                u32 OldLeftIndex = (*NodeIndex)++;
                Nodes[OldLeftIndex] = Nodes[LeftNodeIndex];

                Nodes[LeftNodeIndex] = (node){
                        .Left = &Nodes[OldLeftIndex],
                        .Right = &Nodes[RightNodeIndex],
                        .NodeType = NODE_TYPE_BIN_OP
                };
                Nodes[LeftNodeIndex].Op = CurrentToken;
        }

        Nodes[CurrentNodeIndex] = Nodes[LeftNodeIndex];
}

f64 ParseAST(const node *AST){
        switch(AST->NodeType){
        case NODE_TYPE_EMPTY: return 0;

        case NODE_TYPE_NUM: return AST->Num;

        case NODE_TYPE_UNI_OP:{
                if(AST->Op.Data[0] == '+') return ParseAST(AST->Right); 
                else if(AST->Op.Data[0] == '-') return -ParseAST(AST->Right);
        } break; 

        case NODE_TYPE_BIN_OP:{
                f64 LeftNum = ParseAST(AST->Left);
                f64 RightNum = ParseAST(AST->Right);
                
                switch(AST->Op.Data[0]){
                case '+': return LeftNum + RightNum;
                case '-': return LeftNum - RightNum;
                case '*': return LeftNum * RightNum;
                case '/': return LeftNum / RightNum;
                }
        } break;

        default: printf("UNREACHABLE");
        }
        
        return 0;
}

void ParseCells(cells Table){
        string Tokens[32];
        node Nodes[32];
        u32 NodesCount = 0;

        for(u64 x=0;x<32;x++){ Tokens[x] = EMPTY_STRING; Nodes[x] = EMPTY_NODE; } //look up to this

        for(s32 CellIdx=0; CellIdx<Table.Count->StringsToParse; CellIdx++){

                short_string Cell = Table.StringsToParse[CellIdx];
                string Field = String(&Cell.Data[1], Cell.Length - 1); //NOTE: no need to skip '=' i think
                Tokenize(Field, Tokens);

                u32 CurrentTokenIndex = 0;
                Expression(Tokens, &CurrentTokenIndex, Nodes, &NodesCount, 0);
                f64 ParsedCell = ParseAST(&Nodes[0]);

                short_string *NewCell = &Table.Strings[Table.Count->Strings];
                NewCell->Length = snprintf(NewCell->Data, 32, "%.2f", ParsedCell);

                Table.StringsPos[Table.Count->Strings++] = Table.StringsToParsePos[CellIdx];
                Table.StringsToParsePos[CellIdx] = V2s(0,0);

                for(u64 x=0;x<32;x++){ Tokens[x] = EMPTY_STRING; Nodes[x] = EMPTY_NODE; } //look up to this
        }
}

f32 Absf(f32 x){ return x>0? x: -x; }
f32 Floor(f32 x){ return (f32)(s32)x; }

int main(){ 

        const string Input = STR(
                                 "1      |2\n"
                                 "3      |4   |   5\n"
                                 "=12-13*9    |=19*10\n"
                                 );

        if(regcomp(&Regex, "[0-9]+?\\.?[0-9]+|[A-Z][0-9]+|SUM|\\+|-|\\*|/|\\(|\\)|:", REG_EXTENDED))
                return -1;

        cells Table;
        u8 Buff[128 * (sizeof(*Table.Nums) + sizeof(*Table.NumsPos)) +
                  128 * (sizeof(*Table.Strings) + sizeof(*Table.StringsPos)) +
                  128 * (sizeof(*Table.StringsToParse) + sizeof(*Table.StringsToParsePos))];
        memset(Buff, 0, sizeof(Buff));

        Table.Mem = (void *)Buff;
        Table.Count = Table.Mem;
        Table.Nums = (f64 *)Table.Count + sizeof(*Table.Count);
        Table.NumsPos = (v2s *)&Table.Nums[128];
        Table.Strings = (short_string *)&Table.NumsPos[128];
        Table.StringsPos = (v2s *)&Table.Strings[128];
        Table.StringsToParse = (short_string *)&Table.StringsPos[128];
        Table.StringsToParsePos = (v2s *)&Table.StringsToParse[128];
        
        FillCells(Table, Input, '|');

        InitWindow(800, 600, "Minicel");
        SetTargetFPS(60);
        SetWindowState(FLAG_WINDOW_RESIZABLE);

        f32 Factor = 1.0f;
        Vector2 Offset = {0, 0};
        u32 SelectedCellColumn = 0;
        u32 SelectedCellRow = 0;
        while(!WindowShouldClose()){
                BeginDrawing();
                ClearBackground(GetColor(0x181818AA));

                if(IsKeyDown(KEY_LEFT_CONTROL)){
                        Factor += GetMouseWheelMove() / 16;
                        Factor = Factor <= 0.125f? 0.125f: Factor;
                }

                u32 CellsHorizantalCount = 10 * Factor;
                u32 CellsVerticalCount = 40 * Factor;
                u32 ScreenWidth = GetScreenWidth();
                u32 ScreenHeight = GetScreenHeight();
                u32 CellWidth = ScreenWidth/CellsHorizantalCount;
                u32 CellHeight = ScreenHeight/CellsVerticalCount;
                u32 CellsVertical = CellsVerticalCount + Absf(Offset.y)/CellHeight;
                u32 CellsHorizontal = CellsHorizantalCount + Absf(Offset.x)/CellWidth;
                u32 TextPaddingHorizontal = CellWidth / 16;
                u32 TextPaddingVertical = CellHeight / 4;
                f32 LineThickness = 1/Factor >= 1.0? 1/Factor: 1.0f;
                u32 FontSize = 10 * LineThickness ; 

                Offset.y += GetMouseWheelMove()*CellHeight;
                Offset.y = Offset.y >= 0? 0: Offset.y;

                if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)){
                        Vector2 MouseDelta = GetMouseDelta();
                        Offset.x += MouseDelta.x;
                        Offset.y += MouseDelta.y;
                        Offset.x = Offset.x >= 0? 0: Offset.x;
                        Offset.y = Offset.y >= 0? 0: Offset.y;
                }
                //draw table
                for(u32 Row=0; Row<=CellsVertical; Row++)
                        DrawLineEx((Vector2){0          , Row*CellHeight + Offset.y},
                                   (Vector2){ScreenWidth, Row*CellHeight + Offset.y},
                                   LineThickness, GRAY);

                for(u32 Column=0; Column<=CellsHorizontal; Column++)
                        DrawLineEx((Vector2){Column*CellWidth + Offset.x, 0},
                                   (Vector2){Column*CellWidth + Offset.x, ScreenHeight},
                                   LineThickness, GRAY);

                //draw selected cell
                if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
                        Vector2 MouseLeftPress = GetMousePosition();
                        MouseLeftPress.x -= Offset.x;
                        MouseLeftPress.y -= Offset.y;
                        SelectedCellColumn = Floor(MouseLeftPress.x/CellWidth);
                        SelectedCellRow = Floor(MouseLeftPress.y/CellHeight);
                }
                DrawRectangleLinesEx((Rectangle){ SelectedCellColumn*CellWidth + Offset.x, SelectedCellRow*CellHeight + Offset.y, CellWidth, CellHeight}, 2*LineThickness, WHITE);

                //modify cells
                /*
                s32 CharPressed = GetCharPressed();
                if(CharPressed){
                        string PressedCell = CellGet(Cells, SelectedCellRow, SelectedCellColumn);
                        PressedCell.Data[PressedCell.Length] = CharPressed;
                        PressedCell.Length += 1;
                        CellSet(Cells, SelectedCellRow, SelectedCellColumn, PressedCell);
                }
                if(IsKeyPressed(KEY_BACKSPACE)){
                        string PressedCell = CellGet(Cells, SelectedCellRow, SelectedCellColumn);
                        PressedCell.Length -= PressedCell.Length > 0? 1: 0;
                        CellSet(Cells, SelectedCellRow, SelectedCellColumn, PressedCell);
                }
                */

                if(IsKeyPressed(KEY_ENTER)) ParseCells(Table);
                if(IsKeyPressed(KEY_RIGHT)) SelectedCellColumn += 1; 
                if(IsKeyPressed(KEY_LEFT))  SelectedCellColumn -= SelectedCellColumn > 0? 1: 0;
                if(IsKeyPressed(KEY_DOWN))  SelectedCellRow += 1;
                if(IsKeyPressed(KEY_UP))    SelectedCellRow -= SelectedCellRow > 0? 1: 0;

                for(s32 CellIdx=0; CellIdx<Table.Count->Strings; CellIdx++){
                        v2s CellPos = Table.StringsPos[CellIdx];
                        CellPos.Row--; CellPos.Column--;
                        Vector2 Position = { CellPos.Column*CellWidth + Offset.x + TextPaddingHorizontal, CellPos.Row*CellHeight + Offset.y + TextPaddingVertical};

                        short_string Cell = Table.Strings[CellIdx];
                        for(s32 CharIdx=0; CharIdx<Cell.Length; CharIdx++)
                                DrawTextCodepoint(GetFontDefault(), Cell.Data[CharIdx], (Vector2){ Position.x + FontSize*CharIdx/1.5f, Position.y }, FontSize, WHITE); 
                }

                for(s32 CellIdx=0; CellIdx<Table.Count->StringsToParse; CellIdx++){
                        v2s CellPos = Table.StringsToParsePos[CellIdx];
                        CellPos.Row--; CellPos.Column--;
                        Vector2 Position = { CellPos.Column*CellWidth + Offset.x + TextPaddingHorizontal, CellPos.Row*CellHeight + Offset.y + TextPaddingVertical};

                        short_string Cell = Table.StringsToParse[CellIdx];
                        for(s32 CharIdx=0; CharIdx<Cell.Length; CharIdx++)
                                DrawTextCodepoint(GetFontDefault(), Cell.Data[CharIdx], (Vector2){ Position.x + FontSize*CharIdx/1.5f, Position.y }, FontSize, WHITE); 
                }

#ifdef DEBUG 
                {
                        char DebugBuff[256];
                        snprintf(DebugBuff, 256, "Factor = %f\n 1/F = %f\nLineThickness = %f\nOffset = %f , %f\nSelectedCell = %d, %d",
                                 Factor, 1/Factor, LineThickness, Offset.x, Offset.y, SelectedCellRow, SelectedCellColumn);
                        DrawText(DebugBuff, 0, 100, 18, MAGENTA);
                }
#endif

                EndDrawing();
        }

        CloseWindow();

        return 0;
}
