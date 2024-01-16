#ifndef UI_H
#define UI_H

#include "basic.hpp"
#include <stack>

static inline Vector2 GetScreenCenter() { return {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f}; }

struct TextAlignment {
    typedef uint8_t T;
    const static T LEFT      = 0x00;
    const static T HCENTER   = 0x01;
    const static T RIGHT     = 0x02;
    const static T TOP       = 0x00;
    const static T VCENTER   = 0x04;
    const static T BOTTOM    = 0x08;
};

Rectangle DrawTextAligned(const char* text, Vector2 pos, TextAlignment::T alignment, Color c);

struct ButtonStateFlags {
    typedef uint8_t T;
    const static T NONE            = 0x00;
    const static T HOVER           = 0x01;
    const static T PRESSED         = 0x02;
    const static T DISABLED        = 0x04;
    const static T JUST_PRESSED    = 0x08;
    const static T JUST_UNPRESSED  = 0x10;
    const static T JUST_HOVER_IN   = 0x20;
    const static T JUST_HOVER_OUT  = 0x40;
};

ButtonStateFlags::T GetButtonState(bool is_in_area, bool was_in_area);

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
    ButtonStateFlags::T WriteButton(const char* text, int inset);
    ButtonStateFlags::T AsButton() const;

private:
    void _Advance(Vector2 size);
};

struct UIGlobals {
    std::stack<TextBox> text_box_stack = std::stack<TextBox>();
    char mouseover_text[1024] = "";
    bool scroll_lock;
    Font default_font;
};

void UIContextCreateNew(int x, int y, int w, int h, int text_size, Color color);
void UIContextPushGlobal(int x, int y, int w, int h, int text_size, Color color);

int UIContextPushInset(int margin, int h);
int UIContextPushScrollInset(int margin, int h, int allocated_height, int* scroll);
void UIContextPushInline(int x_margin);
void UIContextPushAligned(int width, int height, TextAlignment::T align);
void UIContextPushHSplit(int x_start, int x_end);
void UIContextPushGridCell(int columns, int rows, int column, int row);
void UIContextPop();
Vector2 UIContextGetRelMousePos();

ButtonStateFlags::T UIContextAsButton();
void UIContextEnclose(Color background_color, Color line_color);
void UIContextShrink(int dx, int dy);
void UIContextWrite(const char* text, bool linebreak=true);
void UIContextFillline(double value, Color fill_color, Color background_color);
ButtonStateFlags::T UIContextDirectButton(const char* text, int inset);
TextBox& UIContextCurrent();
void HandleButtonSound(ButtonStateFlags::T button_state_flags);

void UIInit();
void UIStart();
void UIEnd();
Font GetCustomDefaultFont();
void UISetMouseHint(const char* text);

ButtonStateFlags::T DrawTriangleButton(Vector2 point, Vector2 base, double width, Color color);
ButtonStateFlags::T DrawCircleButton(Vector2 midpoint, double radius, Color color);

#endif  // UI_H