#ifndef UI_H
#define UI_H

#include "basic.hpp"

struct TextBox {
    int text_start_x;
    int text_start_y;
    int text_margin_x;
    int text_margin_y;
    int text_size;
    int text_counter;
    int width, height;
    int x_cursor;
    int y_cursor;
    int line_size_x;
    int line_size_y;
    Color text_color;
    Color text_background;
};

typedef uint8_t ButtonStateFlags;
const ButtonStateFlags BUTTON_STATE_FLAG_NONE = 0x00;
const ButtonStateFlags BUTTON_STATE_FLAG_HOVER = 0x01;
const ButtonStateFlags BUTTON_STATE_FLAG_PRESSED = 0x02;
const ButtonStateFlags BUTTON_STATE_FLAG_JUST_PRESSED = 0x04;

void UIInit();
Font GetCustomDefaultFont();

TextBox TextBoxMake(int x, int y, int w, int h, int text_size, Color color);

void TextBoxLineBreak(TextBox* tb);

void TextBoxEnclose(TextBox* tb, int inset_x, int inset_y, Color background_color, Color line_color);
void TextBoxWrite(TextBox* tb, const char* text);
void TextBoxWriteLine(TextBox* tb, const char* text);
ButtonStateFlags TextBoxWriteButton(TextBox* tb, const char* text, int inset);

ButtonStateFlags DrawTriangleButton(Vector2 point, Vector2 base, double width, Color color);
ButtonStateFlags DrawCircleButton(Vector2 midpoint, double radius, Color color);

#endif  // UI_H