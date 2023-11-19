#include "ui.hpp"
#include "logging.hpp"
#include <stack>

Font default_font;

void DrawTextAligned(const char* text, Vector2 pos, TextAlignment alignment, Color c) {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, 16, 1);
    if (alignment & TEXT_ALIGNMENT_HCENTER) {
        pos.x -= size.x / 2;
    } else if (alignment & TEXT_ALIGNMENT_RIGHT) {
        pos.x -= size.x;
    } else {  // left - aligned
        // Do nothing
    }
    if (alignment & TEXT_ALIGNMENT_VCENTER) {
        pos.x -= size.x / 2;
    } else if (alignment & TEXT_ALIGNMENT_BOTTOM) {
        pos.x -= size.x;
    } else {  // left - aligned
        // Do nothing
    }
    Vector2 bottom_left = Vector2Subtract(pos, Vector2Scale(size, 0.5));
    DrawTextEx(GetCustomDefaultFont(), text, bottom_left, 16, 1, c);
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

TextBox::TextBox(int x, int y, int w, int h, int ptext_size, Color color) {
    ASSERT(w > 0)
    ASSERT(h > 0)
    text_start_x = x;
    text_start_y = y;
    text_margin_x = 2;
    text_margin_y = 2;
    text_size = ptext_size;
    text_counter = 0;
    text_color = color;
    width = w;
    height = h;
    x_cursor = 0;
    y_cursor = 0;
    line_size_x = 0;
    line_size_y = 0;
    text_background = BLANK;
}

void TextBox::_Advance(Vector2 size) {
    if (size.y > line_size_y) line_size_y = size.y;
    x_cursor += size.x + text_margin_x;
}

void TextBox::LineBreak() {
    x_cursor = 0;
    y_cursor += line_size_y + text_margin_y;
    line_size_x = 0;
    line_size_y = 0;
}

void TextBox::EnsureLineBreak() {
    if (x_cursor > 0) {
        y_cursor += line_size_y + text_margin_y;
        x_cursor = 0;
    }
    line_size_x = 0;
    line_size_y = 0;
}

void TextBox::Enclose(int inset_x, int inset_y, Color background_color, Color line_color) {
    Rectangle rect;
    rect.x = text_start_x - inset_x;
    rect.y = text_start_y - inset_y;
    rect.width = width + inset_x*2;
    rect.height = height + inset_y*2;
    int corner_radius = 0;//inset_x < inset_y ? inset_x : inset_y;
    DrawRectangleRounded(rect, corner_radius, 16, background_color);
    DrawRectangleRoundedLines(rect, corner_radius, 16, 1, line_color);
}

int TextBox::GetLineHeight() {
    return text_size + text_margin_y;
}

void TextBox::Write(const char* text) {
    Vector2 pos = {text_start_x + x_cursor, text_start_y + y_cursor};
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    if (text_background.a != 0) {
        DrawRectangleV(pos, size, text_background);
    }
    DrawTextEx(GetCustomDefaultFont(), text, pos, text_size, 1, text_color);
    _Advance(size);
}

void TextBox::WriteLine(const char* text) {
    Vector2 pos = {text_start_x + x_cursor, text_start_y + y_cursor};
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    if (text_background.a != 0) {
        DrawRectangleV(pos, size, text_background);
    }
    DrawTextEx(GetCustomDefaultFont(), text, pos, text_size, 1, text_color);
    _Advance(size);
    LineBreak();
}

ButtonStateFlags TextBox::WriteButton(const char* text, int inset) {
    Vector2 pos = {text_start_x + x_cursor, text_start_y + y_cursor + text_margin_y};
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    if (inset >= 0) {
        size.x += 2*inset;
        size.y += 2*inset;
        DrawRectangleLines(pos.x, pos.y, size.x, size.y, PALETTE_BLUE);
        pos.x += inset;
        pos.y += inset;
    }
    x_cursor += size.x + text_margin_x;
    DrawTextEx(GetCustomDefaultFont(), text, pos, text_size, 1, PALETTE_BLUE);
    bool is_in_area = CheckCollisionPointRec(GetMousePosition(), {pos.x, pos.y, size.x, size.y});
    _Advance(size);
    return _GetButtonState(is_in_area);
}

ButtonStateFlags TextBox::AsButton() {
    bool is_in_area = CheckCollisionPointRec(GetMousePosition(), {(float)text_start_x, (float)text_start_y, (float)width, (float)height});
    return _GetButtonState(is_in_area);
}

std::stack<TextBox> text_box_stack = std::stack<TextBox>();

void UIContextCreate(int x, int y, int w, int h, int text_size, Color color) {
    while (text_box_stack.size() > 0) {
        text_box_stack.pop();
    }
    TextBox new_text_box = TextBox(x, y, w, h, text_size, color);
    text_box_stack.push(new_text_box);
}

void UIContextPushInset(int margin, int h) {
    TextBox& tb = UIContextCurrent();
    tb.EnsureLineBreak();
    TextBox new_text_box = TextBox(
        tb.text_start_x + tb.x_cursor + margin,
        tb.text_start_y + tb.y_cursor + margin,
        tb.width - tb.x_cursor - 2*margin,
        fmin(tb.height - tb.y_cursor - 2*margin, h),
        tb.text_size,
        tb.text_color
    );
    tb.y_cursor += h + 2*margin;
    text_box_stack.push(new_text_box);
}

void UIContextPushHSplit(int x_start, int x_end) {
    TextBox& tb = UIContextCurrent();
    if (x_start < 0) x_start += tb.width;
    if (x_end < 0) x_end += tb.width;
    TextBox new_text_box = TextBox(
        tb.text_start_x + x_start,
        tb.text_start_y,
        x_end - x_start,
        tb.height,
        tb.text_size,
        tb.text_color
    );
    text_box_stack.push(new_text_box);
}

void UIContextPushGridCell(int columns, int rows, int column, int row) {
    TextBox& tb = UIContextCurrent();
    TextBox new_text_box = TextBox(
        tb.text_start_x + column * tb.width / columns,
        tb.text_start_y + row * tb.height / rows,
        tb.width / columns,
        tb.height / rows,
        tb.text_size,
        tb.text_color
    );
    text_box_stack.push(new_text_box);
}

ButtonStateFlags UIContextAsButton() {
    return UIContextCurrent().AsButton();
}

void UIContextPop() {
    text_box_stack.pop();
}

void UIContextEnclose(int inset_x, int inset_y, Color background_color, Color line_color) {
    UIContextCurrent().Enclose(inset_x, inset_y, background_color, line_color);
}

void UIContextWrite(const char* text, bool linebreak) {
    TextBox& tb = UIContextCurrent();
    if (linebreak) {
        tb.WriteLine(text);
    } else {
        tb.Write(text);
    }
}

void UIContextFillline(double value, Color fill_color, Color background_color) {
    TextBox& tb = UIContextCurrent();
    int y_end = tb.text_start_y + tb.height;
    int x_mid_point = tb.text_start_x + tb.width * value;
    DrawLine(tb.text_start_x, y_end, tb.text_start_x + tb.width, y_end, background_color);
    DrawLine(tb.text_start_x, y_end, x_mid_point, y_end, fill_color);
    DrawLine(x_mid_point, y_end, x_mid_point, y_end - 4, fill_color);
}

ButtonStateFlags UIContextDirectButton(const char* text, int inset) {
    TextBox& tb = UIContextCurrent();
    return tb.WriteButton(text, inset);
}


TextBox& UIContextCurrent() {
    return text_box_stack.top();
}

static char mouseover_text[1024] = "";

void UISetMouseHint(const char* text) {
    strncpy(mouseover_text, text, 1024);
}

void UIInit() {
    default_font = LoadFontEx("resources/fonts/OCRAEXT.TTF", 16, NULL, 256);
    //default_font = LoadFontEx("resources/fonts/GOTHIC.TTF", 16, NULL, 256);
}

void UIStart() {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    mouseover_text[0] = '\x00';
}

void UIEnd() {
    if (strlen(mouseover_text) > 0) {
        // Draw mouse
        Vector2 mouse_pos = GetMousePosition();
        Vector2 text_size = MeasureTextEx(GetCustomDefaultFont(), mouseover_text, 16, 1);
        DrawRectangleV(mouse_pos, text_size, BG_COLOR);
        DrawRectangleLines(mouse_pos.x, mouse_pos.y, text_size.x, text_size.y, MAIN_UI_COLOR);
        DrawTextEx(GetCustomDefaultFont(), mouseover_text, mouse_pos, 16, 1, MAIN_UI_COLOR);
    }
}

Font GetCustomDefaultFont() {
    return default_font;
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