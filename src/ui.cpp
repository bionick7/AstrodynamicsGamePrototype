#include "ui.hpp"
#include "logging.hpp"
#include "constants.hpp"
#include "audio_server.hpp"
#include <stack>

Font default_font;

// Copy of raylib's DrawTextEx(), but will not draw over a certain rectangle
void DrawTextConstrained(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint, Rectangle render_rect) {
    if (font.texture.id == 0) font = GetFontDefault();  // (Raylib cmt) Security check in case of not valid font

    int size = TextLength(text);    // (Raylib cmt) Total size in bytes of the text, scanned by codepoints in loop

    int textOffsetY = 0;            // (Raylib cmt) Offset between lines (on linebreak '\n')
    float textOffsetX = 0.0f;       // (Raylib cmt) Offset X to next character to draw

    float scaleFactor = fontSize/font.baseSize;         // (Raylib cmt) Character quad scaling factor

    for (int i = 0; i < size;)
    {
        // (Raylib cmt)Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        if (codepoint == '\n')
        {
            // (Raylib cmt) NOTE: Line spacing is a global variable, use SetTextLineSpacing() to setup
            textOffsetY += 20;  // TODO, raylib doesn't expose line space, so how to?
            textOffsetX = 0.0f;
        }
        else
        {
            float x_increment = (float)font.recs[index].width*scaleFactor + spacing;
            Vector2 char_pos = (Vector2){ position.x + textOffsetX, position.y + textOffsetY };
            Vector2 char_pos2 = (Vector2){ position.x + textOffsetX + x_increment, position.y + textOffsetY + fontSize };
            if (
                (codepoint != ' ') && (codepoint != '\t')
                && CheckCollisionPointRec(char_pos, render_rect)
                && CheckCollisionPointRec(char_pos2, render_rect)
            ) {
                DrawTextCodepoint(font, codepoint, char_pos, fontSize, tint);
            }

            if (font.glyphs[index].advanceX == 0) textOffsetX += x_increment;
            else textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }
}

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
        pos.y -= size.y / 2;
    } else if (alignment & TEXT_ALIGNMENT_BOTTOM) {
        pos.y -= size.y;
    } else {  // top - aligned
        // Do nothing
    }
    //Vector2 bottom_left = Vector2Subtract(pos, Vector2Scale(size, 0.5));
    DrawTextEx(GetCustomDefaultFont(), text, pos, 16, 1, c);
}

ButtonStateFlags _GetButtonState(bool is_in_area, bool was_in_area) {
    ButtonStateFlags res = BUTTON_STATE_FLAG_NONE;
    if (is_in_area) {
        SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
        res |= BUTTON_STATE_FLAG_HOVER;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))     res |= BUTTON_STATE_FLAG_PRESSED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  res |= BUTTON_STATE_FLAG_JUST_PRESSED;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) res |= BUTTON_STATE_FLAG_JUST_UNPRESSED;

        if (!was_in_area) res |= BUTTON_STATE_FLAG_JUST_HOVER_IN;
    }
    if (was_in_area && !is_in_area) res |= BUTTON_STATE_FLAG_JUST_HOVER_OUT;

    return res;
}

void HandleButtonSound(ButtonStateFlags state) {
    if (state & BUTTON_STATE_FLAG_JUST_PRESSED) {
        PlaySFX(SFX_CLICK_BUTTON);
    }
    if (state & BUTTON_STATE_FLAG_JUST_HOVER_IN) {
        PlaySFX(SFX_CLICK_SHORT);
    }
}

TextBox::TextBox(int x, int y, int w, int h, int ptext_size, Color color) {
    ASSERT(w > 0)
    ASSERT(h >= 0)
    text_start_x = x;
    text_start_y = y;
    text_margin_x = 2;
    text_margin_y = 2;

    render_rec.x = x;
    render_rec.y = y;
    render_rec.width = w;
    render_rec.height = h;

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
    rect = GetCollisionRec(rect, render_rec);
    DrawRectangleRounded(rect, corner_radius, 16, background_color);
    DrawRectangleRoundedLines(rect, corner_radius, 16, 1, line_color);
}

void TextBox::DebugDrawRenderRec() const {
    DrawRectangleRec(render_rec, BLACK);
    DrawRectangleLinesEx(render_rec, 4, PURPLE);
}


int TextBox::GetLineHeight() const {
    return text_size + text_margin_y;
}

void TextBox::Write(const char* text) {
    Vector2 pos = {text_start_x + x_cursor, text_start_y + y_cursor};
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    // text fully in render rectangle
    /*if (
        !CheckCollisionPointRec(pos, render_rec)
        || !CheckCollisionPointRec(Vector2Add(pos, size), render_rec)
    ) return;*/

    // avoid drawing if the bg is fully transparent (most cases)
    if(text_background.a != 0) {
        DrawRectangleV(pos, size, text_background);
    }
    DrawTextConstrained(GetCustomDefaultFont(), text, pos, text_size, 1, text_color, render_rec);
    _Advance(size);
}

void TextBox::WriteLine(const char* text) {
    Vector2 pos = {text_start_x + x_cursor, text_start_y + y_cursor};
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    // text fully in render rectangle
    /*if (
        !CheckCollisionPointRec(pos, render_rec)
        || !CheckCollisionPointRec(Vector2Add(pos, size), render_rec)
    ) return;*/

    if (text_background.a != 0) {
        DrawRectangleV(pos, size, text_background);
    }
    DrawTextConstrained(GetCustomDefaultFont(), text, pos, text_size, 1, text_color, render_rec);
    _Advance(size);
    LineBreak();
}

ButtonStateFlags TextBox::WriteButton(const char* text, int inset) {
    Vector2 pos = {text_start_x + x_cursor, text_start_y + y_cursor + text_margin_y};
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    // text fully in render rectangle
    if (
        !CheckCollisionPointRec(pos, render_rec)
        || !CheckCollisionPointRec(Vector2Add(pos, size), render_rec)
    ) return BUTTON_STATE_FLAG_DISABLED;

    if (inset >= 0) {
        size.x += 2*inset;
        size.y += 2*inset;
        pos.x += inset;
        pos.y += inset;
    }
    bool is_in_area = CheckCollisionPointRec(GetMousePosition(), {pos.x, pos.y, size.x, size.y});
    ButtonStateFlags res = _GetButtonState(
        is_in_area,
        CheckCollisionPointRec(Vector2Subtract(GetMousePosition(), GetMouseDelta()), {pos.x, pos.y, size.x, size.y})
    );
    Color c = is_in_area ? MAIN_UI_COLOR : PALETTE_BLUE;
    DrawRectangleLines(pos.x - inset, pos.y - inset, size.x, size.y, c);
    DrawTextEx(GetCustomDefaultFont(), text, pos, text_size, 1, c);
    _Advance(size);
    return res;
}

ButtonStateFlags TextBox::AsButton() const {
    return _GetButtonState(
        CheckCollisionPointRec(GetMousePosition(), {(float)text_start_x, (float)text_start_y, (float)width, (float)height}),
        CheckCollisionPointRec(Vector2Subtract(GetMousePosition(), GetMouseDelta()), {(float)text_start_x, (float)text_start_y, (float)width, (float)height})
    );
}

std::stack<TextBox> text_box_stack = std::stack<TextBox>();

void UIContextCreate(int x, int y, int w, int h, int text_size, Color color) {
    while (text_box_stack.size() > 0) {
        text_box_stack.pop();
    }
    TextBox new_text_box = TextBox(x, y, w, h, text_size, color);
    text_box_stack.push(new_text_box);
}

int UIContextPushInset(int margin, int h) {
    // Returns the actual height
    TextBox& tb = UIContextCurrent();
    tb.EnsureLineBreak();
    int height = fmin(h, fmax(0, tb.height - tb.y_cursor - 2*margin));
    TextBox new_text_box = TextBox(
        tb.text_start_x + tb.x_cursor + margin,
        tb.text_start_y + tb.y_cursor + margin,
        tb.width - tb.x_cursor - 2*margin,
        height,
        tb.text_size,
        tb.text_color
    );
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb.render_rec);

    tb.y_cursor += h + 2*margin;
    text_box_stack.push(new_text_box);
    return height;
}

int UIContextPushScrollInset(int margin, int h, int allocated_height, int scroll) {
    // Returns the actual height
    const int buildin_scrollbar_margin = 3;
    const int buildin_scrollbar_width = 6;

    TextBox& tb = UIContextCurrent();
    tb.EnsureLineBreak();
    int height = fmin(h, fmax(0, tb.height - tb.y_cursor - 2*margin));
    float scroll_progress = Clamp((float)scroll / (allocated_height - h), 0, 1);
    int scrollbar_height = h * h / allocated_height;
    DrawRectangleRounded({
        (float) tb.text_start_x + tb.width - buildin_scrollbar_margin,
        (float) tb.text_start_y + scroll_progress * (h - scrollbar_height),
        (float) buildin_scrollbar_width,
        (float) scrollbar_height},
         buildin_scrollbar_width/2,
        4, tb.text_color
    );
    TextBox new_text_box = TextBox(
        tb.text_start_x + tb.x_cursor + margin,
        tb.text_start_y + tb.y_cursor + margin - scroll,
        tb.width - tb.x_cursor - 2*margin - 2*buildin_scrollbar_margin - buildin_scrollbar_width,
        allocated_height,
        tb.text_size,
        tb.text_color
    );
    new_text_box.render_rec.y = tb.text_start_y + tb.y_cursor + margin;
    new_text_box.render_rec.height = height;

    tb.y_cursor += h + 2*margin;
    text_box_stack.push(new_text_box);
    return height;
}

void UIContextPushInline(int margin) {
    TextBox& tb = UIContextCurrent();
    TextBox new_text_box = TextBox(
        tb.text_start_x + tb.x_cursor + margin,
        tb.text_start_y,
        tb.width - tb.x_cursor - 2*margin,
        tb.height,
        tb.text_size,
        tb.text_color
    );
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb.render_rec);

    tb.x_cursor = tb.width;
    text_box_stack.push(new_text_box);
}

void UIContextPushAligned(int width, int height, TextAlignment alignment) {
    TextBox& tb = UIContextCurrent();
    tb.EnsureLineBreak();

    int x = tb.text_start_x;
    int y = tb.text_start_y;
    if (alignment & TEXT_ALIGNMENT_HCENTER) {
        x += (tb.width - width) / 2;
    } else if (alignment & TEXT_ALIGNMENT_RIGHT) {
        x += tb.width - width;
    } else {  // left - aligned
        // Do nothing
    }
    if (alignment & TEXT_ALIGNMENT_VCENTER) {
        y += (tb.height - height) / 2;
    } else if (alignment & TEXT_ALIGNMENT_BOTTOM) {
        y += (tb.height - height);
    } else {  // top - aligned
        // Do nothing
    }

    TextBox new_text_box = TextBox(
        x, y, width, height,
        tb.text_size,
        tb.text_color
    );

    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb.render_rec);
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
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb.render_rec);

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
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb.render_rec);

    text_box_stack.push(new_text_box);
}

ButtonStateFlags UIContextAsButton() {
    return UIContextCurrent().AsButton();
}

void UIContextPop() {
    text_box_stack.pop();
}

Vector2 UIContextGetRelMousePos() {
    return {
        GetMousePosition().x - UIContextCurrent().text_start_x,
        GetMousePosition().y - UIContextCurrent().text_start_y
    };
}

void UIContextShrink(int dx, int dy) {
    UIContextCurrent().text_start_x += dx;
    UIContextCurrent().text_start_y += dy;
    UIContextCurrent().width -= 2*dx;
    UIContextCurrent().height -= 2*dy;
}

void UIContextEnclose(Color background_color, Color line_color) {
    UIContextCurrent().Enclose(1, 1, background_color, line_color);
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
    ButtonStateFlags button_state = tb.WriteButton(text, inset);
    HandleButtonSound(button_state);
    return button_state;
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
    return _GetButtonState(
        is_in_area,
        CheckCollisionPointTriangle(Vector2Subtract(GetMousePosition(), GetMouseDelta()), side_1, point, side_2)
    );
}

ButtonStateFlags DrawCircleButton(Vector2 midpoint, double radius, Color color) {
    bool is_in_area = CheckCollisionPointCircle(GetMousePosition(), midpoint, radius);
    if (is_in_area) {
        DrawCircleV(midpoint, radius, PALETTE_BLUE);
    } else {
        DrawCircleLines(midpoint.x, midpoint.y, radius, PALETTE_BLUE);
    }
    return _GetButtonState(
        is_in_area,
        CheckCollisionPointCircle(Vector2Subtract(GetMousePosition(), GetMouseDelta()), midpoint, radius)
    );
}