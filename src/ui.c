#include "ui.h"

Font default_font;

void BasicInit() {
    default_font = LoadFontEx("resources/fonts/OCRAEXT.TTF", 32, NULL, 256);
    //default_font = LoadFontEx("resources/fonts/GOTHIC.TTF", 16, NULL, 256);
}

TextBox TextBoxMake(int x, int y, int size, Color color) {
    TextBox res = {0};
    res.text_start_x = x;
    res.text_start_y = y;
    res.text_size = size;
    res.text_counter = 0;
    res.text_color = color;
    return res;
}

void TextBoxWrite(TextBox* tb, const char* text) {
    Vector2 pos = (Vector2) {tb->text_start_x, (tb->text_size + tb->text_margin_y) * tb->text_counter++ + tb->text_start_y};
    DrawTextEx(GetCustomDefaultFont(), text, pos, tb->text_size, 1, tb->text_color);
}

Font GetCustomDefaultFont() {
    return default_font;
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