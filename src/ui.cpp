#include "ui.hpp"

Font default_font;

void UIInit() {
    default_font = LoadFontEx("resources/fonts/OCRAEXT.TTF", 16, NULL, 256);
    //default_font = LoadFontEx("resources/fonts/GOTHIC.TTF", 16, NULL, 256);
}

Font GetCustomDefaultFont() {
    return default_font;
}

ButtonStateFlags _GetButtonState(bool is_in_area) {
    ButtonStateFlags res = BUTTON_STATE_FLAG_NONE;
    if (is_in_area) {
        SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
        res |= BUTTON_STATE_FLAG_HOVER;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) res |= BUTTON_STATE_FLAG_PRESSED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) res |= BUTTON_STATE_FLAG_JUST_PRESSED;
    }
    return res;
}

TextBox TextBoxMake(int x, int y, int w, int h, int text_size, Color color) {
    TextBox res = {0};
    res.text_start_x = x;
    res.text_start_y = y;
    res.text_margin_x = 2;
    res.text_margin_y = 2;
    res.text_size = text_size;
    res.text_counter = 0;
    res.text_color = color;
    res.width = w;
    res.height = h;
    res.x_cursor = 0;
    res.y_cursor = 0;
    res.line_size_x = 0;
    res.line_size_y = 0;
    res.text_background = BLANK;
    return res;
}

void _TextboxAdvance(TextBox* tb, Vector2 size) {
    if (size.y > tb->line_size_y) tb->line_size_y = size.y;
    tb->x_cursor += size.x + tb->text_margin_x;
}

void TextBoxLineBreak(TextBox* tb) {
    tb->x_cursor = 0;
    tb->y_cursor += tb->line_size_y + tb->text_margin_y;
    tb->line_size_x = 0;
    tb->line_size_y = 0;
}

void TextBoxEnclose(TextBox* tb, int inset_x, int inset_y, Color background_color, Color line_color) {
    Rectangle rect;
    rect.x = tb->text_start_x - inset_x;
    rect.y = tb->text_start_y - inset_y;
    rect.width = tb->width + inset_x*2;
    rect.height = tb->height + inset_y*2;
    int corner_radius = 0;//inset_x < inset_y ? inset_x : inset_y;
    DrawRectangleRounded(rect, corner_radius, 16, background_color);
    DrawRectangleRoundedLines(rect, corner_radius, 16, 1, line_color);
}

void TextBoxWrite(TextBox* tb, const char* text) {
    Vector2 pos = (Vector2) {tb->text_start_x + tb->x_cursor, tb->text_start_y + tb->y_cursor};
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, tb->text_size, 1);
    if (tb->text_background.a != 0) {
        DrawRectangleV(pos, size, tb->text_background);
    }
    DrawTextEx(GetCustomDefaultFont(), text, pos, tb->text_size, 1, tb->text_color);
    _TextboxAdvance(tb, size);
}

void TextBoxWriteLine(TextBox* tb, const char* text) {
    Vector2 pos = (Vector2) {tb->text_start_x + tb->x_cursor, tb->text_start_y + tb->y_cursor};
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, tb->text_size, 1);
    if (tb->text_background.a != 0) {
        DrawRectangleV(pos, size, tb->text_background);
    }
    DrawTextEx(GetCustomDefaultFont(), text, pos, tb->text_size, 1, tb->text_color);
    _TextboxAdvance(tb, size);
    TextBoxLineBreak(tb);
}

ButtonStateFlags TextBoxWriteButton(TextBox* tb, const char* text, int inset) {
    Vector2 pos = (Vector2) {tb->text_start_x + tb->x_cursor, tb->text_start_y + tb->y_cursor + tb->text_margin_y};
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, tb->text_size, 1);
    if (inset >= 0) {
        size.x += 2*inset;
        size.y += 2*inset;
        DrawRectangleLines(pos.x, pos.y, size.x, size.y, PALETTE_BLUE);
        pos.x += inset;
        pos.y += inset;
    }
    tb->x_cursor += size.x + tb->text_margin_x;
    DrawTextEx(GetCustomDefaultFont(), text, pos, tb->text_size, 1, PALETTE_BLUE);
    bool is_in_area = CheckCollisionPointRec(GetMousePosition(), (Rectangle) {pos.x, pos.y, size.x, size.y});
    _TextboxAdvance(tb, size);
    return _GetButtonState(is_in_area);
}

ButtonStateFlags DrawTriangleButton(Vector2 point, Vector2 base, double width, Color color) {
    Vector2 base_pos = Vector2Add(point, base);
    Vector2 tangent_dir = Vector2Rotate(Vector2Normalize(base), PI/2);
    Vector2 side_1 =  Vector2Add(base_pos, Vector2Scale(tangent_dir, -width));
    Vector2 side_2 =  Vector2Add(base_pos, Vector2Scale(tangent_dir, width));
    bool is_in_area = CheckCollisionPointTriangle(GetMousePosition(), side_1, point, side_2);
    if (is_in_area) {
        DrawTriangle(side_1, point, side_2, PALETTE_BLUE);
    } else {
        DrawTriangleLines(side_1, point, side_2, PALETTE_BLUE);
    }
    return _GetButtonState(is_in_area);
}

ButtonStateFlags DrawCircleButton(Vector2 midpoint, double radius, Color color) {
    bool is_in_area = CheckCollisionPointCircle(GetMousePosition(), midpoint, radius);
    if (is_in_area) {
        DrawCircleV(midpoint, radius, PALETTE_BLUE);
    } else {
        DrawCircleLines(midpoint.x, midpoint.y, radius, PALETTE_BLUE);
    }
    return _GetButtonState(is_in_area);
}