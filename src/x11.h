#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

typedef struct{
        Display *Display;
        Window Window;
        u32 WindowWidth, WindowHeight;
        XImage *Image;
        Atom WMDeleteWindow;
        GC GC;
} x11app;
static x11app App;

typedef struct{
        s32 Width, Height, Stride;
        u32 *Pixels; 
} frame_buffer;

#define GLYPH_WIDTH 8
#define GLYPH_HEIGHT 8
u8 BitmapFont[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00 , 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00 , 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00 , 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00 , 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00, 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00 , 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00 , 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00 , 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06 , 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00 , 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00 , 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00 , 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00 , 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00 , 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00 , 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00 , 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00 , 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00, 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00 , 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00 , 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00 , 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00 , 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06 , 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00 , 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00 , 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00, 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00 , 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00 , 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00 , 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00 , 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00 , 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00 , 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00 , 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00, 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00 , 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00 , 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 , 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00 , 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00 , 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00 , 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00 , 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00 , 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00 , 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00 , 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00 , 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00 , 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00 , 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 , 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00 , 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00, 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00 , 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00 , 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00 , 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00 , 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00 , 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00 , 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00 , 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF , 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00 , 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00 , 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00 , 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00 , 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00 , 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00, 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F , 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00 , 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 , 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E , 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00 , 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 , 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00 , 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00 , 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00 , 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F , 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78 , 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00 , 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00 , 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00 , 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00 , 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00, 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00 , 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00 , 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F , 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00 , 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00 , 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00 , 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00 , 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


void InitWindow(frame_buffer Frame, const char *Title){
        App.Display = XOpenDisplay(NULL);
        XSetWindowAttributes SetAttributes = {0};
        App.Window = XCreateWindow(App.Display, XRootWindow(App.Display, 0), 0, 0, Frame.Width, Frame.Height, 0, 0, InputOutput, CopyFromParent, CWOverrideRedirect, &SetAttributes);
        App.WindowWidth = Frame.Width;
        App.WindowHeight = Frame.Height;
        XWindowAttributes WindowAttributes = {0};
        XGetWindowAttributes(App.Display, App.Window, &WindowAttributes);
        App.Image = XCreateImage(App.Display, WindowAttributes.visual, WindowAttributes.depth, ZPixmap, 0, (char *)Frame.Pixels, Frame.Width, Frame.Height, 32, Frame.Width*4);  
        App.WMDeleteWindow = XInternAtom(App.Display, "WM_DELETE_WINDOW", 0);
        App.GC = XDefaultGC(App.Display, 0);

        XSizeHints Hints = {.min_width = Frame.Width, .min_height = Frame.Height, .max_height = Frame.Height, .max_width = Frame.Width, .flags = PMinSize | PMaxSize};
        XSetSizeHints(App.Display, App.Window, &Hints, XA_WM_NORMAL_HINTS);

        XSetWMProtocols(App.Display, App.Window, &App.WMDeleteWindow, 1);
        XStoreName(App.Display, App.Window, Title);
        XSelectInput(App.Display, App.Window, KeyPressMask | ExposureMask | ButtonPressMask | ButtonMotionMask);
        XMapWindow(App.Display, App.Window);
        XSync(App.Display, 0);
}

void DrawImage(){
        XPutImage(App.Display, App.Window, App.GC, App.Image, 0, 0, 0, 0, App.WindowWidth, App.WindowHeight);
        XSync(App.Display, 0);
}

void RenderText(frame_buffer Frame, char *Text, u32 Length, v2s Pos, s32 Scale, u32 Color){
        s32 OffsetX = Pos.x;
        for(u32 i = 0; i < Length; i++){
                char c = Text[i];
                if(c == '\n'){
                        Pos.y += GLYPH_HEIGHT*Scale;
                        OffsetX = Pos.x;
                        continue;
                }
                for(s32 y = 0; y < GLYPH_HEIGHT; y++)
                        for(s32 x = 0; x < GLYPH_WIDTH; x++){
                                u8 Row = BitmapFont[c*GLYPH_WIDTH + y];
                                if(Row & (1 << x)){
                                        for(s32 ScaleY = 0; ScaleY < Scale; ScaleY++)
                                                for(s32 ScaleX = 0; ScaleX < Scale; ScaleX++)
                                                        Frame.Pixels[(Pos.y + y*Scale + ScaleY)*(Frame.Width + Frame.Stride) + (x*Scale + ScaleX + OffsetX)] = Color;
                                }
                        }
                OffsetX += GLYPH_WIDTH * Scale;
        }
}

void RenderLineShader(frame_buffer Frame, v2s P0, v2s P1, s32 LineThickness, u32 Color){
        //TODO: find a way to make the bounding box smaller
        s32 StartX = MAX((P0.x - LineThickness), 0);
        s32 EndX = MIN(P1.x+LineThickness, Frame.Width);
        if(P0.x > P1.x){
                StartX = MAX((P1.x - LineThickness), 0);
                EndX = MIN(P0.x+LineThickness, Frame.Width);
        }

        s32 StartY = MAX((P0.y - LineThickness), 0);
        s32 EndY = MIN(P1.y+LineThickness, Frame.Width);
        if(P0.y > P1.y){
                StartY = MAX((P1.y - LineThickness), 0);
                EndY = MIN(P0.y+LineThickness, Frame.Width);
        }

        v2f P0N = (v2f){ .x = (f32)P0.x/ Frame.Width, .y = (f32)P0.y / Frame.Height };
        v2f P1N = (v2f){ .x = (f32)P1.x/ Frame.Width, .y = (f32)P1.y / Frame.Height };
        v2f P1N_P0N = (v2f){ .x = P1N.x - P0N.x, .y = P1N.y - P0N.y };
        f32 LengthSquaredP1N_P0N = P1N_P0N.x * P1N_P0N.x + P1N_P0N.y * P1N_P0N.y;
        f32 ThicknessSquared = (f32)(LineThickness*LineThickness) / (Frame.Width*Frame.Width + Frame.Height*Frame.Height);

        for(s32 y=StartY; y<=EndY; y++)
                for(s32 x=StartX; x<=EndX; x++){
                        v2f P = (v2f){ .x = (f32)x / Frame.Width, .y = (f32)y / Frame.Height };
                        v2f P_P0N = (v2f){ .x = P0N.x - P.x, .y = P0N.y - P.y };
                        f32 CrossProduct = P_P0N.x * P1N_P0N.y - P_P0N.y * P1N_P0N.x;
                        f32 DistSquared = (CrossProduct*CrossProduct) / LengthSquaredP1N_P0N;
                        if(DistSquared <= ThicknessSquared)
                                Frame.Pixels[y * (Frame.Width + Frame.Stride) + x] = Color;
                }
}

void RenderLineLowHigh(frame_buffer Frame, v2s P0, v2s P1, s32 LineThickness, u32 Color, bool High){
        if(High){
                SWAP(P0.x, P0.y, s32);
                SWAP(P1.x, P1.y, s32);
        }
        s32 dx = P1.x - P0.x;
        s32 dy = P1.y - P0.y;
        s32 S = SIGN(dy);
        dy *= S;
        s32 D = 2*dy - dx;

        for(s32 x=P0.x, y=P0.y; x<P1.x; x++){

                for(s32 ScaleY = 0; ScaleY < LineThickness; ScaleY++)
                        for(s32 ScaleX = 0; ScaleX < LineThickness; ScaleX++)
                                if(High)
                                        Frame.Pixels[(MIN(x + ScaleX, Frame.Width))*(Frame.Height + Frame.Stride) + (MIN(y + ScaleY, Frame.Height))] = Color;
                                else
                                        Frame.Pixels[(MIN(y + ScaleY, Frame.Height))*(Frame.Width + Frame.Stride) + (MIN(x + ScaleX, Frame.Width))] = Color;

                if(D > 0){
                        y += S;
                        D += 2*(dy-dx);
                }
                else
                        D += 2*dy;
        }
}

void RenderLine(frame_buffer Frame, v2s P0, v2s P1, s32 LineThickness, u32 Color){
        s32 AbsDy = ABS(P1.y - P0.y);
        s32 AbsDx = ABS(P1.x - P0.x);
        if(AbsDy < AbsDx)
                if(P1.x < P0.x)
                        RenderLineLowHigh(Frame, P1, P0, LineThickness, Color, false);
                else
                        RenderLineLowHigh(Frame, P0, P1, LineThickness, Color, false);
        else
                if(P1.y < P0.y)
                        RenderLineLowHigh(Frame, P1, P0, LineThickness, Color, true);
                else
                        RenderLineLowHigh(Frame, P0, P1, LineThickness, Color, true);
}

void RenderBackground(frame_buffer Frame, u32 Color){
        for(s32 y=0; y<Frame.Height; y++)
                for(s32 x=0; x<Frame.Width; x++)
                        Frame.Pixels[y * (Frame.Width + Frame.Stride) + x] = Color;
}

void RenderRectangle(frame_buffer Frame, v2s P0, v2s P1, u32 Color){
        s32 StartX = P0.x > P1.x? P1.x: P0.x;
        s32 EndX = P0.x > P1.x? P0.x: P1.x;
        StartX = CLAMP(StartX, 0, Frame.Width);
        EndX = CLAMP(EndX, 0, Frame.Width);

        s32 StartY = P0.y > P1.y? P1.y: P0.y;
        s32 EndY = P0.y > P1.y? P0.y: P1.y;
        StartY = CLAMP(StartY, 0, Frame.Height);
        EndY = CLAMP(EndY, 0, Frame.Height);

        for(s32 y=StartY; y<EndY; y++)
                for(s32 x=StartX; x<EndX; x++)
                        Frame.Pixels[y * (Frame.Width + Frame.Stride) + x] = Color;
}

void RenderCircle(frame_buffer Frame, v2s Center, s32 Radius, u32 Color){
        s32 StartX = MAX(Center.x-Radius, 0);
        s32 EndX = MIN(Center.x+Radius, Frame.Width);
        s32 StartY = MAX(Center.y-Radius, 0);
        s32 EndY = MIN(Center.y+Radius, Frame.Height);
        for(s32 y=StartY; y<EndY; y++)
                for(s32 x=StartX; x<EndX; x++)
                        if((y-Center.y)*(y-Center.y) + (x-Center.x)*(x-Center.x) <= Radius*Radius)
                                Frame.Pixels[y * (Frame.Width + Frame.Stride) + x] = Color;
}