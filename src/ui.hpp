#ifndef UI_H
#define UI_H

#include "basic.hpp"
#include "id_allocator.hpp"
#include "string_builder.hpp"
#include "text_rendering.hpp"
#include <stack>

#define DEFAULT_FONT_SIZE 20
#define ATLAS_SIZE 40
#define MAX_TOOLTIP_RECURSIONS 50

static inline Vector2 GetScreenCenter() { return {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f}; }
static inline Rectangle GetScreenRect() { return {0, 0, GetScreenWidth(), GetScreenHeight()}; }

namespace TextAlignment {
    typedef uint8_t T;
    const static T LEFT      = 0x00;
    const static T HCENTER   = 0x01;
    const static T RIGHT     = 0x02;
    const static T HCONFORM  = 0x03;

    const static T TOP       = 0x00;
    const static T VCENTER   = 0x04;
    const static T BOTTOM    = 0x08;
    const static T VCONFORM  = 0x0c;

    const static T CENTER    = 0x05;
    const static T CONFORM   = 0x0f;

    const static T HFILTER   = 0x03;
    const static T VFILTER   = 0x0c;
};

Vector2 ApplyAlignment(Vector2 pos, Vector2 size, TextAlignment::T alignment);
Rectangle DrawTextAligned(const char* text, Vector2 pos, TextAlignment::T alignment, Color c, uint8_t z_layer);

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

namespace Direction {
    typedef uint8_t T;
    const static T TOP   = 0x00;
    const static T DOWN  = 0x01;
    const static T LEFT  = 0x02;
    const static T RIGHT = 0x04;
}

ButtonStateFlags::T GetButtonState(bool is_in_area, bool was_in_area);
ButtonStateFlags::T GetButtonStateRec(Rectangle rec);

struct TextBox {
    // Rect
    int text_start_x;
    int text_start_y;
    int width;
    int height;

    // Render Rect
    Rectangle render_rec;
    bool flexible;
    int z_layer;

    // Layout
    int text_margin_x;
    int text_margin_y;
    int text_size;
    Color text_color;
    Color text_background;
    Color background_color;

    // Variables
    int x_cursor;
    int y_cursor;
    int line_size_y;
    int text_max_x;
    int text_max_y;

    TextBox(int x, int y, int w, int h, int text_size, Color color, Color background_color, uint8_t z_layer);
    TextBox(const TextBox* parent, int x, int y, int w, int h);
    int GetCharWidth();
    void LineBreak();
    void EnsureLineBreak();
    void Enclose(int inset, int corner_radius, Color background_color, Color line_color);
    void EnclosePartial(int inset, Color background_color, Color line_color, Direction::T directions);
    void EncloseDynamic(int inset, int corner_radius, Color background_color, Color line_color);
    void Shrink(int dx, int dy);
    void WriteRaw(const char* text, TextAlignment::T align);
    void Write(const char* text, TextAlignment::T align);
    void WriteLine(const char* text, TextAlignment::T align);
    void WriteLayout(const text::Layout* layout, bool advance_cursor);
    void Decorate(const text::Layout* layout, const TokenList* tokens);
    void DrawTexture(Texture2D texture, Rectangle source, int height, Color tint, bool sdf);
    ButtonStateFlags::T WriteButton(const char* text, int inset);
    ButtonStateFlags::T AsButton() const;
    Vector2 GetTextCursor() const;
    Rectangle GetRect() const;

    Vector2 GetAnchorPoint(TextAlignment::T align) const;
    int GetLineHeight() const;
    text::Layout GetTextLayout(const char *text, TextAlignment::T alignemnt);
    //int TbGetCharacterIndex(Vector2 collision_pos, const char* text, TextAlignment::T alignemnt) const;
    //Rectangle TbGetTextRect(const char* text, TextAlignment::T alignemnt, int token_start, int token_end, int token_from) const;
    Rectangle TbMeasureText(const char* text, TextAlignment::T alignemnt) const;

    void _Advance(Vector2 pos, Vector2 size);
};

struct AtlasPos {
    int x, y;

    AtlasPos() = default;
    AtlasPos(int x, int y);
    Rectangle GetRect(int size) const;
};

#define MAX_BLOCKING_RECTS 20

struct UIGlobals {
    std::stack<TextBox> text_box_stack = std::stack<TextBox>();
    struct BlockingRect {Rectangle rec; uint8_t z;};
    BlockingRect acc_blocking_rects[MAX_BLOCKING_RECTS];
    BlockingRect blocking_rects[MAX_BLOCKING_RECTS];
    int blocking_rect_index = 0;
    int acc_blocking_rect_index = 0;

    struct MouseHints {
        float lock_progress = 0.0;
        Rectangle origin_button_rects[MAX_TOOLTIP_RECURSIONS+1];
        Rectangle hint_rects[MAX_TOOLTIP_RECURSIONS+1];
        char* hints[MAX_TOOLTIP_RECURSIONS+1] = { NULL };
        int count = 0;

        void AddHint(Rectangle origin_button, Vector2 anchor, const char *hint);
        static int Hash(Rectangle origin_button);
    } mousehints;

    void UIInit();
    void UIStart();
    void UIEnd();

    void _HandleMouseTips();

    void AddBlockingRect(Rectangle rect, uint8_t z_layer);
    // Relies on previous's frame information
    bool IsPointBlocked(Vector2 pos, uint8_t z_layer) const;
    const char* GetConceptDescription(const char* key);
    
    Texture2D GetIconAtlas();
    Texture2D GetIconAtlasSDF();
};

namespace ui {
    void PushTextBox(TextBox tb);
    void PushGlobal(int x, int y, int w, int h, int text_size, Color color, Color background, uint8_t z_layer);
    void CreateNew(int x, int y, int w, int h, int text_size, Color color, Color background);
    void PushMouseHint(Vector2 mouse_pos, int width, int height, uint8_t z_layer);

    int PushInset(int margin, int h);
    int PushScrollInset(int margin, int h, int allocated_height, int* scroll);
    void PushFree(int x, int y, int w, int h);
    void PushInline(int width, int height);
    void PushAligned(int width, int height, TextAlignment::T align);
    void PushHSplit(int x_start, int x_end);
    void PushGridCell(int columns, int rows, int column, int row);
    void Pop();

    ButtonStateFlags::T AsButton();
    void Enclose();
    void EncloseEx(int shrink, Color background_color, Color line_color, int corner_radius);
    void EnclosePartial(int inset, Color background_color, Color line_color, Direction::T directions);
    void EncloseDynamic(int shrink, Color background_color, Color line_color, int corner_radius);
    void Shrink(int dx, int dy);

    void DrawIcon(AtlasPos atlas_index, Color tint, int height);
    void DrawIconSDF(AtlasPos atlas_index, Color tint, int height);
    void Write(const char* text);
    void WriteEx(const char* text, TextAlignment::T alignemnt, bool linebreak);
    void DecorateEx(const text::Layout* layout, const TokenList* tokens);
    Rectangle MeasureTextEx(const char* text, TextAlignment::T alignemnt);
    void Fillline(double value, Color fill_color, Color background_color);
    void FilllineEx(int x_start, int x_end, int y, double value, Color fill_color, Color background_color);
    ButtonStateFlags::T DirectButton(const char* text, int inset);
    ButtonStateFlags::T ToggleButton(bool on);
    int DrawLimitedSlider(int current, int min, int max, int limit, Color fg, Color bg);
    void HelperText(const char* description);

    void BeginDirectDraw();
    void EndDirectDraw();

    void HSpace(int pixels);
    void VSpace(int pixels);
    void SetMouseHint(const char* text);

    Vector2 GetRelMousePos();
    TextBox* Current();
}

void HandleButtonSound(ButtonStateFlags::T button_state_flags);
Font GetCustomDefaultFont();

ButtonStateFlags::T DrawTriangleButton(Vector2 point, Vector2 base, double width, Color color);
ButtonStateFlags::T DrawCircleButton(Vector2 midpoint, double radius, Color color);

#endif  // UI_H