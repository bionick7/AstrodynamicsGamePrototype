#ifndef UI_H
#define UI_H

#include "basic.hpp"

static inline Vector2 GetScreenCenter() { return {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f}; }

typedef uint8_t TextAlignment;
const TextAlignment TEXT_ALIGNMENT_LEFT      = 0x00;
const TextAlignment TEXT_ALIGNMENT_HCENTER   = 0x01;
const TextAlignment TEXT_ALIGNMENT_RIGHT     = 0x02;
const TextAlignment TEXT_ALIGNMENT_TOP       = 0x00;
const TextAlignment TEXT_ALIGNMENT_VCENTER   = 0x04;
const TextAlignment TEXT_ALIGNMENT_BOTTOM    = 0x08;

void DrawTextAligned(const char* text, Vector2 pos, TextAlignment alignment, Color c);

typedef uint8_t ButtonStateFlags;
const ButtonStateFlags BUTTON_STATE_FLAG_NONE            = 0x00;
const ButtonStateFlags BUTTON_STATE_FLAG_HOVER           = 0x01;
const ButtonStateFlags BUTTON_STATE_FLAG_PRESSED         = 0x02;
const ButtonStateFlags BUTTON_STATE_FLAG_DISABLED        = 0x04;
const ButtonStateFlags BUTTON_STATE_FLAG_JUST_PRESSED    = 0x08;
const ButtonStateFlags BUTTON_STATE_FLAG_JUST_UNPRESSED  = 0x10;
const ButtonStateFlags BUTTON_STATE_FLAG_JUST_HOVER_IN   = 0x20;
const ButtonStateFlags BUTTON_STATE_FLAG_JUST_HOVER_OUT  = 0x40;

struct TextBox {
    // Rect
    int text_start_x;
    int text_start_y;
    int width;
    int height;

    // Render Rect
    Rectangle render_rec;

    // Layout
    int text_margin_x;
    int text_margin_y;
    int text_size;
    Color text_color;
    Color text_background;

    // Variables
    int x_cursor;
    int y_cursor;
    int line_size_x;
    int line_size_y;
    int text_counter;

    TextBox(int x, int y, int w, int h, int text_size, Color color);
    void LineBreak();
    void EnsureLineBreak();
    void Enclose(int inset_x, int inset_y, Color background_color, Color line_color);
    void Write(const char* text);
    void WriteLine(const char* text);
    void DebugDrawRenderRec() const;
    int GetLineHeight() const;
    ButtonStateFlags WriteButton(const char* text, int inset);
    ButtonStateFlags AsButton() const;

private:
    void _Advance(Vector2 size);
};

void UIContextCreate(int x, int y, int w, int h, int text_size, Color color);

int UIContextPushInset(int margin, int h);
int UIContextPushScrollInset(int margin, int h, int allocated_height, int scroll);
void UIContextPushInline(int x_margin);
void UIContextPushAligned(int width, int height, TextAlignment align);
void UIContextPushHSplit(int x_start, int x_end);
void UIContextPushGridCell(int columns, int rows, int column, int row);
void UIContextPop();
Vector2 UIContextGetRelMousePos();

ButtonStateFlags UIContextAsButton();
void HandleButtonSound(ButtonStateFlags button_state_flags);
void UIContextEnclose(Color background_color, Color line_color);
void UIContextShrink(int dx, int dy);
void UIContextWrite(const char* text, bool linebreak=true);
void UIContextFillline(double value, Color fill_color, Color background_color);
ButtonStateFlags UIContextDirectButton(const char* text, int inset);
TextBox& UIContextCurrent();

void UISetMouseHint(const char* text);

void UIInit();
void UIStart();
void UIEnd();
Font GetCustomDefaultFont();

ButtonStateFlags DrawTriangleButton(Vector2 point, Vector2 base, double width, Color color);
ButtonStateFlags DrawCircleButton(Vector2 midpoint, double radius, Color color);

#endif  // UI_H