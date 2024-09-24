#include "text_rendering.hpp"
#include "render_utils.hpp"
#include "ui.hpp"
#include "debug_console.hpp"

void text::GetLayout(Layout* layout, Vector2 start, Font font, 
                     const char *text, float fontSize, float spacing, int max_width) {

    if (font.texture.id == 0) font = GetFontDefault();
    layout->size = TextLength(text);
    layout->text = text;

    float text_offset_x = start.x;
    float text_offset_y = start.y;

    float scaleFactor = fontSize/font.baseSize;

    layout->bounding_box.x = start.x;
    layout->bounding_box.y = start.y;
    layout->bounding_box.width = 0;
    layout->rects = new Rectangle[layout->size];

    int last_wordbreak = 0;
    bool reiterating = false;

    for (int i = 0; i < layout->size;) {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // Handle auto-breaks
        if (codepoint == ' ' || codepoint == '\t') {
            if (text_offset_x - start.x > max_width && !reiterating) {
                i = last_wordbreak;

                int max_offset = layout->rects[i].x + layout->rects[i].width;
                if (max_offset > layout->bounding_box.x + layout->bounding_box.width) {
                    layout->bounding_box.width = max_offset - layout->bounding_box.x;
                }
                
                text_offset_x = start.x;
                text_offset_y += 20;
                reiterating = true;
            } else {
                reiterating = false;
            }
            last_wordbreak = i + codepointByteCount;
        }

        float x_increment = (float)font.recs[index].width*scaleFactor + spacing;
        layout->rects[i] = { text_offset_x, text_offset_y, x_increment, fontSize };

        if (codepoint == '\n') {
            if (text_offset_x > layout->bounding_box.x + layout->bounding_box.width) {
                layout->bounding_box.width = text_offset_x - layout->bounding_box.x;
            }
            text_offset_x = start.x;
            text_offset_y += 20;
        } else {
            if (font.glyphs[index].advanceX == 0) text_offset_x += x_increment;
            else text_offset_x += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
        }

        i += codepointByteCount;
    }
    
    if (text_offset_x > layout->bounding_box.x + layout->bounding_box.width) {
        layout->bounding_box.width = text_offset_x - layout->bounding_box.x;
    }
    layout->bounding_box.height = text_offset_y + 20 - layout->bounding_box.y;
}

text::Layout::Layout() {
    size = 0;
    rects = new Rectangle[size];
    bounding_box = Rectangle();
    text = NULL;  // Does not own
}

text::Layout::~Layout() {
    delete[] rects;
}

void text::Layout::Offset(int x, int y) {
    bounding_box.x += x;
    bounding_box.y += y;
    for (int i = 0; i < size; i++) {
        rects[i].x += x;
        rects[i].y += y;
    }
}

int text::Layout::GetCharacterIndex(Vector2 position) const {
    // Returns byte offset into the char array, rather than glyph position
    // Returns -1 if position is outside text region

    for (int i = 0; i < size;) {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);

        if (CheckCollisionPointRec(position, rects[i])) {
            return i;
        }

        i += codepointByteCount;
    }
    return -1;
}

Rectangle text::Layout::GetTokenRect(int token_start, int token_end, int token_from) const {
    Rectangle rect = Rectangle{ 0, 0, 0, 20 };

    for (int i = 0; i < size;) {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);

        int x = rects[i].x;
        int y = rects[i].y;

        if (i == token_start) {
            rect.x = x;
            rect.y = y;
        }
        else if (i == token_end) {
            rect.width = x - rect.x;
            return rect;
        }
        if (codepoint == '\n') {
            if (i > token_from) {
                rect.width = x - rect.x;
                return rect;
            }
            x = 0.0f;
            y += 20;
            if (i > token_start) {
                rect.x = x;
                rect.y = y;
            }
        }
        i += codepointByteCount;
    }
    return rect;
}

void text::Layout::GetTextRects(List<Rectangle>* buffer, const TokenList *tokens) const {
    if (tokens->length == 0) return;

    int current_token_index = 0;
    bool in_token = false;

    Rectangle rect = Rectangle{ 0, 0, 0, 20 };

    int current_start_pos;
    int current_end_pos;

    for (int i = 0; i < size;) {
        if (current_token_index < tokens->length) {
            current_start_pos = tokens->start_positions[current_token_index];
            current_end_pos = tokens->end_positions[current_token_index];
        } else {
            current_start_pos = -1;
            current_end_pos = -1;
        }

        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);

        int x = rects[i].x;
        int y = rects[i].y;
        
        if (i == current_start_pos) {
            rect.x = x;
            rect.y = y;
            in_token = true;
        }
        else if (i == current_end_pos) {
            rect.width = x - rect.x;
            buffer->Append(rect);
            in_token = false;
            current_token_index++;
        }
        if (codepoint == '\n') {
            if (in_token) {
                rect.width = x - rect.x;
                buffer->Append(rect);
            }
            if (in_token) {
                rect.x = x;
                rect.y = y;
            }
        }

        i += codepointByteCount;
    }
}

void text::Layout::DrawTextLayout(Font font, float fontSize, Color tint, Color background, Rectangle render_rect, uint8_t z_layer) const {
    // Modified Raylib function '(Raylib cmt)' refers to original documentation

    if (font.texture.id == 0) font = GetFontDefault();  // (Raylib cmt) Security check in case of not valid font

    if (GetSettingBool("sdf_text", false)) {
        BeginRenderSDFInUILayer(z_layer, background);
    } else {
        BeginRenderInUILayer(z_layer, background);
    }

    for (int i = 0; i < size;) {
        // (Raylib cmt)Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        if (codepoint == '\n') {
            
        } else {
            // This part is custom
            if (
                (codepoint != ' ') && (codepoint != '\t')
                && CheckCollisionRecs(rects[i], render_rect)
            ) {
                Vector2 char_pos = { roundf(rects[i].x), roundf(rects[i].y) };
                //Vector2 char_pos = { rects[i].x, rects[i].y };
                DrawTextCodepoint(font, codepoint, char_pos, fontSize, tint);
            }
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }

    EndRenderInUILayer();
}


void text::DrawText(const char *text, Vector2 position, Color color) {
    text::DrawTextEx(GetCustomDefaultFont(DEFAULT_FONT_SIZE), text, position, DEFAULT_FONT_SIZE, 1, color, BLANK, GetScreenRect(), 0);
}

void text::DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
    text::DrawTextEx(font, text, position, fontSize, spacing, tint, BLANK, GetScreenRect(), 0);
}

void text::DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, 
                        Color tint, Color bg, Rectangle render_rect, uint8_t z_layer) {
    Layout layout;
    text::GetLayout(&layout, position, font, text, fontSize, spacing, render_rect.width);
    layout.DrawTextLayout(font, fontSize, tint, bg, render_rect, z_layer);
}
