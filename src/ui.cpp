#include "ui.hpp"
#include "logging.hpp"
#include "constants.hpp"
#include "audio_server.hpp"
#include "global_state.hpp"
#include "utils.hpp"
#include "debug_console.hpp"
#include "debug_drawing.hpp"
#include "render_utils.hpp"
#include "text_rendering.hpp"

Vector2 ApplyAlignment(Vector2 anchorpoint, Vector2 size, text_alignment::T alignment) {
    switch(alignment & text_alignment::HFILTER){
        case text_alignment::HCENTER: anchorpoint.x -= size.x/2; break;
        case text_alignment::RIGHT: anchorpoint.x -= size.x; break;
    }
    switch(alignment & text_alignment::VFILTER){
        case text_alignment::VCENTER: anchorpoint.y -= size.y/2; break;
        case text_alignment::BOTTOM: anchorpoint.y -= size.y; break;
    }
    return anchorpoint;
}

Vector2 ApplyAlignmentInRect(Rectangle enclosing, Vector2 size, text_alignment::T alignment) {
    Vector2 anchor = { enclosing.x, enclosing.y };
    int w2 = enclosing.width;
    int h2 = enclosing.height;
    switch(alignment & text_alignment::HFILTER){
        case text_alignment::HCENTER: anchor.x += w2/2; break;
        case text_alignment::RIGHT: anchor.x += w2; break;
    }
    switch(alignment & text_alignment::VFILTER){
        case text_alignment::VCENTER: anchor.y += h2/2; break;
        case text_alignment::BOTTOM: anchor.y += h2; break;
    }
    return ApplyAlignment(anchor, size, alignment);
}

Rectangle DrawTextAligned(const char* text, Vector2 pos, text_alignment::T alignment, Color c, Color bg, uint8_t z_layer) {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(DEFAULT_FONT_SIZE), text, DEFAULT_FONT_SIZE, 1);
    pos = ApplyAlignment(pos, size, alignment);
    //Vector2 bottom_left = Vector2Subtract(pos, Vector2Scale(size, 0.5));
    Rectangle rect = { pos.x, pos.y, size.x, size.y };
    text::DrawTextEx(GetCustomDefaultFont(DEFAULT_FONT_SIZE), text, pos, DEFAULT_FONT_SIZE, 1, c, bg, GetScreenRect(), z_layer);
    return rect;
}

button_state_flags::T GetButtonState(bool is_in_area, bool was_in_area) {
    button_state_flags::T res = button_state_flags::NONE;
    if (is_in_area) {
        res |= button_state_flags::HOVER;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))     res |= button_state_flags::PRESSED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  res |= button_state_flags::JUST_PRESSED;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) res |= button_state_flags::JUST_UNPRESSED;

        if (!was_in_area) res |= button_state_flags::JUST_HOVER_IN;
    }
    if (was_in_area && !is_in_area) res |= button_state_flags::JUST_HOVER_OUT;

    return res;
}

button_state_flags::T GetButtonStateRec(Rectangle rec) {
    return GetButtonState(
        CheckCollisionPointRec(GetMousePosition(), rec), 
        CheckCollisionPointRec(Vector2Subtract(GetMousePosition(), GetMouseDelta()), rec)
    );
}

void HandleButtonSound(button_state_flags::T state) {
    if (state & button_state_flags::JUST_PRESSED) {
        PlaySFX(SFX_CLICK_BUTTON);
    }
    if (state & button_state_flags::JUST_HOVER_IN) {
        PlaySFX(SFX_CLICK_SHORT);
    }
}

TextBox::TextBox(int p_x, int p_y, int w, int h, int ptext_size, Color color, Color pbackground_color, uint8_t pz_layer) {
    ASSERT(w > 0)
    ASSERT(h >= 0)
    x = p_x;
    y = p_y;
    width = w;
    height = h;
    text_margin_x = 6;
    text_margin_y = 0;

    render_rec.x = p_x;
    render_rec.y = p_y;
    render_rec.width = w;
    render_rec.height = h;
    flexible = false;
    z_layer = pz_layer;

    text_size = ptext_size;
    text_color = color;
    text_background = pbackground_color;
    background_color = pbackground_color;

    x_cursor = 0;
    y_cursor = 0;
    line_size_y = 0;
    extend_x = 0;
    extend_y = 0;
}

TextBox::TextBox(const TextBox *parent, int x, int y, int w, int h) : 
    TextBox::TextBox(x, y, w, h, parent->text_size, parent->text_color, 
                     parent->background_color, parent->z_layer+2) {
        flexible = parent->flexible;
    }

int TextBox::GetCharWidth() {
    Font font = GetCustomDefaultFont(text_size);
    int _;
    int W_codepoint = GetCodepointNext("W", &_);
    int width = font.recs[W_codepoint].width + font.glyphs[W_codepoint].offsetX;
    float scaling = (text_size / font.baseSize);
    return (int) (width * scaling);
}

void TextBox::_Advance(Vector2 pos, Vector2 size) {
    if (size.y > line_size_y) line_size_y = size.y;
    if (pos.x + size.x > x + x_cursor) x_cursor = pos.x + size.x - (x + text_margin_x);

    RecalculateExtends();
}

void TextBox::RecalculateExtends() {
    if (x_cursor > extend_x) {
        extend_x = x_cursor;
    }
    if (y_cursor + line_size_y > extend_y) {
        extend_y = y_cursor + line_size_y;
    }
}

void TextBox::LineBreak() {
    x_cursor = 0;
    y_cursor += line_size_y;
    line_size_y = 0;
    RecalculateExtends();
}

void TextBox::EnsureLineBreak() {
    if (x_cursor > 0) {
        y_cursor += line_size_y;
        x_cursor = 0;
    }
    line_size_y = 0;
    RecalculateExtends();
}

void TextBox::Enclose(int inset, int corner_radius, Color background_color, Color line_color) {
    Rectangle rect = GetRect();

    if (!flexible)
        rect = GetCollisionRec(rect, render_rec);
    rect.x += inset;
    rect.y += inset;
    rect.width -= 2*inset;
    rect.height -= 2*inset;

    float roundness = corner_radius * 2.0f / fmin(rect.width, rect.height);
    BeginRenderInUILayer(z_layer);
    DrawRectangleRounded(rect, roundness, 16, background_color);
    DrawRectangleRoundedLines(rect, roundness, 16, 1, line_color);
    EndRenderInUILayer();
}

void TextBox::EnclosePartial(int inset, Color background_color, Color line_color, direction::T directions) {
    Rectangle rect;
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;

    BeginRenderInUILayer(z_layer);
    DrawRectangleRounded(rect, 0, 1, background_color);
    if (directions & direction::TOP) {
        DrawLine(rect.x, rect.y, rect.x + width, rect.y, line_color);
    }
    if (directions & direction::DOWN) {
        DrawLine(rect.x, rect.y + rect.height, rect.x + width, rect.y + rect.height, line_color);
    }
    if (directions & direction::LEFT) {
        DrawLine(rect.x, rect.y, rect.x, rect.y + rect.height, line_color);
    }
    if (directions & direction::RIGHT) {
        DrawLine(rect.x + width, rect.y, rect.x + width, rect.y + rect.height, line_color);
    }
    EndRenderInUILayer();

    if (!flexible)
        rect = GetCollisionRec(rect, render_rec);
    Shrink(inset, inset);
}

void TextBox::EncloseDynamic(int inset, int corner_radius, Color background_color, Color line_color) {
    width = extend_x + 2 * text_margin_x;
    height = extend_y + 2 * text_margin_y;
    Rectangle rect = GetRect();

    if (!flexible)
        rect = GetCollisionRec(rect, render_rec);
    rect.x -= inset;
    rect.y -= inset;
    rect.width += 2*inset;
    rect.height += 2*inset;

    float roundness = corner_radius * 2.0f / fmin(rect.width, rect.height);
    BeginRenderInUILayer(z_layer - 1);
    DrawRectangleRounded(rect, roundness, 16, background_color);
    DrawRectangleRoundedLines(rect, roundness, 16, 1, line_color);
    EndRenderInUILayer();
}

void TextBox::Shrink(int dx, int dy) {
    x += dx;
    y += dy;
    width -= 2*dx;
    height -= 2*dy;
}

void TextBox::WriteRaw(const char *text, text_alignment::T align) {
    text::Layout layout = GetTextLayout(text, align);
    WriteLayout(&layout, false);
}

void TextBox::Write(const char* text, text_alignment::T align) {
    text::Layout layout = GetTextLayout(text, align);
    WriteLayout(&layout, true);
}

void TextBox::WriteLine(const char* text, text_alignment::T align) {
    Write(text, align);
    LineBreak();
}

void TextBox::WriteLayout(const text::Layout* layout, bool advance_cursor) {
    Rectangle rect = layout->bounding_box;
    if (flexible) {
        layout->DrawTextLayout(GetCustomDefaultFont(text_size), text_size, text_color, text_background, GetScreenRect(), z_layer);
    } else {
        layout->DrawTextLayout(GetCustomDefaultFont(text_size), text_size, text_color, text_background, render_rec, z_layer);
    }
    if (advance_cursor) {
        AdvanceLayout(layout);
    }
    if (GetSettingBool("draw_textrects", false)) {
        BeginRenderInUILayer(z_layer);
        DrawRectangleLinesEx(rect, 1, RED);
        DrawCircle(x + text_margin_x + x_cursor, y + text_margin_y + y_cursor + line_size_y, 2, RED);
        for(int i=0; i < layout->size; i++) {
            DrawRectangleLinesEx(layout->rects[i], 1, ColorBrightness(RED, -0.5));
        }
        EndRenderInUILayer();
    }
}

void TextBox::AdvanceLayout(const text::Layout* layout) {
    Rectangle rect = layout->bounding_box;
    line_size_y = 20;
    x_cursor = rect.x + rect.width - (x + text_margin_x);
    y_cursor = rect.y + rect.height - (y + text_margin_y) - line_size_y;
    if (x_cursor > extend_x) extend_x = x_cursor;
    if (y_cursor + line_size_y > extend_y) extend_y = y_cursor + line_size_y;
}

void TextBox::Decorate(const text::Layout* layout, const TokenList* tokens) {
    List<Rectangle> recs;
    layout->GetTextRects(&recs, tokens);
    BeginRenderInUILayer(z_layer);
    for (int i=0; i < recs.size; i++) {
        int y = recs[i].y + recs[i].height;
        DrawLine(recs[i].x, y, recs[i].x + recs[i].width, y, text_color);
    }
    EndRenderInUILayer();
}

void TextBox::DrawTexture(Texture2D texture, Rectangle source, int texture_height, Color tint, 
                          text_alignment::T alignment, TextureDrawMode draw_mode) {
    int texture_width = texture_height * source.width / source.height;
    
    Vector2 pos = ApplyAlignmentInRect(GetRect(), { (float)texture_width, (float)texture_height }, alignment);
    Rectangle destination = { pos.x, pos.y, (float)texture_width, (float)texture_height };

    bool outside_render_rect = !CheckCollisionRecs(render_rec, destination);
    bool inside_render_rect = CheckEnclosingRecs(render_rec, destination);
    if (outside_render_rect) {
        return;
    }
    if (!inside_render_rect && !flexible) {
        BeginScissorMode(render_rec.x, render_rec.y, render_rec.width, render_rec.height);
    }

    switch (draw_mode) {
    case TEXTURE_DRAW_SDF:{
        DrawTextureSDF(texture, source, destination, Vector2Zero(), 0, tint, z_layer);
        break;
    }
    case TEXTURE_DRAW_DEFAULT:{
        BeginRenderInUILayer(z_layer, BLANK);
        DrawTexturePro(texture, source, destination, Vector2Zero(), 0, tint);
        EndRenderInUILayer();
        break;
    }
    case TEXTURE_DRAW_RAW:{
        BeginRenderInUILayer(z_layer, BLANK, true);
        DrawTexturePro(texture, source, destination, Vector2Zero(), 0, tint);
        EndRenderInUILayer();
        break;
    }
    }

    if (!inside_render_rect && !flexible) {
        EndScissorMode();
    }
    x_cursor = 0;
    y_cursor += height;
    RecalculateExtends();
}

button_state_flags::T TextBox::WriteButton(const char* text) {
    // text fully in render rectangle    
    button_state_flags::T button_state = AsButton();
    HandleButtonSound(button_state);
    if (button_state & button_state_flags::HOVER) {
        text_color = Palette::interactable_main;
        Enclose(0, 4, background_color, text_color);
    }

    Write(text, text_alignment::CENTER);
    text_color = Palette::ui_main;

    return button_state;
}

button_state_flags::T TextBox::AsButton() const {
    return GetButtonStateRec(GetRect());
}

Vector2 TextBox::GetTextCursor() const {
    return {
        (float)(x + x_cursor),
        (float)(y + y_cursor)
    };
}

Rectangle TextBox::GetRect() const {
    // Different from render_rec e.g. in case of ScrollInset
    return { (float)x, (float)y, (float)width, (float)height};
}

int TextBox::GetLineHeight() const {
    return text_size;
}

text::Layout TextBox::GetTextLayout(const char *text, text_alignment::T alignment) {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(text_size), text, text_size, 1);
    Vector2 pos = ApplyAlignment(GetAnchorPointText(alignment), size, alignment);
    text::Layout text_layout;
    text::GetLayout(&text_layout, pos, GetCustomDefaultFont(text_size), text, text_size, 1, width);
    return text_layout;
}

Rectangle TextBox::TbMeasureText(const char* text, text_alignment::T alignment) const {
    Vector2 size = MeasureTextEx(GetCustomDefaultFont(text_size), text, text_size, 1);
    Vector2 pos = ApplyAlignment(GetAnchorPointText(alignment), size, alignment);
    return {pos.x, pos.y, size.x, size.y};
}

Vector2 TextBox::GetAnchorPoint(text_alignment::T align) const {
    Vector2 res = { (float)x, (float)y };
    switch(align & text_alignment::HFILTER){
        case text_alignment::HCENTER: res.x += width/2; break;
        case text_alignment::RIGHT: res.x += width; break;
        case text_alignment::HCONFORM: res.x += x_cursor; break;
    }
    switch(align & text_alignment::VFILTER){
        case text_alignment::VCENTER: res.y += height/2; break;
        case text_alignment::BOTTOM: res.y += height; break;
        case text_alignment::VCONFORM: res.y += y_cursor; break;
    }
    return res;
}

Vector2 TextBox::GetAnchorPointText(text_alignment::T align) const {
    Vector2 res = { (float)x + text_margin_x, (float)y + text_margin_y };
    int w2 = width - 2 * text_margin_x;
    int h2 = height - 2 * text_margin_y;
    switch(align & text_alignment::HFILTER){
        case text_alignment::HCENTER: res.x += w2/2; break;
        case text_alignment::RIGHT: res.x += w2; break;
        case text_alignment::HCONFORM: res.x += x_cursor; break;
    }
    switch(align & text_alignment::VFILTER){
        case text_alignment::VCENTER: res.y += h2/2; break;
        case text_alignment::BOTTOM: res.y += h2; break;
        case text_alignment::VCONFORM: res.y += y_cursor; break;
    }
    return res;
}

void ui::PushTextBox(TextBox tb) {
    if (GetUI()->text_box_stack == NULL) {
        GetUI()->text_box_stack = (TextBox*) malloc(5 * sizeof(TextBox));
        GetUI()->text_box_stack_capacity = 5;
    }
    if (GetUI()->text_box_stack_index + 1 >= GetUI()->text_box_stack_capacity) {
        int new_capacity = GetUI()->text_box_stack_index + 5;
        GetUI()->text_box_stack = (TextBox*) realloc(GetUI()->text_box_stack, new_capacity * sizeof(TextBox));
        GetUI()->text_box_stack_capacity = new_capacity;
    }
    GetUI()->text_box_stack[GetUI()->text_box_stack_index] = tb;
    GetUI()->text_box_stack_index++;
    if (GetSettingBool("draw_renderrects", false)) {
        BeginRenderInUILayer(tb.z_layer);
        DrawRectangleLinesEx(tb.render_rec, 1, GREEN);
        EndRenderInUILayer();
    }
}

void ui::PushGlobal(int x, int y, int w, int h, int text_size, Color color, Color background, uint8_t z_layer) {
    TextBox new_text_box = TextBox(x, y, w, h, text_size, color, background, z_layer);
    ui::PushTextBox(new_text_box);
    GetUI()->AddBlockingRect({(float)x, (float)y, (float)w, (float)h}, z_layer);
}

void ui::CreateNew(int x, int y, int w, int h, int text_size, Color color, Color background, uint8_t z_layer) {
    while (GetUI()->text_box_stack_index > 0) {  // Clear stack
        ui::Pop();
    }
    ui::PushGlobal(x, y, w, h, text_size, color, background, z_layer);
}

void ui::PushMouseHint(Vector2 mousepos, int width, int height, uint8_t z_layer) {
    const int border_margin = 2;
    int x_pos = ClampInt(mousepos.x, border_margin, GetScreenWidth() - width - border_margin);
    int y_pos = ClampInt(mousepos.y, border_margin, GetScreenHeight() - height - border_margin);
    ui::PushGlobal(x_pos, y_pos, width, height, DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, z_layer);
}

void ui::PushFree(int x, int y, int w, int h) {
    TextBox* tb = ui::Current();
    TextBox new_text_box = TextBox(tb, x, y, w, h);
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb->render_rec);
    ui::PushTextBox(new_text_box);
}

void _FlexibleInsetPopAction(TextBox* parent, TextBox* child) {
    parent->y_cursor += child->height;
    parent->RecalculateExtends();
}

int ui::PushInset(int h) {
    // Returns the actual height
    bool is_flexible = false;
    if (h < 0) {
        h = 1;
        is_flexible = true;
    }
    TextBox* tb = ui::Current();
    if (tb == NULL) {
        FAIL("Cannot push inset textbox as root box")
    }
    tb->EnsureLineBreak();
    if (!tb->flexible) {  // Limit height according to parent
        h = fmin(h, fmax(0, tb->height - tb->y_cursor));
    }
    TextBox new_text_box = TextBox(tb,
        tb->x + tb->x_cursor,
        tb->y + tb->y_cursor,
        tb->width - tb->x_cursor,
        h
    );
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb->render_rec);

    if (is_flexible) {
        new_text_box.flexible = true;
        new_text_box.extend_x = new_text_box.width - 2 * new_text_box.text_margin_x;  // Keep width
        new_text_box.on_pop_action = _FlexibleInsetPopAction;
    } else {
        tb->y_cursor += h;
        tb->RecalculateExtends();
    }
    ui::PushTextBox(new_text_box);
    return h;
}

int ui::PushScrollInset(int margin, int h, int allocated_height, int* scroll) {
    // Returns the actual height
    int buildin_scrollbar_margin = 3;
    int buildin_scrollbar_width = 6;

    if (ui::AsButton() & button_state_flags::HOVER) {
        int max_scroll = MaxInt(allocated_height - ui::Current()->height, 0);
        *scroll = ClampInt(*scroll - GetMouseWheelMove() * 20, 0, max_scroll);
    }

    TextBox* tb = ui::Current();
    tb->EnsureLineBreak();
    int height = fmin(h, fmax(0, tb->height - tb->y_cursor - 2*margin));
    float scroll_progress = Clamp((float)(*scroll) / (allocated_height - h), 0, 1);
    int scrollbar_height = h * h / allocated_height;
    if (allocated_height > h) {
        BeginRenderInUILayer(tb->z_layer + 1);
        DrawRectangleRounded({
            (float) tb->x + tb->width - buildin_scrollbar_margin,
            (float) tb->y + scroll_progress * (h - scrollbar_height),
            (float) buildin_scrollbar_width,
            (float) scrollbar_height},
            buildin_scrollbar_width/2,
            1, tb->text_color
        );
        EndRenderInUILayer();
    } else {
        buildin_scrollbar_margin = 0;
        buildin_scrollbar_width = 0;
    }
    TextBox new_text_box = TextBox(tb,
        tb->x + tb->x_cursor + margin,
        tb->y + tb->y_cursor + margin - *scroll,
        tb->width - tb->x_cursor - 2*margin - 2*buildin_scrollbar_margin - buildin_scrollbar_width,
        allocated_height
    );
    new_text_box.render_rec.y = tb->y + tb->y_cursor + margin;
    new_text_box.render_rec.height = height;

    tb->y_cursor += h + 2*margin;
    tb->RecalculateExtends();
    ui::PushTextBox(new_text_box);
    return height;
}

void ui::PushInline(int width, int height) {
    TextBox* tb = ui::Current();
    if (width > tb->width - tb->x_cursor)
        width = tb->width - tb->x_cursor;
    TextBox new_tb = TextBox(tb,
        tb->x + tb->x_cursor,
        tb->y + tb->y_cursor,
        width,
        height
    );
    new_tb.render_rec = GetCollisionRec(new_tb.render_rec, tb->render_rec);

    tb->_Advance(
        { (float)new_tb.x, (float)new_tb.y }, 
        { (float)new_tb.width, (float)new_tb.height }
    );
    ui::PushTextBox(new_tb);
}

void ui::PushAligned(int width, int height, text_alignment::T alignment) {
    TextBox* tb = ui::Current();
    tb->EnsureLineBreak();

    int x = tb->x;
    int y = tb->y;

    switch(alignment & text_alignment::HFILTER){
        case text_alignment::HCENTER: x += (tb->width - width) / 2; break;
        case text_alignment::RIGHT: x += tb->width - width; break;
        case text_alignment::HCONFORM: x += tb->x_cursor + tb->text_margin_x; break;
    }
    switch(alignment & text_alignment::VFILTER){
        case text_alignment::VCENTER: y += (tb->height - height) / 2; break;
        case text_alignment::BOTTOM: y += (tb->height - height); break;
        case text_alignment::VCONFORM: y += tb->y_cursor + tb->text_margin_y; break;
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
        tb->x + x_start,
        tb->y,
        x_end - x_start,
        tb->height
    );
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb->render_rec);
    ui::PushTextBox(new_text_box);
}

void ui::PushGridCell(int columns, int rows, int column, int row) {
    TextBox* tb = ui::Current();
    TextBox new_text_box = TextBox(tb,
        tb->x + column * tb->width / columns,
        tb->y + row * tb->height / rows,
        tb->width / columns,
        tb->height / rows
    );
    new_text_box.render_rec = GetCollisionRec(new_text_box.render_rec, tb->render_rec);
    ui::PushTextBox(new_text_box);
}

button_state_flags::T ui::AsButton() {
    return ui::Current()->AsButton();
}

void ui::Pop() {
    TextBox* current = ui::Current();
    TextBox* parent = NULL;
    if (GetUI()->text_box_stack_index >= 2) {
        parent = &GetUI()->text_box_stack[GetUI()->text_box_stack_index - 2];
    }
    if (current->on_pop_action != NULL) {
        (*current->on_pop_action)(parent, current);
    }
    GetUI()->text_box_stack_index--;
}

Vector2 ui::GetRelMousePos() {
    return {
        GetMousePosition().x - ui::Current()->x,
        GetMousePosition().y - ui::Current()->y
    };
}

void ui::Shrink(int dx, int dy) {
    ui::Current()->Shrink(dx, dy);
}

void ui::Enclose() {
    ui::Current()->Enclose(0, 4, ui::Current()->background_color, ui::Current()->text_color);
    //Shrink(4, 4);
}

void ui::EncloseEx(int shrink, Color background_color, Color line_color, int corner_radius) {
    ui::Current()->Enclose(shrink, corner_radius, background_color, line_color);
}

void ui::EnclosePartial(int inset, Color background_color, Color line_color, direction::T directions) {
    ui::Current()->EnclosePartial(inset, background_color, line_color, directions);
}

void ui::EncloseDynamic(int shrink, Color background_color, Color line_color, int corner_radius) {
    ui::Current()->EncloseDynamic(shrink, corner_radius, background_color, line_color);
}

void ui::DrawIcon(AtlasPos atlas_index, text_alignment::T alignment, Color tint, int height) {
    if (GetSettingBool("sdf_icons", false)) {
        ui::Current()->DrawTexture(GetUI()->GetIconAtlasSDF(), atlas_index.GetRect(ATLAS_SIZE), 
                                   height, tint, alignment, TextBox::TEXTURE_DRAW_SDF);
    } else {
        ui::Current()->DrawTexture(GetUI()->GetIconAtlas(), atlas_index.GetRect(ATLAS_SIZE), 
                                   height, tint, alignment, TextBox::TEXTURE_DRAW_DEFAULT);
    }
}

void ui::Write(const char *text) {
    ui::WriteEx(text, text_alignment::CONFORM, true);
}

void ui::WriteEx(const char *text, text_alignment::T alignment, bool linebreak) {
    TextBox* tb = ui::Current();
    if (linebreak) {
        tb->WriteLine(text, alignment);
    } else {
        tb->Write(text, alignment);
    }
}

button_state_flags::T ui::WriteButton(const char *text) {
    return ui::Current()->WriteButton(text);
}

void ui::DecorateEx(const text::Layout* layout, const TokenList* tokens) {
    ui::Current()->Decorate(layout, tokens);
}

Rectangle ui::MeasureTextEx(const char *text, text_alignment::T alignment) {
    return ui::Current()->TbMeasureText(text, alignment);
}

void ui::FillLine(double value, Color fill_color, Color background_color) {
    TextBox* tb = ui::Current();
    int y = tb->y + tb->y_cursor + tb->line_size_y;
    int x_start = tb->x;
    int x_end = tb->x + tb->width;
    if (!tb->flexible) {
        x_start = ClampInt(x_start, tb->render_rec.x, tb->render_rec.x + tb->render_rec.width);
        x_end = ClampInt(x_end, tb->render_rec.x, tb->render_rec.x + tb->render_rec.width);
    }
    tb->y_cursor += 2;
    ui::FillLineEx(x_start, x_end, y, value, fill_color, background_color);
}

// Ignores render-rect
void ui::FillLineEx(int x_start, int x_end, int y, double value, Color fill_color, Color background_color) {
    TextBox* tb = ui::Current();
    if (!tb->flexible && (y > tb->render_rec.y + tb->render_rec.height || y < tb->render_rec.y))
        return;
    
    int x_mid_point = ClampInt(x_start + (x_end - x_start) * value, tb->render_rec.x, tb->render_rec.x + tb->render_rec.width);
    BeginRenderInUILayer(tb->z_layer);
    DrawLine(x_start, y, x_end, y, background_color);
    DrawLine(x_start, y, x_mid_point, y, fill_color);
    //DrawLine(x_mid_point, y_end, x_mid_point, y_end - 4, fill_color);
    EndRenderInUILayer();
}

button_state_flags::T ui::DirectButton(const char* text, int margin) {
    TextBox* tb = ui::Current();
    /*ButtonStateFlags::T button_state = tb->WriteButton(text, inset);
    HandleButtonSound(button_state);*/
    Rectangle text_rect = tb->TbMeasureText(text, text_alignment::CONFORM);
    ui::PushInline(text_rect.width + 2*margin, text_rect.height + 2*margin);
    button_state_flags::T button_state = ui::AsButton();
    if (button_state & button_state_flags::HOVER) {
        ui::EncloseEx(2, tb->background_color, Palette::interactable_main, 0);
    } else {
        ui::EncloseEx(2, tb->background_color, tb->text_color, 0);
    }
    ui::WriteEx(text, text_alignment::CENTER, false);
    HandleButtonSound(button_state);
    ui::Pop();
    return button_state;
}

button_state_flags::T ui::ToggleButton(bool on) {
    ui::WriteEx(" ", text_alignment::CONFORM, false);
    button_state_flags::T res = ui::DirectButton(on ? " X " : "   ", -2);
    ui::WriteEx(" ", text_alignment::CONFORM, false);
    return res;
}

int ui::DrawLimitedSlider(int current, int min, int max, int limit, int width, int height, Color fg, Color bg) {
    TextBox* tb = ui::Current();
    //DebugPrintText("%d - (%d - %d), %d", current, min, max, limit);

    double val = (current - min) / (double) (max - min);
    if (val < 0) val = 0;
    if (val > 1) val = 1;
    
    Rectangle primary = { tb->x_cursor + tb->x, tb->y_cursor + tb->y, width, height };
    Rectangle fill = primary;
    fill.width *= val;

    if (current > limit) current = limit;

    ui::BeginDirectDraw();
    //DebugPrintText("%f %f %f %f", primary.x, primary.y, primary.width, primary.height);
    DrawRectangleRec(fill, bg);
    DrawRectangleLinesEx(primary, 1, fg);
    int x_limit = primary.x + primary.width * (limit - min) / (double) (max - min);
    DrawLine(x_limit, primary.y, x_limit, primary.y + primary.height, fg);
    ui::EndDirectDraw();
    Rectangle collision_rec = primary;
    collision_rec.x -= 5;
    collision_rec.width += 10;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), collision_rec)) {
        double new_val = (GetMousePosition().x - primary.x) / primary.width;
        if (new_val < 0) new_val = 0;
        if (new_val > 1) new_val = 1;
        int new_current = min + new_val * (max - min);
        if (new_current > limit) new_current = limit;
        if (new_current < min) new_current = min;
        if (new_current > max) new_current = max;
        return new_current;
    }

    return current;
}

void ui::HelperText(const char* description) {
    if (!GetSettingBool("show_help", true)) {
        return;
    }
    TextBox* tb = ui::Current();
    StringBuilder sb = StringBuilder(description);
    sb.AutoBreak(100);
    Vector2 button_size = { 24, 24 };
    text_alignment::T button_align = text_alignment::TOP | text_alignment::RIGHT;
    Vector2 button_pos = ApplyAlignment(ui::Current()->GetAnchorPoint(button_align), button_size, button_align);
    button_pos.x -= 2;
    button_pos.y += 2;
    Rectangle rect = { button_pos.x, button_pos.y, button_size.x, button_size.y };
    //BeginRenderInUILayer(tb->z_layer);
    //DrawRectangleLinesEx(tb->render_rec, 1, GREEN);
    //EndRenderInUILayer();
    text::DrawTextEx(GetCustomDefaultFont(tb->text_size), "??", { button_pos.x, button_pos.y }, tb->text_size, 1, 
                     Palette::interactable_main, Palette::bg, tb->render_rec, tb->z_layer);

    if (CheckCollisionCircleRec(GetMousePosition(), 2, rect) && !GetUI()->IsPointBlocked(GetMousePosition(), tb->z_layer)) {
        //GetUI()->mousehints.count = 0;
        GetUI()->mousehints.AddHint(rect, { button_pos.x, button_pos.y + button_size.y}, description);
    }
}

void ui::BeginDirectDraw() {
    BeginScissorMode(
        ui::Current()->render_rec.x, ui::Current()->render_rec.y,
        ui::Current()->render_rec.width, ui::Current()->render_rec.height
    );
    BeginRenderInUILayer(ui::Current()->z_layer);
}

void ui::EndDirectDraw() {
    EndRenderInUILayer();
    EndScissorMode();
}

TextBox* ui::Current() {
    if (GetUI()->text_box_stack_index > 0) {
        return &GetUI()->text_box_stack[GetUI()->text_box_stack_index-1];
    }
    return NULL;
}

void ui::HSpace(int pixels) {
    ui::Current()->x_cursor += pixels;
    ui::Current()->RecalculateExtends();
}

void ui::VSpace(int pixels) {
    ui::Current()->y_cursor += pixels;
    ui::Current()->RecalculateExtends();
}

void ui::SetMouseHint(const char* text) {
    Vector2 mousepos = Vector2Add(GetMousePosition(), {5, 5});
    int max_width = GetScreenWidth() - mousepos.x;
    
    text::Layout layout;
    text::GetLayout(&layout, 
        Vector2Add(mousepos, {-4,-4}),
        GetCustomDefaultFont(DEFAULT_FONT_SIZE), text, 
        DEFAULT_FONT_SIZE, 1, max_width
    );

    Rectangle rect = layout.bounding_box;
    ui::PushMouseHint({rect.x, rect.y}, rect.width, rect.height, 255 - MAX_TOOLTIP_RECURSIONS);
    Rectangle true_rect = ui::Current()->GetRect();
    layout.Offset(true_rect.x - rect.x, true_rect.y - rect.y);
    ui::Enclose();
    ui::Current()->WriteLayout(&layout, true);
    ui::Pop();
}

void UIGlobals::UIInit() {
    SetTextureFilter(assets::GetFont("resources/fonts/space_mono_extended_sdf.fnt").texture, TEXTURE_FILTER_BILINEAR);  // Very important step
    SetTextureFilter(assets::GetFont("resources/fonts/space_mono_extended_20_sdf.fnt").texture, TEXTURE_FILTER_BILINEAR);  // Very important step
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
    for (int i=mousehints.count-1; i >= 0; i--) {  // Avoid overdraw
        bool top_most = i == mousehints.count-1;

        // Text manipulation
        StringBuilder sb = StringBuilder(mousehints.hints[i]);
        sb.AutoBreak(100);
        TokenList tokens = sb.ExtractTokens("[[", "]]");
        int trailing_newlines = 0;
        for (;sb.c_str[sb.length - trailing_newlines - 2] == '\n'; trailing_newlines++) { }
        StringBuilder sb_substr = sb.GetSubstring(0, sb.length - trailing_newlines - 1);

        // Push Textbox
        Vector2 text_size = MeasureTextEx(GetCustomDefaultFont(DEFAULT_FONT_SIZE), sb_substr.c_str, DEFAULT_FONT_SIZE, 1);
        mousehints.hint_rects[i].width = text_size.x;
        mousehints.hint_rects[i].height = text_size.y;
        Vector2 anchor = { mousehints.hint_rects[i].x, mousehints.hint_rects[i].y};
        ui::PushMouseHint(anchor, text_size.x+8, text_size.y+8, 255 - MAX_TOOLTIP_RECURSIONS + i);

        text::Layout layout = ui::Current()->GetTextLayout(sb_substr.c_str, text_alignment::CONFORM);
        mousehints.hint_rects[i] = ui::Current()->render_rec;
        
        int hover_char_index = -1;
        int hover_token_index = -1;
        // Check for recursive tooltips
        if (!IsPointBlocked(GetMousePosition(), ui::Current()->z_layer)) {
            int char_index = layout.GetCharacterIndex(GetMousePosition());
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
            Rectangle rect = layout.GetTokenRect(
                tokens.start_positions[hover_token_index], 
                tokens.end_positions[hover_token_index], 
                hover_char_index
            );
            //DrawRectangleLinesEx(rect, 1, GREEN);

            int substr_len =  tokens.end_positions[hover_token_index] - tokens.start_positions[hover_token_index];
            char* substr = new char[substr_len + 1];
            strncpy(substr, &sb_substr.c_str[tokens.start_positions[hover_token_index]], substr_len);
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
        ui::Current()->WriteLayout(&layout, false);
        ui::DecorateEx(&layout, &tokens);
        ui::Current()->AdvanceLayout(&layout);
        if (i == mousehints.count-1) {
            ui::VSpace(3);
            ui::FillLine(mousehints.lock_progress, Palette::ui_main, Palette::bg);
        }
        ui::Pop();
    }

    // Draw connections
    BeginRenderInUILayer(255 - MAX_TOOLTIP_RECURSIONS + mousehints.count);
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
    EndRenderInUILayer();
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
    return assets::GetData("resources/data/concepts.yaml")->Get(key, "No descritpion found", true);
}

Texture2D UIGlobals::GetIconAtlas() {
    return assets::GetFont("resources/fonts/space_mono_extended.fnt").texture;
}

Texture2D UIGlobals::GetIconAtlasSDF() {
    return assets::GetFont("resources/fonts/space_mono_extended_sdf.fnt").texture;
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

Font GetCustomDefaultFont(int size) {
    Font font;
    if (GetSettingBool("sdf_text", false)) {
        if (size <= 20) {
            font = assets::GetFont("resources/fonts/space_mono_extended_20_sdf.fnt");
        } else {
            font = assets::GetFont("resources/fonts/space_mono_extended_sdf.fnt");
        }
    } else  {
        if (size <= 20) {
            font = assets::GetFont("resources/fonts/space_mono_extended_20.fnt");
        } else {
            font = assets::GetFont("resources/fonts/space_mono_extended.fnt");
        }
    }
    return font;
}

button_state_flags::T DrawTriangleButton(Vector2 point, Vector2 base, double width, Color color) {
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

button_state_flags::T DrawCircleButton(Vector2 midpoint, double radius, Color color) {
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