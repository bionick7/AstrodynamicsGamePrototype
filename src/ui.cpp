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

Rectangle DrawTextAligned(const char* text, Vector2 pos, TextAlignment::T alignment, Color c) {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, DEFAULT_FONT_SIZE, 1);
    pos = ApplyAlignment(pos, size, alignment);
    //Vector2 bottom_left = Vector2Subtract(pos, Vector2Scale(size, 0.5));
    Rectangle rect = { pos.x, pos.y, size.x, size.y };
    InternalDrawTextEx(GetCustomDefaultFont(), text, pos, DEFAULT_FONT_SIZE, 1, c);
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
    DrawRectangleRounded(rect, roundness, 16, background_color);
    DrawRectangleRoundedLines(rect, roundness, 16, 1, line_color);
    Shrink(inset, inset);
}

void TextBox::EnclosePartial(int inset, Color background_color, Color line_color, Direction::T directions) {
    Rectangle rect;
    rect.x = text_start_x;
    rect.y = text_start_y;
    rect.width = width;
    rect.height = height;

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

    rect = GetCollisionRec(rect, render_rec);
    Shrink(inset, inset);
}

void TextBox::Shrink(int dx, int dy) {
    text_start_x += dx;
    text_start_y += dy;
    width -= 2*dx;
    height -= 2*dy;
}

void TextBox::Write(const char* text, TextAlignment::T align) {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    Vector2 pos = ApplyAlignment(GetAnchorPoint(align), size, align);
    // avoid drawing text_background is fully transparent (useless)
    if(text_background.a != 0) {
        DrawRectangleV(pos, size, text_background);
    }
    InternalDrawTextEx(GetCustomDefaultFont(), text, pos, text_size, 1, text_color, render_rec);
    if (GetSettingBool("text_boundrects", false)) {
        DrawRectangleLines(pos.x, pos.y, size.x, size.y, GREEN);
    }
    _Advance(pos, size);
}

void TextBox::WriteLine(const char* text, TextAlignment::T align) {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    Vector2 pos = ApplyAlignment(GetAnchorPoint(align), size, align);
    //DebugPrintText("(%f, %f) ; (%f, %f)", pos.x, pos.y, size.x, size.y);

    // avoid drawing if text_background is fully transparent (useless)
    if (text_background.a != 0) {
        DrawRectangleV(pos, size, text_background);
    }
    InternalDrawTextEx(GetCustomDefaultFont(), text, pos, text_size, 1, text_color, render_rec);
    if (GetSettingBool("text_boundrects", false)) {
        DrawRectangleLines(pos.x, pos.y, size.x, size.y, GREEN);
    }
    _Advance(pos, size);
    LineBreak();
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
        DrawTextureSDF(texture, source, destination, Vector2Zero(), 0, tint);
    } else {
        DrawTexturePro(texture, source, destination, Vector2Zero(), 0, tint);
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
    InternalDrawTextEx(GetCustomDefaultFont(), text, pos, text_size, 1, text_color, render_rec);
    _Advance(pos, size);
    return res;
}

ButtonStateFlags::T TextBox::AsButton() const {
    return GetButtonState(
        CheckCollisionPointRec(GetMousePosition(), {(float)text_start_x, (float)text_start_y, (float)width, (float)height}),
        CheckCollisionPointRec(Vector2Subtract(GetMousePosition(), GetMouseDelta()), {(float)text_start_x, (float)text_start_y, (float)width, (float)height})
    );
}

void TextBox::DebugDrawRenderRec() const {
    DrawRectangleRec(render_rec, BLACK);
    DrawRectangleLinesEx(render_rec, 4, PURPLE);
}


int TextBox::GetLineHeight() const {
    return text_size + text_margin_y;
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

Rectangle TextBox::TbMeasureTextEx(const char* text, TextAlignment::T alignemnt) const {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(), text, text_size, 1);
    Vector2 pos = ApplyAlignment(GetAnchorPoint(alignemnt), size, alignemnt);
    return {pos.x, pos.y, size.x, size.y};
}

void ui::PushGlobal(int x, int y, int w, int h, int text_size, Color color) {
    TextBox new_text_box = TextBox(x, y, w, h, text_size, color);
    GetUI()->text_box_stack.push(new_text_box);
}

void ui::CreateNew(int x, int y, int w, int h, int text_size, Color color) {
    while (GetUI()->text_box_stack.size() > 0) {  // Clear stack
        GetUI()->text_box_stack.pop();
    }
    GetUI()->AddBlockingRect({(float)x, (float)y, (float)w, (float)h});
    ui::PushGlobal(x, y, w, h, text_size, color);
}

void ui::PushMouseHint(int width, int height) {
    Vector2 m_pos = GetMousePosition();
    int x_pos = m_pos.x;
    int y_pos = m_pos.y;
    if (x_pos > GetScreenWidth() - width) x_pos = GetScreenWidth() - width;
    if (y_pos > GetScreenHeight() - height) y_pos = GetScreenHeight() - height;

    ui::PushGlobal(x_pos, y_pos, width, height, DEFAULT_FONT_SIZE, Palette::ui_main);
}

int ui::PushInset(int margin, int h) {
    // Returns the actual height
    h += 8;  // straight up allocating extra space for magins
    TextBox* tb = ui::Current();
    tb->EnsureLineBreak();
    h = fmin(h, fmax(0, tb->height - tb->y_cursor - 2*margin));
    TextBox new_text_box = TextBox(
        tb->text_start_x + tb->x_cursor + margin,
        tb->text_start_y + tb->y_cursor + margin,
        tb->width - tb->x_cursor - 2*margin,
        h,
        tb->text_size,
        tb->text_color
    );
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb->render_rec);

    tb->y_cursor += h + 2*margin;
    GetUI()->text_box_stack.push(new_text_box);
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
        DrawRectangleRounded({
            (float) tb->text_start_x + tb->width - buildin_scrollbar_margin,
            (float) tb->text_start_y + scroll_progress * (h - scrollbar_height),
            (float) buildin_scrollbar_width,
            (float) scrollbar_height},
            buildin_scrollbar_width/2,
            1, tb->text_color
        );
    } else {
        buildin_scrollbar_margin = 0;
        buildin_scrollbar_width = 0;
    }
    TextBox new_text_box = TextBox(
        tb->text_start_x + tb->x_cursor + margin,
        tb->text_start_y + tb->y_cursor + margin - *scroll,
        tb->width - tb->x_cursor - 2*margin - 2*buildin_scrollbar_margin - buildin_scrollbar_width,
        allocated_height,
        tb->text_size,
        tb->text_color
    );
    new_text_box.render_rec.y = tb->text_start_y + tb->y_cursor + margin;
    new_text_box.render_rec.height = height;

    tb->y_cursor += h + 2*margin;
    GetUI()->text_box_stack.push(new_text_box);
    return height;
}

void ui::PushInline(int width, int height) {
    TextBox* tb = ui::Current();
    if (width > tb->width - tb->x_cursor)
        width = tb->width - tb->x_cursor;
    TextBox new_tb = TextBox(
        tb->text_start_x + tb->x_cursor,
        tb->text_start_y + tb->y_cursor,
        width,
        height + 8,
        tb->text_size,
        tb->text_color
    );
    new_tb.render_rec = GetCollisionRec(new_tb.render_rec, tb->render_rec);

    tb->_Advance(
        {(float)new_tb.text_start_x, (float)new_tb.text_start_y}, 
        {(float)new_tb.width, (float)new_tb.height}
    );
    GetUI()->text_box_stack.push(new_tb);
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

    TextBox new_text_box = TextBox(
        x, y, width, height,
        tb->text_size,
        tb->text_color
    );

    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb->render_rec);
    GetUI()->text_box_stack.push(new_text_box);
}

void ui::PushHSplit(int x_start, int x_end) {
    TextBox* tb = ui::Current();
    if (x_start < 0) x_start += tb->width;
    if (x_end < 0) x_end += tb->width;
    TextBox new_text_box = TextBox(
        tb->text_start_x + x_start,
        tb->text_start_y,
        x_end - x_start,
        tb->height,
        tb->text_size,
        tb->text_color
    );
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb->render_rec);

    GetUI()->text_box_stack.push(new_text_box);
}

void ui::PushGridCell(int columns, int rows, int column, int row) {
    TextBox* tb = ui::Current();
    TextBox new_text_box = TextBox(
        tb->text_start_x + column * tb->width / columns,
        tb->text_start_y + row * tb->height / rows,
        tb->width / columns,
        tb->height / rows,
        tb->text_size,
        tb->text_color
    );
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb->render_rec);

    GetUI()->text_box_stack.push(new_text_box);
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

void ui::Enclose(Color background_color, Color line_color) {
    ui::Current()->Enclose(4, 4, background_color, line_color);
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

Rectangle ui::MeasureTextEx(const char *text, TextAlignment::T alignemnt) {
    return ui::Current()->TbMeasureTextEx(text, alignemnt);
}

void ui::Fillline(double value, Color fill_color, Color background_color) {
    TextBox* tb = ui::Current();
    int y_end = tb->text_start_y + tb->y_cursor;
    if (y_end > tb->render_rec.y + tb->render_rec.height || y_end < tb->render_rec.y)
        return;
    
    int x_start = ClampInt(tb->text_start_x, tb->render_rec.x, tb->render_rec.x + tb->render_rec.width);
    int x_end = ClampInt(tb->text_start_x + tb->width, tb->render_rec.x, tb->render_rec.x + tb->render_rec.width);
    int x_mid_point = ClampInt(tb->text_start_x + tb->width * value, tb->render_rec.x, tb->render_rec.x + tb->render_rec.width);
    DrawLine(x_start, y_end, x_end, y_end, background_color);
    DrawLine(x_start, y_end, x_mid_point, y_end, fill_color);
    DrawLine(x_mid_point, y_end, x_mid_point, y_end - 4, fill_color);
}

ButtonStateFlags::T ui::DirectButton(const char* text, int margin) {
    TextBox* tb = ui::Current();
    /*ButtonStateFlags::T button_state = tb->WriteButton(text, inset);
    HandleButtonSound(button_state);*/
    Rectangle text_rect = tb->TbMeasureTextEx(text, TextAlignment::CONFORM);
    ui::PushInline(text_rect.width + 2*margin, text_rect.height + 2*margin);
    ButtonStateFlags::T button_state = ui::AsButton();
    if (button_state & ButtonStateFlags::HOVER) {
        ui::Enclose(tb->text_background, Palette::interactable_main);
    } else {
        ui::Enclose(tb->text_background, tb->text_color);
    }
    ui::WriteEx(text, TextAlignment::CENTER, false);
    HandleButtonSound(button_state);
    ui::Pop();
    return button_state;
}

void ui::HelperText(const char* description) {
    StringBuilder sb = StringBuilder(description);
    sb.AutoBreak(100);
    Vector2 buton_size = { 24, 24 };
    TextAlignment::T button_align = TextAlignment::TOP | TextAlignment::RIGHT;
    Vector2 buton_pos = ApplyAlignment(ui::Current()->GetAnchorPoint(button_align), buton_size, button_align);
    buton_pos.x -= 2;
    buton_pos.y -= 2;
    Rectangle rect = { buton_pos.x, buton_pos.y, buton_size.x, buton_size.y };
    InternalDrawText("??", { buton_pos.x, buton_pos.y }, Palette::interactable_main);
    if (CheckCollisionPointRec(GetMousePosition(), rect)) {
        ui::SetMouseHint(sb.c_str);
    }
}

TextBox* ui::Current() {
    return &GetUI()->text_box_stack.top();
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
    for(; blocking_rect_index >= 0; blocking_rect_index--) {
        blocking_rects[blocking_rect_index] = {0};
    }
    blocking_rect_index = 0;  // Undershoots in loop
}

void UIGlobals::UIEnd() {  // Called each frame after drawing UI
    if (mouseover_text[0] != '\0') {
        // Draw mouse
        Vector2 mouse_pos = GetMousePosition();
        Vector2 text_size = MeasureTextEx(GetCustomDefaultFont(), mouseover_text, DEFAULT_FONT_SIZE, 1);
        ui::PushMouseHint(text_size.x+8, text_size.y+8);
        ui::Enclose(Palette::bg, Palette::ui_main);
        ui::Write(mouseover_text);
        GetUI()->mouseover_text[0] = '\0';
    }
}

void UIGlobals::AddBlockingRect(Rectangle rect) {
    if (blocking_rect_index == MAX_BLOCKING_RECTS-1) return;
    blocking_rects[blocking_rect_index++] = rect;
}

bool UIGlobals::IsPointBlocked(Vector2 pos) const {
    for(int i=0; i < blocking_rect_index; i++) {
        if (CheckCollisionPointRec(pos, blocking_rects[i])) {
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