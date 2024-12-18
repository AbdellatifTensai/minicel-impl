#include <regex.h>

#include "types.h"
#include "x11.h"
//#include <raylib.h>

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

typedef struct{
        char *Data;
        s32 Length;
} string;

typedef struct{
        s32 Length;
        char Data[32];
} short_string;

#define STR_FMT "%.*s"
#define STR_ARG(s) (s).Length, (s).Data
#define V2S_FMT "%d, %d"
#define V2S_ARG(v) (v).x, (v).y

#define STR(str) (string){(str), sizeof((str))-1}
string String(char *Data, s32 Length){ return (string){ .Data = Data, .Length = Length }; }
//s32 StringComp(string a, string b){ return strncmp(a.Data, b.Data, MAX(a.Length, b.Length)); }


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

static regex_t RegexParse;
static regex_t RegexFill;

s32 Tokenize(regex_t Regex, string Input, string *OutStrings){
        regmatch_t Matches;
        char *CurrentChar = Input.Data;
        s32 StringsCount = 0;
        for(; !regexec(&Regex, CurrentChar, 1, &Matches, 0); StringsCount++){
                OutStrings[StringsCount] = String(CurrentChar + Matches.rm_so, Matches.rm_eo - Matches.rm_so);
                CurrentChar += Matches.rm_eo;
        }
        return StringsCount;
}

void FillCells(cells Table, string Input, char Delim){
        bool IsParse = false;
        string Tokens[32];
        s32 TokensCount = Tokenize(RegexFill, Input, Tokens);
        for(s32 TokenIdx=0, Row = 1, Column = 0; TokenIdx<TokensCount; TokenIdx++){
                string Token = Tokens[TokenIdx];
                if(Token.Data[0] == '=')
                        IsParse = true;

                if(Token.Data[0] == Delim){
                        if(IsParse){
                                Table.StringsToParsePos[Table.Count->StringsToParse] = V2s(Row, ++Column);
                                Table.Count->StringsToParse++;
                                IsParse = false;
                        }
                        else{
                                Table.StringsPos[Table.Count->Strings] = V2s(Row, ++Column);
                                Table.Count->Strings++;
                        }
                }
                
                else if(Token.Data[0] == '\n'){
                        if(IsParse){
                                Table.StringsToParsePos[Table.Count->StringsToParse] = V2s(Row++, ++Column);
                                Table.Count->StringsToParse++;
                                Column = 0;
                                IsParse = false;
                        }
                        else{
                                Table.StringsPos[Table.Count->Strings] = V2s(Row++, ++Column);
                                Table.Count->Strings++;
                                Column = 0;
                        }
                }

                else{
                        if(IsParse){
                                short_string *Cell = &Table.StringsToParse[Table.Count->StringsToParse];
                                for(s32 i=0; i<Token.Length; i++) Cell->Data[i] = Token.Data[i];
                        }
                        else{
                                short_string *Cell = &Table.Strings[Table.Count->Strings];
                                for(s32 i=0; i<Token.Length; i++) Cell->Data[i] = Token.Data[i];
                        }
                }
        }
        
        /* for(s32 CharIndex = 0, Row = 1, Column = 0 ; CharIndex<Input.Length; CharIndex++){ */
        /*         char CurrentChar = Input.Data[CharIndex]; */
        /*         if(CurrentChar == '=') */
        /*                 IsToParse = true; */

        /*         if(CurrentChar == Delim){ */
        /*                 if(IsToParse){ */
        /*                         Table.StringsToParsePos[Table.Count->StringsToParse] = V2s(Row, ++Column); */
        /*                         Table.Count->StringsToParse++; */
        /*                         IsToParse = false; */
        /*                 } */
        /*                 else{ */
        /*                         Table.StringsPos[Table.Count->Strings] = V2s(Row, ++Column); */
        /*                         Table.Count->Strings++; */
        /*                 } */
        /*         } */
        /*         else if(CurrentChar == '\n'){ */
        /*                 if(IsToParse){ */
        /*                         Table.StringsToParsePos[Table.Count->StringsToParse] = V2s(Row++, ++Column); */
        /*                         Table.Count->StringsToParse++; */
        /*                         Column = 0; */
        /*                         IsToParse = false; */
        /*                 } */
        /*                 else{ */
        /*                         Table.StringsPos[Table.Count->Strings] = V2s(Row++, ++Column); */
        /*                         Table.Count->Strings++; */
        /*                         Column = 0; */
        /*                 } */
        /*         } */
        /*         else{ */
        /*                 if(IsToParse){ */
        /*                         short_string *Cell = &Table.StringsToParse[Table.Count->StringsToParse]; */
        /*                         Cell->Data[Cell->Length++] = CurrentChar; */
        /*                 } */
        /*                 else{ */
        /*                         short_string *Cell = &Table.Strings[Table.Count->Strings]; */
        /*                         Cell->Data[Cell->Length++] = CurrentChar; */
        /*                 } */
        /*         } */
        /* } */
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

void Factor(string *Tokens, u32 *TokenIndex, node *Nodes, u32 *NodeIndex, u32 CurrentNodeIndex){
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

        case EXPR_TYPE_PRIMARY_OP: break;
        }
}

void Term(string *Tokens, u32 *TokenIndex, node *Nodes, u32 *NodeIndex, u32 CurrentNodeIndex){
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

void Expression(string *Tokens, u32 *TokenIndex, node *Nodes, u32 *NodeIndex, u32 CurrentNodeIndex){
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

f64 ParseAST(node *AST){
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

        default:;//UNREACHABLE;
        }
        
        return 0;
}

void ParseCells(cells Table){
        string Tokens[32];
        node Nodes[32];
        u32 NodesCount = 0;

        for(s32 x=0;x<32;x++){ Tokens[x] = EMPTY_STRING; Nodes[x] = EMPTY_NODE; } //look up to this

        for(s32 CellIdx=0; CellIdx<Table.Count->StringsToParse; CellIdx++){

                short_string Cell = Table.StringsToParse[CellIdx];
                string Field = String(&Cell.Data[1], Cell.Length - 1); //NOTE: no need to skip '=' i think
                Tokenize(RegexParse, Field, Tokens);

                u32 CurrentTokenIndex = 0;
                Expression(Tokens, &CurrentTokenIndex, Nodes, &NodesCount, 0);
                f64 ParsedCell = ParseAST(&Nodes[0]);

                short_string *NewCell = &Table.Strings[Table.Count->Strings];
                NewCell->Length = stbsp_snprintf(NewCell->Data, 32, "%.2f", ParsedCell);

                Table.StringsPos[Table.Count->Strings++] = Table.StringsToParsePos[CellIdx];
                Table.StringsToParsePos[CellIdx] = V2s(0,0);

                for(s32 x=0;x<32;x++){ Tokens[x] = EMPTY_STRING; Nodes[x] = EMPTY_NODE; } //look up to this
        }
}

f32 Absf(f32 x){ return x>0? x: -x; }
f32 Floor(f32 x){ return (f32)(s32)x; }

s32 InitRegex(string Delim){

        if(regcomp(&RegexParse, "[0-9]+?\\.?[0-9]+|[A-Z][0-9]+|SUM|\\+|-|\\*|/|\\(|\\)|:", REG_EXTENDED))
                return -1;
        
        char Tmp[] = "=|\\n|";
        char Pattern[32];
        u32 PatternLength = sizeof(Tmp)-1;
        for(u32 i=0; i<PatternLength; i++)
                Pattern[i] = Tmp[i];
        for(s32 i=0; i<Delim.Length; i++)
                Pattern[PatternLength + i] = Delim.Data[i];

        if(regcomp(&RegexFill, Pattern, REG_EXTENDED))
                return -1;

        return 0;
}

int main(){ 

        const string Input = STR(
                                 "1      |2\n"
                                 "3      |4   |   5\n"
                                 "=12-13*9    |=19*10\n"
                                 );

        if(InitRegex(STR("\\|"))) return -1;

        cells Table;
        u8 Buff[128 * (sizeof(*Table.Nums) + sizeof(*Table.NumsPos)) +
                  128 * (sizeof(*Table.Strings) + sizeof(*Table.StringsPos)) +
                  128 * (sizeof(*Table.StringsToParse) + sizeof(*Table.StringsToParsePos))];

        for(u32 x=0;x<sizeof(Buff);x++) Buff[x] = 0;

        Table.Mem = (void *)Buff;
        Table.Count = Table.Mem;
        Table.Nums = (f64 *)Table.Count + sizeof(*Table.Count);
        Table.NumsPos = (v2s *)&Table.Nums[128];
        Table.Strings = (short_string *)&Table.NumsPos[128];
        Table.StringsPos = (v2s *)&Table.Strings[128];
        Table.StringsToParse = (short_string *)&Table.StringsPos[128];
        Table.StringsToParsePos = (v2s *)&Table.StringsToParse[128];
        
        FillCells(Table, Input, '|');

        frame_buffer Frame = { .Width = 800, .Height = 600, .Stride = 0 };
        u32 PixelBuff[800 * 600];
        Frame.Pixels = PixelBuff;

        InitWindow(Frame, "Minicel");

        f32 Factor = 1.0f;
        v2s Offset = V2s(0, 0);
        v2s SelectedCell = V2s(0, 0);
        v2s MousePos = V2s(0, 0);
        v2s OldMousePos = V2s(0, 0);

        XEvent Event = {0};
        bool WindowShouldClose = false;
        while(!WindowShouldClose){

                u32 CellsHorizantalCount = 10 * Factor;
                u32 CellsVerticalCount = 40 * Factor;
                u32 ScreenWidth = Frame.Width;
                u32 ScreenHeight = Frame.Height;
                u32 CellWidth = ScreenWidth/CellsHorizantalCount;
                u32 CellHeight = ScreenHeight/CellsVerticalCount;
                u32 CellsVertical = CellsVerticalCount + Absf(Offset.y)/CellHeight;
                u32 CellsHorizontal = CellsHorizantalCount + Absf(Offset.x)/CellWidth;
                u32 TextPaddingHorizontal = CellWidth / 16;
                u32 TextPaddingVertical = CellHeight / 4;
                f32 LineThickness = 1/Factor >= 1.0? 1/Factor: 1.0f;

                while(XPending(App.Display)){
                        XNextEvent(App.Display, &Event);
                        switch(Event.type){
                        case ClientMessage:{
                                if((Atom)Event.xclient.data.l[0] == App.WMDeleteWindow)
                                        WindowShouldClose = true;
                        }break;

                        case ButtonPress:{
                                MousePos = V2s(CLAMP(Event.xbutton.x, 0, Frame.Width), CLAMP(Event.xbutton.y, 0, Frame.Height));
                                MousePos.x -= Offset.x;
                                MousePos.y -= Offset.y;
                                if(Event.xbutton.button == Button1){
                                        SelectedCell.Column = Floor((f32)MousePos.x/CellWidth);
                                        SelectedCell.Row = Floor((f32)MousePos.y/CellHeight);
                                }
                                OldMousePos = MousePos;
                        }break;

                        case MotionNotify:{
                                MousePos = V2s(CLAMP(Event.xbutton.x, 0, Frame.Width), CLAMP(Event.xbutton.y, 0, Frame.Height));
                                MousePos.x -= Offset.x;
                                MousePos.y -= Offset.y;

                                switch(Event.xmotion.state){
                                case Button1MotionMask:{
                                        SelectedCell.Column = Floor((f32)MousePos.x/CellWidth);
                                        SelectedCell.Row = Floor((f32)MousePos.y/CellHeight);
                                }break;
                                case Button3MotionMask:{
                                        Offset.x += MousePos.x - OldMousePos.x;
                                        Offset.x = MIN(Offset.x, 0);
                                        Offset.y += MousePos.y - OldMousePos.y;
                                        Offset.y = MIN(Offset.y, 0);
                                }break;
                                }

                                OldMousePos = MousePos;
                        }break;

                        case Expose: DrawImage(); break;
                        }
                }

                RenderBackground(Frame, 0x00181818);

                /*
                if(IsKeyDown(KEY_LEFT_CONTROL)){
                        Factor += GetMouseWheelMove() / 16;
                        Factor = Factor <= 0.125f? 0.125f: Factor;
                }
                */
                //Offset.y += GetMouseWheelMove()*CellHeight;
                //Offset.y = Offset.y >= 0? 0: Offset.y;

                //draw table
                for(u32 Row=0; Row<=CellsVertical; Row++)
                        RenderRectangle(Frame, V2s(0, Row*CellHeight + Offset.y), V2s(ScreenWidth, Row*CellHeight + Offset.y + LineThickness), 0x00555555);

                for(u32 Column=0; Column<=CellsHorizontal; Column++)
                        RenderRectangle(Frame, V2s(Column*CellWidth + Offset.x, 0), V2s(Column*CellWidth + Offset.x + LineThickness, ScreenHeight), 0x00555555);

                v2s CellPos = V2s(SelectedCell.Column*CellWidth + Offset.x, SelectedCell.Row*CellHeight + Offset.y);
                RenderRectangle(Frame, CellPos, V2s(CellPos.x + CellWidth, CellPos.y + LineThickness + 1), 0x00FFFFFF);
                RenderRectangle(Frame, V2s(CellPos.x, CellPos.y + CellHeight), V2s(CellPos.x + CellWidth, CellPos.y + CellHeight + LineThickness + 1), 0x00FFFFFF);
                RenderRectangle(Frame, CellPos, V2s(CellPos.x + LineThickness + 1, CellPos.y + CellHeight), 0x00FFFFFF);
                RenderRectangle(Frame, V2s(CellPos.x + CellWidth, CellPos.y), V2s(CellPos.x + CellWidth + LineThickness + 1, CellPos.y + CellHeight), 0x00FFFFFF);

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

                /*
                if(IsKeyPressed(KEY_ENTER)) ParseCells(Table);
                if(IsKeyPressed(KEY_RIGHT)) SelectedCellColumn += 1; 
                if(IsKeyPressed(KEY_LEFT))  SelectedCellColumn -= SelectedCellColumn > 0? 1: 0;
                if(IsKeyPressed(KEY_DOWN))  SelectedCellRow += 1;
                if(IsKeyPressed(KEY_UP))    SelectedCellRow -= SelectedCellRow > 0? 1: 0;
                */

                for(s32 CellIdx=0; CellIdx<Table.Count->Strings; CellIdx++){
                        v2s CellPos = Table.StringsPos[CellIdx];
                        CellPos.Row--; CellPos.Column--;
                        v2s Pos = V2s(CellPos.Column*CellWidth + Offset.x + TextPaddingHorizontal, CellPos.Row*CellHeight + Offset.y + TextPaddingVertical);
                        if(Pos.x < 0 || Pos.y < 0) continue;

                        short_string Cell = Table.Strings[CellIdx];
                        RenderText(Frame, Cell.Data, Cell.Length, Pos, 1, 0x00FF00FF);
                }

                for(s32 CellIdx=0; CellIdx<Table.Count->StringsToParse; CellIdx++){
                        v2s CellPos = Table.StringsToParsePos[CellIdx];
                        CellPos.Row--; CellPos.Column--;
                        v2s Pos = V2s(CellPos.Column*CellWidth + Offset.x + TextPaddingHorizontal, CellPos.Row*CellHeight + Offset.y + TextPaddingVertical);
                        if(Pos.x < 0 || Pos.y < 0) continue;

                        short_string Cell = Table.StringsToParse[CellIdx];
                        RenderText(Frame, Cell.Data, Cell.Length, Pos, 1, 0x00FF00FF);
                }

#ifdef DEBUG 
                {
                        char DebugBuff[256];
                        u32 DebugBuffLength = stbsp_snprintf(DebugBuff, 256, "Factor = %f\n1/F = %f\nLineThickness = %f\nOffset = "V2S_FMT"\nSelectedCell = "V2S_FMT"\nMousePos = "V2S_FMT"\nOldMousePos = "V2S_FMT,
                                                             Factor, 1/Factor, LineThickness, V2S_ARG(Offset), V2S_ARG(SelectedCell), V2S_ARG(MousePos), V2S_ARG(OldMousePos));
                        RenderText(Frame, DebugBuff, DebugBuffLength, V2s(0, 100), 3, 0x00FF00FF);
                }
#endif

                DrawImage();
        }

        XFree(App.Image);
        XCloseDisplay(App.Display);

        return 0;
}
