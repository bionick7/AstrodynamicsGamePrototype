#include "ui.h"

Font default_font;

void UIInit() {
    default_font = LoadFontEx("resources/fonts/OCRAEXT.TTF", 32, NULL, 256);
    //default_font = LoadFontEx("resources/fonts/GOTHIC.TTF", 16, NULL, 256);
}

Font GetCustomDefaultFont() {
    return default_font;
}

TextBox TextBoxMake(int x, int y, int w, int h, int text_size, Color color) {
    TextBox res = {0};
    res.text_start_x = x;
    res.text_start_y = y;
    res.text_size = text_size;
    res.text_counter = 0;
    res.text_color = color;
    res.width = w;
    res.height = h;
    res.y_cursor = 0;
    return res;
}

void TextBoxWrite(TextBox* tb, const char* text) {
    Vector2 pos = (Vector2) {tb->text_start_x, tb->text_start_y + tb->y_cursor + tb->text_margin_y};
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, tb->text_size, 1);
    tb->y_cursor += size.y + tb->text_margin_y;
    DrawTextEx(GetCustomDefaultFont(), text, pos, tb->text_size, 1, tb->text_color);
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


ButtonState DrawTriangleButton(Vector2 point, Vector2 base, double width, Color color) {
    Vector2 base_pos = Vector2Add(point, base);
    Vector2 tangent_dir = Vector2Rotate(Vector2Normalize(base), PI/2);
    Vector2 side_1 =  Vector2Add(base_pos, Vector2Scale(tangent_dir, -width));
    Vector2 side_2 =  Vector2Add(base_pos, Vector2Scale(tangent_dir, width));
    ButtonState res = 0;
    if (CheckCollisionPointTriangle(GetMousePosition(), side_1, point, side_2)) {
        DrawTriangle(side_1, point, side_2, color);
        res |= BUTTON_HOVER;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) res |= BUTTON_PRESSED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) res |= BUTTON_JUST_PRESSED;
    } else {
        DrawTriangleLines(side_1, point, side_2, color);
    }
    return res;
}

ButtonState DrawCircleButton(Vector2 midpoint, double radius, Color color) {
    ButtonState res = 0;
    if (CheckCollisionPointCircle(GetMousePosition(), midpoint, radius)) {
        DrawCircleV(midpoint, radius, color);
        res |= BUTTON_HOVER;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) res |= BUTTON_PRESSED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) res |= BUTTON_JUST_PRESSED;
    } else {
        DrawCircleLines(midpoint.x, midpoint.y, radius, color);
    }
    return res;
}