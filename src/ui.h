#ifndef UI_H
#define UI_H

#include "basic.h"

STRUCT_DECL(TextBox) {
    int text_start_x;
    int text_start_y;
    int text_margin_x;
    int text_margin_y;
    int text_size;
    int text_counter;
    Color text_color;
};

ENUM_DECL(ButtonState) {
    BUTTON_NONE = 0x00,
    BUTTON_HOVER = 0x01,
    BUTTON_PRESSED = 0x02,
    BUTTON_JUST_PRESSED = 0x04,
};

TextBox TextBoxMake(int x, int y, int text_size, Color color);
void TextBoxWrite(TextBox* tb, const char* text);

ButtonState DrawTriangleButton(Vector2 point, Vector2 base, double width, Color color);
ButtonState DrawCircleButton(Vector2 midpoint, double radius, Color color);

void UIInit();
Font GetCustomDefaultFont();

#endif  // UI_H