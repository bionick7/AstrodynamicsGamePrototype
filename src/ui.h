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
    int width, height;
    Color text_color;
};

ENUM_DECL(ButtonState) {
    BUTTON_NONE = 0x00,
    BUTTON_HOVER = 0x01,
    BUTTON_PRESSED = 0x02,
    BUTTON_JUST_PRESSED = 0x04,
};

void UIInit();
Font GetCustomDefaultFont();

TextBox TextBoxMake(int x, int y, int text_size, Color text_color);
void TextBoxWrite(TextBox* tb, const char* text);
void TextBoxEnclose(TextBox* tb, int inset_x, int inset_y, Color background_color, Color line_color);

ButtonState DrawTriangleButton(Vector2 point, Vector2 base, double width, Color color);
ButtonState DrawCircleButton(Vector2 midpoint, double radius, Color color);

#endif  // UI_H