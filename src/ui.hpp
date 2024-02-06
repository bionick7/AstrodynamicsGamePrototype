#ifndef UI_H
#define UI_H

#include "basic.hpp"
#include "id_allocator.hpp"
#include <stack>

static inline Vector2 GetScreenCenter() { return {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f}; }

namespace TextAlignment {
    typedef uint8_t T;
    const static T LEFT      = 0x00;
    const static T HCENTER   = 0x01;
    const static T RIGHT     = 0x02;
    const static T TOP       = 0x00;
    const static T VCENTER   = 0x04;
    const static T BOTTOM    = 0x08;
};

Vector2 ApplyAlignment(Vector2 pos, Vector2 size, TextAlignment::T alignment);
Rectangle DrawTextAligned(const char* text, Vector2 pos, TextAlignment::T alignment, Color c);

namespace ButtonStateFlags {
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
    void Enclose(int inset_x, int inset_y, int corner_radius, Color background_color, Color line_color);
    void Write(const char* text);
    void WriteLine(const char* text);
    void DebugDrawRenderRec() const;
    int GetLineHeight() const;
    ButtonStateFlags::T WriteButton(const char* text, int inset);
    ButtonStateFlags::T AsButton() const;

private:
    void _Advance(Vector2 size);
};

struct ImageAtlas {
    int rows;
    int columns;

    Texture2D texture;
};

struct UIGlobals {
    std::stack<TextBox> text_box_stack = std::stack<TextBox>();
    char mouseover_text[1024] = "";
    bool scroll_lock;
    Font default_font;

    void UIInit();
    void UIStart();
    void UIEnd();
};

namespace ui {
    void PushGlobal(int x, int y, int w, int h, int text_size, Color color);
    void CreateNew(int x, int y, int w, int h, int text_size, Color color);
    void PushMouseHint(int width, int height);

    int PushInset(int margin, int h);
    int PushScrollInset(int margin, int h, int allocated_height, int* scroll);
    void PushInline(int x_margin);
    void PushAligned(int width, int height, TextAlignment::T align);
    void PushHSplit(int x_start, int x_end);
    void PushGridCell(int columns, int rows, int column, int row);
    void Pop();

    ButtonStateFlags::T AsButton();
    void Enclose(Color background_color, Color line_color);
    void EncloseEx(Color background_color, Color line_color, int corner_radius);
    void Shrink(int dx, int dy);

    void Write(const char* text, bool linebreak=true);
    void DrawIcon(AtlasPos pos, int size);
    void DrawIconInline(AtlasPos pos);
    void Fillline(double value, Color fill_color, Color background_color);
    ButtonStateFlags::T DirectButton(const char* text, int inset);

    Vector2 GetRelMousePos();
    TextBox* Current();
}
    
void HandleButtonSound(ButtonStateFlags::T button_state_flags);

Font GetCustomDefaultFont();
void UISetMouseHint(const char* text);


ButtonStateFlags::T DrawTriangleButton(Vector2 point, Vector2 base, double width, Color color);
ButtonStateFlags::T DrawCircleButton(Vector2 midpoint, double radius, Color color);

#endif  // UI_H