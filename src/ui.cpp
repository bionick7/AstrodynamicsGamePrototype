#include "ui.hpp"
#include "logging.hpp"
#include "constants.hpp"
#include "audio_server.hpp"
#include "global_state.hpp"
#include "utils.hpp"
#include "debug_console.hpp"
#include "debug_drawing.hpp"
#include "render_utils.hpp"

Vector2 ApplyAlignment(Vector2 anchorpoint, Vector2 size, TextAlignment::T alignment) {
    switch(alignment & TextAlignment::HFILTER){
        case TextAlignment::HCENTER: anchorpoint.x -= size.x/2; break;
        case TextAlignment::RIGHT: anchorpoint.x -= size.x; break;
    }
    switch(alignment & TextAlignment::VFILTER){
        case TextAlignment::VCENTER: anchorpoint.y -= size.y/2; break;
        case TextAlignment::BOTTOM: anchorpoint.y -= size.y; break;
    }
    return anchorpoint;
}

Rectangle DrawTextAligned(const char* text, Vector2 pos, TextAlignment::T alignment, Color c, uint8_t z_layer) {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, DEFAULT_FONT_SIZE, 1);
    pos = ApplyAlignment(pos, size, alignment);
    //Vector2 bottom_left = Vector2Subtract(pos, Vector2Scale(size, 0.5));
    Rectangle rect = { pos.x, pos.y, size.x, size.y };
    InternalDrawTextEx(GetCustomDefaultFont(), text, pos, DEFAULT_FONT_SIZE, 1, c, GetScreenRect(), z_layer);
    return rect;
}

ButtonStateFlags::T GetButtonState(bool is_in_area, bool was_in_area) {
    ButtonStateFlags::T res = ButtonStateFlags::NONE;
    if (is_in_area) {
        res |= ButtonStateFlags::HOVER;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))     res |= ButtonStateFlags::PRESSED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  res |= ButtonStateFlags::JUST_PRESSED;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) res |= ButtonStateFlags::JUST_UNPRESSED;

        if (!was_in_area) res |= ButtonStateFlags::JUST_HOVER_IN;
    }
    if (was_in_area && !is_in_area) res |= ButtonStateFlags::JUST_HOVER_OUT;

    return res;
}

void HandleButtonSound(ButtonStateFlags::T state) {
    if (state & ButtonStateFlags::JUST_PRESSED) {
        PlaySFX(SFX_CLICK_BUTTON);
    }
    if (state & ButtonStateFlags::JUST_HOVER_IN) {
        PlaySFX(SFX_CLICK_SHORT);
    }
}

TextBox::TextBox(int x, int y, int w, int h, int ptext_size, Color color, Color pbackground_color, uint8_t pz_layer) {
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
    z_layer = pz_layer;

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
    background_color = pbackground_color;
}

TextBox::TextBox(const TextBox *parent, int x, int y, int w, int h) : 
    TextBox::TextBox(x, y, w, h, parent->text_size, parent->text_color, 
                     parent->background_color, parent->z_layer) {}

int TextBox::GetCharWidth() {
    Font font = GetCustomDefaultFont();
    const char* test_string = "abcdefghiJKLMNOP0123";
    return MeasureTextEx(GetCustomDefaultFont(), test_string, text_size, 1).x / strlen(test_string);
}

void TextBox::_Advance(Vector2 pos, Vector2 size) {
    if (size.y > line_size_y) line_size_y = size.y;
    if (pos.x + size.x > text_start_x + x_cursor) x_cursor = pos.x + size.x - text_start_x;
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

void TextBox::Enclose(int inset, int corner_radius, Color background_color, Color line_color) {
    Rectangle rect;
    rect.x = text_start_x;
    rect.y = text_start_y;
    rect.width = width;
    rect.height = height;

    float roundness = corner_radius * 2.0f / fmin(rect.width, rect.height);
    rect = GetCollisionRec(rect, render_rec);
    BeginRenderInUIMode(z_layer);
    DrawRectangleRounded(rect, roundness, 16, background_color);
    DrawRectangleRoundedLines(rect, roundness, 16, 1, line_color);
    EndRenderInUIMode();
    Shrink(inset, inset);
}

void TextBox::EnclosePartial(int inset, Color background_color, Color line_color, Direction::T directions) {
    Rectangle rect;
    rect.x = text_start_x;
    rect.y = text_start_y;
    rect.width = width;
    rect.height = height;

    BeginRenderInUIMode(z_layer);
    DrawRectangleRounded(rect, 0, 1, background_color);
    if (directions & Direction::TOP) {
        DrawLine(rect.x, rect.y, rect.x + width, rect.y, line_color);
    }
    if (directions & Direction::DOWN) {
        DrawLine(rect.x, rect.y + rect.height, rect.x + width, rect.y + rect.height, line_color);
    }
    if (directions & Direction::LEFT) {
        DrawLine(rect.x, rect.y, rect.x, rect.y + rect.height, line_color);
    }
    if (directions & Direction::RIGHT) {
        DrawLine(rect.x + width, rect.y, rect.x + width, rect.y + rect.height, line_color);
    }
    EndRenderInUIMode();

    rect = GetCollisionRec(rect, render_rec);
    Shrink(inset, inset);
}

void TextBox::Shrink(int dx, int dy) {
    text_start_x += dx;
    text_start_y += dy;
    width -= 2*dx;
    height -= 2*dy;
}

void TextBox::WriteRaw(const char *text, TextAlignment::T align) {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    Vector2 pos = ApplyAlignment(GetAnchorPoint(align), size, align);
    // avoid drawing text_background is fully transparent (useless)
    if(text_background.a != 0) {
        BeginRenderInUIMode(z_layer);
        DrawRectangleRec(GetCollisionRec({pos.x, pos.y, size.x, size.y}, render_rec), text_background);
        EndRenderInUIMode();
    }
    InternalDrawTextEx(GetCustomDefaultFont(), text, pos, text_size, 1, text_color, render_rec, z_layer);
    if (GetSettingBool("draw_textrects", false)) {
        BeginRenderInUIMode(z_layer);
        DrawRectangleLinesEx({pos.x, pos.y, size.x, size.y}, 1, RED);
        EndRenderInUIMode();
    }
    // Not Advance
}

void TextBox::Write(const char* text, TextAlignment::T align) {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    Vector2 pos = ApplyAlignment(GetAnchorPoint(align), size, align);
    // avoid drawing text_background is fully transparent (useless)
    if(text_background.a != 0) {
        BeginRenderInUIMode(z_layer);
        DrawRectangleRec(GetCollisionRec({ pos.x, pos.y, size.x, size.y }, render_rec), text_background);
        EndRenderInUIMode();
    }
    InternalDrawTextEx(GetCustomDefaultFont(), text, pos, text_size, 1, text_color, render_rec, z_layer);
    if (GetSettingBool("draw_textrects", false)) {
        BeginRenderInUIMode(z_layer);
        DrawRectangleLinesEx({pos.x, pos.y, size.x, size.y}, 1, RED);
        EndRenderInUIMode();
    }
    _Advance(pos, size);
}

void TextBox::WriteLine(const char* text, TextAlignment::T align) {
    Write(text, align);
    LineBreak();
}

void TextBox::Decorate(const char *text, TextAlignment::T align, const TokenList* tokens) {
    /*int len = strlen(text);
    char* underline_txt = new char[len+1];
    for (int i=0; i < len; i++) {
        underline_txt[i] = ' ';
    }
    underline_txt[len+1] = '\0'
    for (int i=0; i < tokens->length; i++) {
        for (int j=tokens->start_positions[i]; j < tokens->end_positions[i]; j++) {
            underline_txt[j] = '_';
        }
    }
    WriteRaw(underline_txt, align);
    delete[] underline_txt;*/
    std::vector rr = GetTextRects(
        tokens, GetCustomDefaultFont(),
        text, text_size, 1
    );
    BeginRenderInUIMode(z_layer);
    for (Rectangle rec : rr) {
        int y = text_start_y + rec.y + rec.height;
        DrawLine(text_start_x + rec.x, y, text_start_x + rec.x + rec.width, y, text_color);
    }
    EndRenderInUIMode();
}

void TextBox::DrawTexture(Texture2D texture, Rectangle source, int texture_height, Color tint, bool sdf) {
    Vector2 pos = {text_start_x + x_cursor, text_start_y + y_cursor + text_margin_y};
    int texture_width = texture_height * source.width / source.height;
    Rectangle destination = { pos.x, pos.y, (float)texture_width, (float)texture_height };
    bool outside_render_rect = !CheckCollisionRecs(render_rec, destination);
    bool inside_render_rect = 
        render_rec.x > destination.x && render_rec.x + render_rec.width  < destination.x + destination.width &&
        render_rec.y > destination.y && render_rec.y + render_rec.height < destination.y + destination.height;
    if (outside_render_rect) {
        return;
    }
    if (!inside_render_rect) {
        BeginScissorMode(render_rec.x, render_rec.y, render_rec.width, render_rec.height);
    }

    if (sdf) {
        DrawTextureSDF(texture, source, destination, Vector2Zero(), 0, tint, z_layer);
    } else {
        BeginRenderInUIMode(z_layer);
        DrawTexturePro(texture, source, destination, Vector2Zero(), 0, tint);
        EndRenderInUIMode();
    }
    if (!inside_render_rect) {
        EndScissorMode();
    }
    x_cursor = 0;
    y_cursor += height;
}

ButtonStateFlags::T TextBox::WriteButton(const char* text, int inset) {
    Vector2 pos = {text_start_x + x_cursor, text_start_y + y_cursor + text_margin_y};
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    // text fully in render rectangle
    if (
        !CheckCollisionPointRec(pos, render_rec)
        || !CheckCollisionPointRec(Vector2Add(pos, size), render_rec)
    ) return ButtonStateFlags::DISABLED;

    if (inset >= 0) {
        size.x += 2*inset;
        size.y += 2*inset;
        pos.x += inset;
        pos.y += inset;
    }
    bool is_in_area = CheckCollisionPointRec(GetMousePosition(), {pos.x, pos.y, size.x, size.y});
    bool was_in_area = CheckCollisionPointRec(Vector2Subtract(GetMousePosition(), GetMouseDelta()), {pos.x, pos.y, size.x, size.y});
    ButtonStateFlags::T res = GetButtonState(is_in_area, was_in_area);
    Color c = is_in_area ? Palette::ui_main : Palette::interactable_main;
    DrawRectangleLines(pos.x - inset, pos.y - inset, size.x, size.y, c);
    InternalDrawTextEx(GetCustomDefaultFont(), text, pos, text_size, 1, text_color, render_rec, z_layer);
    _Advance(pos, size);
    return res;
}

ButtonStateFlags::T TextBox::AsButton() const {
    return GetButtonState(
        CheckCollisionPointRec(GetMousePosition(), {(float)text_start_x, (float)text_start_y, (float)width, (float)height}),
        CheckCollisionPointRec(Vector2Subtract(GetMousePosition(), GetMouseDelta()), {(float)text_start_x, (float)text_start_y, (float)width, (float)height})
    );
}


int TextBox::GetLineHeight() const {
    return text_size + text_margin_y;
}

int TextBox::TbGetCharacterIndex(Vector2 collision_pos, const char *text, TextAlignment::T alignemnt) const {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    Vector2 pos = ApplyAlignment(GetAnchorPoint(alignemnt), size, alignemnt);

    if (!CheckCollisionPointRec(collision_pos, { pos.x, pos.y, size.x, size.y })) {
        return -1;
    }
    return GetCharacterIndex(
        Vector2Subtract(collision_pos, pos),
        GetCustomDefaultFont(),
        text, text_size, 1
    );
}

Rectangle TextBox::TbGetTextRect(const char *text, TextAlignment::T alignemnt, int token_start, int token_end, int token_from) const {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    Vector2 pos = ApplyAlignment(GetAnchorPoint(alignemnt), size, alignemnt);

    Rectangle rect = GetTextRect(
        token_start, token_end, token_from,
        GetCustomDefaultFont(),
        text, text_size, 1
    );

    rect.x += pos.x;
    rect.y += pos.y;

    return rect;
}

Vector2 TextBox::GetAnchorPoint(TextAlignment::T align) const {
    Vector2 res = { (float)text_start_x, (float)text_start_y };
    switch(align & TextAlignment::HFILTER){
        case TextAlignment::HCENTER: res.x += width/2; break;
        case TextAlignment::RIGHT: res.x += width; break;
        case TextAlignment::HCONFORM: res.x += x_cursor; break;
    }
    switch(align & TextAlignment::VFILTER){
        case TextAlignment::VCENTER: res.y += height/2; break;
        case TextAlignment::BOTTOM: res.y += height; break;
        case TextAlignment::VCONFORM: res.y += y_cursor; break;
    }
    return res;
}

Rectangle TextBox::TbMeasureText(const char* text, TextAlignment::T alignemnt) const {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    Vector2 pos = ApplyAlignment(GetAnchorPoint(alignemnt), size, alignemnt);
    return {pos.x, pos.y, size.x, size.y};
}

void ui::PushTextBox(TextBox tb) {
    GetUI()->text_box_stack.push(tb);
    if (GetSettingBool("draw_renderrects", false)) {
        BeginRenderInUIMode(tb.z_layer);
        DrawRectangleLinesEx(tb.render_rec, 1, GREEN);
        EndRenderInUIMode();
    }
}

void ui::PushGlobal(int x, int y, int w, int h, int text_size, Color color, Color background, uint8_t z_layer) {
    TextBox new_text_box = TextBox(x, y, w, h, text_size, color, background, z_layer);
    new_text_box.z_layer = z_layer;
    ui::PushTextBox(new_text_box);
    GetUI()->AddBlockingRect({(float)x, (float)y, (float)w, (float)h}, z_layer);
}

void ui::CreateNew(int x, int y, int w, int h, int text_size, Color color, Color background) {
    while (GetUI()->text_box_stack.size() > 0) {  // Clear stack
        ui::Pop();
    }
    ui::PushGlobal(x, y, w, h, text_size, color, background, 10);
}

void ui::PushMouseHint(Vector2 mousepos, int width, int height, uint8_t z_layer) {
    const int border_margin = 2;
    int x_pos = ClampInt(mousepos.x, border_margin, GetScreenWidth() - width - border_margin);
    int y_pos = ClampInt(mousepos.y, border_margin, GetScreenHeight() - height - border_margin);
    ui::PushGlobal(x_pos, y_pos, width, height, DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, z_layer);
}

int ui::PushInset(int margin, int h) {
    // Returns the actual height
    h += 8;  // straight up allocating extra space for magins
    TextBox* tb = ui::Current();
    tb->EnsureLineBreak();
    h = fmin(h, fmax(0, tb->height - tb->y_cursor - 2*margin));
    TextBox new_text_box = TextBox(tb,
        tb->text_start_x + tb->x_cursor + margin,
        tb->text_start_y + tb->y_cursor + margin,
        tb->width - tb->x_cursor - 2*margin,
        h
    );
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb->render_rec);

    tb->y_cursor += h + 2*margin;
    ui::PushTextBox(new_text_box);
    return h;
}

int ui::PushScrollInset(int margin, int h, int allocated_height, int* scroll) {
    // Returns the actual height
    int buildin_scrollbar_margin = 3;
    int buildin_scrollbar_width = 6;

    if (ui::AsButton() & ButtonStateFlags::HOVER) {
        int max_scroll = MaxInt(allocated_height - ui::Current()->height, 0);
        *scroll = ClampInt(*scroll - GetMouseWheelMove() * 20, 0, max_scroll);
    }

    TextBox* tb = ui::Current();
    tb->EnsureLineBreak();
    int height = fmin(h, fmax(0, tb->height - tb->y_cursor - 2*margin));
    float scroll_progress = Clamp((float)(*scroll) / (allocated_height - h), 0, 1);
    int scrollbar_height = h * h / allocated_height;
    if (allocated_height > h) {
        BeginRenderInUIMode(tb->z_layer + 1);
        DrawRectangleRounded({
            (float) tb->text_start_x + tb->width - buildin_scrollbar_margin,
            (float) tb->text_start_y + scroll_progress * (h - scrollbar_height),
            (float) buildin_scrollbar_width,
            (float) scrollbar_height},
            buildin_scrollbar_width/2,
            1, tb->text_color
        );
        EndRenderInUIMode();
    } else {
        buildin_scrollbar_margin = 0;
        buildin_scrollbar_width = 0;
    }
    TextBox new_text_box = TextBox(tb,
        tb->text_start_x + tb->x_cursor + margin,
        tb->text_start_y + tb->y_cursor + margin - *scroll,
        tb->width - tb->x_cursor - 2*margin - 2*buildin_scrollbar_margin - buildin_scrollbar_width,
        allocated_height
    );
    new_text_box.render_rec.y = tb->text_start_y + tb->y_cursor + margin;
    new_text_box.render_rec.height = height;

    tb->y_cursor += h + 2*margin;
    ui::PushTextBox(new_text_box);
    return height;
}

void ui::PushInline(int width, int height) {
    TextBox* tb = ui::Current();
    if (width > tb->width - tb->x_cursor)
        width = tb->width - tb->x_cursor;
    TextBox new_tb = TextBox(tb,
        tb->text_start_x + tb->x_cursor,
        tb->text_start_y + tb->y_cursor,
        width,
        height + 8
    );
    new_tb.render_rec = GetCollisionRec(new_tb.render_rec, tb->render_rec);

    tb->_Advance(
        {(float)new_tb.text_start_x, (float)new_tb.text_start_y}, 
        {(float)new_tb.width, (float)new_tb.height}
    );
    ui::PushTextBox(new_tb);
}

void ui::PushAligned(int width, int height, TextAlignment::T alignment) {
    TextBox* tb = ui::Current();
    tb->EnsureLineBreak();

    int x = tb->text_start_x;
    int y = tb->text_start_y;
    if (alignment & TextAlignment::HCENTER) {
        x += (tb->width - width) / 2;
    } else if (alignment & TextAlignment::RIGHT) {
        x += tb->width - width;
    } else {  // left - aligned
        // Do nothing
    }
    if (alignment & TextAlignment::VCENTER) {
        y += (tb->height - height) / 2;
    } else if (alignment & TextAlignment::BOTTOM) {
        y += (tb->height - height);
    } else {  // top - aligned
        // Do nothing
    }

    TextBox new_text_box = TextBox(tb, x, y, width, height);
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb->render_rec);
    ui::PushTextBox(new_text_box);
}

void ui::PushHSplit(int x_start, int x_end) {
    TextBox* tb = ui::Current();
    if (x_start < 0) x_start += tb->width;
    if (x_end < 0) x_end += tb->width;
    TextBox new_text_box = TextBox(tb,
        tb->text_start_x + x_start,
        tb->text_start_y,
        x_end - x_start,
        tb->height
    );
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb->render_rec);
    ui::PushTextBox(new_text_box);
}

void ui::PushGridCell(int columns, int rows, int column, int row) {
    TextBox* tb = ui::Current();
    TextBox new_text_box = TextBox(tb,
        tb->text_start_x + column * tb->width / columns,
        tb->text_start_y + row * tb->height / rows,
        tb->width / columns,
        tb->height / rows
    );
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb->render_rec);
    ui::PushTextBox(new_text_box);
}

ButtonStateFlags::T ui::AsButton() {
    return ui::Current()->AsButton();
}

void ui::Pop() {
    GetUI()->text_box_stack.pop();
}

Vector2 ui::GetRelMousePos() {
    return {
        GetMousePosition().x - ui::Current()->text_start_x,
        GetMousePosition().y - ui::Current()->text_start_y
    };
}

void ui::Shrink(int dx, int dy) {
    ui::Current()->Shrink(dx, dy);
}

void ui::Enclose() {
    ui::Current()->Enclose(4, 4, ui::Current()->background_color, ui::Current()->text_color);
}

void ui::EncloseEx(int shrink, Color background_color, Color line_color, int corner_radius) {
    ui::Current()->Enclose(shrink, corner_radius, background_color, line_color);
}

void ui::EnclosePartial(int inset, Color background_color, Color line_color, Direction::T directions) {
    ui::Current()->EnclosePartial(inset, background_color, line_color, directions);
}

void ui::DrawIcon(AtlasPos atlas_index, Color tint, int height) {
    ui::Current()->DrawTexture(GetUI()->GetIconAtlas(), atlas_index.GetRect(ATLAS_SIZE), height, tint, false);
}

void ui::DrawIconSDF(AtlasPos atlas_index, Color tint, int height) {
    ui::Current()->DrawTexture(GetUI()->GetIconAtlasSDF(), atlas_index.GetRect(ATLAS_SIZE), height, tint, true);
}

void ui::Write(const char *text) {
    ui::WriteEx(text, TextAlignment::CONFORM, true);
}

void ui::WriteEx(const char *text, TextAlignment::T alignemnt, bool linebreak) {
    TextBox* tb = ui::Current();
    if (linebreak) {
        tb->WriteLine(text, alignemnt);
    } else {
        tb->Write(text, alignemnt);
    }
}

void ui::DecorateEx(const char *text, TextAlignment::T alignemnt, const TokenList* tokens) {
    ui::Current()->Decorate(text, alignemnt, tokens);
}

Rectangle ui::MeasureTextEx(const char *text, TextAlignment::T alignemnt) {
    return ui::Current()->TbMeasureText(text, alignemnt);
}

void ui::Fillline(double value, Color fill_color, Color background_color) {
    TextBox* tb = ui::Current();
    int y_end = tb->text_start_y + tb->y_cursor;
    if (y_end > tb->render_rec.y + tb->render_rec.height || y_end < tb->render_rec.y)
        return;
    
    int x_start = ClampInt(tb->text_start_x, tb->render_rec.x, tb->render_rec.x + tb->render_rec.width);
    int x_end = ClampInt(tb->text_start_x + tb->width, tb->render_rec.x, tb->render_rec.x + tb->render_rec.width);
    int x_mid_point = ClampInt(tb->text_start_x + tb->width * value, tb->render_rec.x, tb->render_rec.x + tb->render_rec.width);
    BeginRenderInUIMode(tb->z_layer);
    DrawLine(x_start, y_end, x_end, y_end, background_color);
    DrawLine(x_start, y_end, x_mid_point, y_end, fill_color);
    //DrawLine(x_mid_point, y_end, x_mid_point, y_end - 4, fill_color);
    EndRenderInUIMode();
}

ButtonStateFlags::T ui::DirectButton(const char* text, int margin) {
    TextBox* tb = ui::Current();
    /*ButtonStateFlags::T button_state = tb->WriteButton(text, inset);
    HandleButtonSound(button_state);*/
    Rectangle text_rect = tb->TbMeasureText(text, TextAlignment::CONFORM);
    ui::PushInline(text_rect.width + 2*margin, text_rect.height + 2*margin);
    ButtonStateFlags::T button_state = ui::AsButton();
    if (button_state & ButtonStateFlags::HOVER) {
        ui::EncloseEx(2, tb->background_color, Palette::interactable_main, 0);
    } else {
        ui::EncloseEx(2, tb->background_color, tb->text_color, 0);
    }
    ui::WriteEx(text, TextAlignment::CENTER, false);
    HandleButtonSound(button_state);
    ui::Pop();
    return button_state;
}

ButtonStateFlags::T ui::ToggleButton(bool on) {
    ui::WriteEx(" ", TextAlignment::CONFORM, false);
    ButtonStateFlags::T res = ui::DirectButton(on ? " X " : "   ", -2);
    ui::WriteEx(" ", TextAlignment::CONFORM, false);
    return res;
}

void ui::HelperText(const char* description) {
    TextBox* tb = ui::Current();
    StringBuilder sb = StringBuilder(description);
    sb.AutoBreak(100);
    Vector2 button_size = { 24, 24 };
    TextAlignment::T button_align = TextAlignment::TOP | TextAlignment::RIGHT;
    Vector2 button_pos = ApplyAlignment(ui::Current()->GetAnchorPoint(button_align), button_size, button_align);
    button_pos.x -= 2;
    button_pos.y += 2;
    Rectangle rect = { button_pos.x, button_pos.y, button_size.x, button_size.y };
    //BeginRenderInUIMode(tb->z_layer);
    //DrawRectangleLinesEx(tb->render_rec, 1, GREEN);
    //EndRenderInUIMode();
    InternalDrawTextEx(GetCustomDefaultFont(), "??", { button_pos.x, button_pos.y }, tb->text_size, 1, 
        Palette::interactable_main, tb->render_rec, tb->z_layer
    );

    if (CheckCollisionCircleRec(GetMousePosition(), 2, rect) && !GetUI()->IsPointBlocked(GetMousePosition(), tb->z_layer)) {
        //GetUI()->mousehints.count = 0;
        GetUI()->mousehints.AddHint(rect, { button_pos.x, button_pos.y + button_size.y}, description);
    }
}

TextBox* ui::Current() {
    if (GetUI()->text_box_stack.size() > 0)
        return &GetUI()->text_box_stack.top();
    return NULL;
}

void ui::HSpace(int pixels) {
    ui::Current()->x_cursor += pixels;
}

void ui::VSpace(int pixels) {
    ui::Current()->y_cursor += pixels;
}

void ui::SetMouseHint(const char* text) {
    strncpy(GetUI()->mouseover_text, text, 1024);
}

void UIGlobals::UIInit() {
    if (IsTextureReady(default_font.texture) && IsTextureReady(default_font_sdf.texture)) return;
    default_font = LoadFont("resources/fonts/space_mono_small.fnt");
    default_font_sdf = LoadFont("resources/fonts/space_mono_small_sdf.fnt");
    SetTextureFilter(default_font_sdf.texture, TEXTURE_FILTER_BILINEAR);  // Very important step
    SetTextLineSpacing(20);
}

void UIGlobals::UIStart() {  // Called each frame before drawing UI
    SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
    for( int i=0; i < acc_blocking_rect_index; i++) {
        blocking_rects[i].rec = acc_blocking_rects[i].rec;
        blocking_rects[i].z = acc_blocking_rects[i].z;
    }
    blocking_rect_index = acc_blocking_rect_index;
    while(acc_blocking_rect_index > 0) {
        acc_blocking_rects[--acc_blocking_rect_index] = {0};
    }
}

void UIGlobals::UIEnd() {  // Called each frame after drawing UI
    _HandleMouseTips();
}

void UIGlobals::_HandleMouseTips() {
    if (mouseover_text[0] != '\0') {
        // Draw mouse
        Vector2 mouse_pos = GetMousePosition();
        Vector2 text_size = MeasureTextEx(GetCustomDefaultFont(), mouseover_text, DEFAULT_FONT_SIZE, 1);
        ui::PushMouseHint(mouse_pos, text_size.x+8, text_size.y+8, 255 - MAX_TOOLTIP_RECURSIONS);
        ui::Enclose();
        ui::Write(mouseover_text);
        ui::Pop();
        mouseover_text[0] = '\0';
    }

    for (int i=mousehints.count-1; i >= 0; i--) {  // Avoid overdraw
        bool top_most = i == mousehints.count-1;
        StringBuilder sb = StringBuilder(mousehints.hints[i]);
        sb.AutoBreak(100);
        TokenList tokens = sb.ExtractTokens("[[", "]]");
        Rectangle org_rect = mousehints.origin_button_rects[i];
        //Vector2 anchor = { org_rect.x, org_rect.y + org_rect.height };  // bottom-left
        Vector2 text_size = MeasureTextEx(GetCustomDefaultFont(), sb.c_str, DEFAULT_FONT_SIZE, 1);
        mousehints.hint_rects[i].width = text_size.x;
        mousehints.hint_rects[i].height = text_size.y;
        Vector2 anchor = { mousehints.hint_rects[i].x, mousehints.hint_rects[i].y};
        ui::PushMouseHint(anchor, text_size.x+8, text_size.y+8, 255 - MAX_TOOLTIP_RECURSIONS + i);
        mousehints.hint_rects[i] = ui::Current()->render_rec;
        
        int hover_char_index = -1;
        int hover_token_index = -1;
        // Check for recursive tooltips
        if (!IsPointBlocked(GetMousePosition(), ui::Current()->z_layer)) {
            int char_index = ui::Current()->TbGetCharacterIndex(GetMousePosition(), sb.c_str, TextAlignment::CONFORM);
            for (int j=0; j < tokens.length; j++) {
                if (char_index > tokens.start_positions[j] && char_index < tokens.end_positions[j]) {
                    hover_token_index = j;
                    hover_char_index = char_index;
                    break;
                }
            }
        }
        // Add recursive tooltips
        if (hover_token_index >= 0) {
            Rectangle rect = ui::Current()->TbGetTextRect(
                sb.c_str, TextAlignment::CONFORM, tokens.start_positions[hover_token_index], 
                tokens.end_positions[hover_token_index], hover_char_index
            );
            //DrawRectangleLinesEx(rect, 1, GREEN);

            int substr_len =  tokens.end_positions[hover_token_index] - tokens.start_positions[hover_token_index];
            char* substr = new char[substr_len + 1];
            strncpy(substr, &sb.c_str[tokens.start_positions[hover_token_index]], substr_len);
            substr[substr_len] = '\0';
            const char* new_hint = GetConceptDescription(substr);
            delete[] substr;

            Vector2 new_anchor = {
                rect.x + rect.width / 2.0f,
                mousehints.hint_rects[i].y + mousehints.hint_rects[i].height + 10
            };
            if (!top_most && (MouseHints::Hash(mousehints.origin_button_rects[i+1]) != MouseHints::Hash(rect))) {
                mousehints.count = i + 1;
            }
            mousehints.AddHint(rect, new_anchor, new_hint);
        }

        if (i < mousehints.count-1) {
            //ui::Current()->text_color = Palette::ui_alt;
        }

        // Draw 
        ui::Enclose();
        ui::DecorateEx(sb.c_str, TextAlignment::CONFORM, &tokens);
        ui::Write(sb.c_str);
        if (i == mousehints.count-1) {
            ui::Fillline(mousehints.lock_progress, Palette::ui_main, Palette::bg);
        }
        ui::Pop();
    }

    // Draw connections
    BeginRenderInUIMode(255 - MAX_TOOLTIP_RECURSIONS + mousehints.count);
    for (int i = mousehints.count - 1; i >= 0; i--) {
        int midpoint = mousehints.origin_button_rects[i].x + mousehints.origin_button_rects[i].width / 2.0f;
        DrawLine(
            midpoint, mousehints.origin_button_rects[i].y + mousehints.origin_button_rects[i].height,
            midpoint, mousehints.hint_rects[i].y,
            Palette::ui_main
        );
        /*DrawLine(
            mousehints.hint_rects[i].x,
            mousehints.hint_rects[i].y,
            mousehints.origin_button_rects[i].x,
            mousehints.origin_button_rects[i].y + mousehints.origin_button_rects[i].height,
            Palette::ui_alt
        );
        DrawLine(
            mousehints.hint_rects[i].x + mousehints.hint_rects[i].width,
            mousehints.hint_rects[i].y,
            mousehints.origin_button_rects[i].x + mousehints.origin_button_rects[i].width,
            mousehints.origin_button_rects[i].y + mousehints.origin_button_rects[i].height,
            Palette::ui_alt
        );*/
    }
    EndRenderInUIMode();
    // Handle deselect
    int top_hint_index = mousehints.count - 1;
    for (int i=top_hint_index; i >= 0; i--) {
        bool hover = CheckCollisionCircleRec(GetMousePosition(), 5, mousehints.origin_button_rects[i]);
        //DrawRectangleLinesEx(mousehints.origin_button_rects[i], 1, GREEN);
        if (mousehints.lock_progress > 0.99 || i != top_hint_index) {
            Rectangle encapsulating = EncapsulationRectangle(
                mousehints.origin_button_rects[i], 
                mousehints.hint_rects[i]
            );
            //DrawRectangleLinesEx(mousehints.hint_rects[i], 1, GREEN);
            //DrawRectangleLinesEx(encapsulating, 1, RED);
            hover = CheckCollisionPointRec(GetMousePosition(), encapsulating);
        }
        if (hover) {
            mousehints.lock_progress = Clamp(mousehints.lock_progress + GetFrameTime() * 1.0f, 0.0f, 1.0f);
            break;
        } else {
            mousehints.lock_progress = 1.0f;
            mousehints.count = i;
        }
    }
}

void UIGlobals::AddBlockingRect(Rectangle rect, uint8_t z_layer) {
    if (acc_blocking_rect_index == MAX_BLOCKING_RECTS-1) return;
    acc_blocking_rects[acc_blocking_rect_index].rec = rect;
    acc_blocking_rects[acc_blocking_rect_index].z = z_layer;
    acc_blocking_rect_index++;
}

bool UIGlobals::IsPointBlocked(Vector2 pos, uint8_t z_layer) const {
    for(int i=0; i < blocking_rect_index; i++) {
        if (CheckCollisionPointRec(pos, blocking_rects[i].rec) && blocking_rects[i].z > z_layer) {
            return true;
        }
    }
    return false;
}

const char* UIGlobals::GetConceptDescription(const char* key) {
    if (concept_descriptions.GetFieldCount() == 0) {
        DataNode::FromFile(&concept_descriptions, "resources/data/concepts.yaml");
    }
    return concept_descriptions.Get(key, "No descritpion found", true);
}

Texture2D UIGlobals::GetIconAtlas() {
    if (!IsTextureReady(default_font.texture)) {
        UIInit();
    }
    return default_font.texture;
}

Texture2D UIGlobals::GetIconAtlasSDF() {
    if (!IsTextureReady(default_font_sdf.texture)) {
        UIInit();
    }
    return default_font_sdf.texture;
}

void UIGlobals::MouseHints::AddHint(Rectangle origin_button, Vector2 anchor, const char *hint) {
    // Adds iff it dousn't find the rectangle in list
    int hash = Hash(origin_button);

    for (int i=0; i < count; i++) {
        if (Hash(origin_button_rects[i]) == hash) {
            return;
        }
    }

    // Not found in list
    if (count > MAX_TOOLTIP_RECURSIONS) return;
    //INFO("new tt: %f, %f", origin_button.x, origin_button.y)

    lock_progress = 0.0f;

    origin_button_rects[count] = origin_button;
    hint_rects[count].x = anchor.x;
    hint_rects[count].y = anchor.y;
    delete[] hints[count];
    hints[count] = new char[strlen(hint) + 1];
    strcpy(hints[count], hint);
    count++;
}

int UIGlobals::MouseHints::Hash(Rectangle origin_button) {
    return (int)origin_button.x * 10000 + (int)origin_button.y;
}

Font GetCustomDefaultFont() {
    if (!IsTextureReady(GetUI()->default_font.texture) || !IsTextureReady(GetUI()->default_font_sdf.texture)) {
        GetUI()->UIInit();
    }
    if (GetSettingBool("sdf_text", false))
        return GetUI()->default_font_sdf;
    else
        return GetUI()->default_font;
}

ButtonStateFlags::T DrawTriangleButton(Vector2 point, Vector2 base, double width, Color color) {
    Vector2 base_pos = Vector2Add(point, base);
    Vector2 tangent_dir = Vector2Rotate(Vector2Normalize(base), PI/2);
    Vector2 side_1 =  Vector2Add(base_pos, Vector2Scale(tangent_dir, -width));
    Vector2 side_2 =  Vector2Add(base_pos, Vector2Scale(tangent_dir, width));
    bool is_in_area = CheckCollisionPointTriangle(GetMousePosition(), side_1, point, side_2);
    if (is_in_area) {
        DrawTriangle(side_1, point, side_2, Palette::interactable_main);
    } else {
        DrawTriangleLines(side_1, point, side_2, Palette::interactable_main);
    }
    return GetButtonState(
        is_in_area,
        CheckCollisionPointTriangle(Vector2Subtract(GetMousePosition(), GetMouseDelta()), side_1, point, side_2)
    );
}

ButtonStateFlags::T DrawCircleButton(Vector2 midpoint, double radius, Color color) {
    bool is_in_area = CheckCollisionPointCircle(GetMousePosition(), midpoint, radius);
    if (is_in_area) {
        DrawCircleV(midpoint, radius, Palette::interactable_main);
    } else {
        DrawCircleLines(midpoint.x, midpoint.y, radius, Palette::interactable_main);
    }
    return GetButtonState(
        is_in_area,
        CheckCollisionPointCircle(Vector2Subtract(GetMousePosition(), GetMouseDelta()), midpoint, radius)
    );
}