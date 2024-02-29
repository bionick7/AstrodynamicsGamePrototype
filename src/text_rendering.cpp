#include "text_rendering.hpp"
#include "render_utils.hpp"
#include "ui.hpp"
#include "debug_console.hpp"

void text::GetLayout(Layout* layout, Vector2 start, Font font, const char *text, float fontSize, float spacing) {
    if (font.texture.id == 0) font = GetFontDefault();
    layout->size = TextLength(text);
    layout->text = text;

    float textOffsetX = start.x;
    float textOffsetY = start.y;

    float scaleFactor = fontSize/font.baseSize;

    layout->bounding_box.x = start.x;
    layout->bounding_box.y = start.y;
    layout->bounding_box.width = 0;
    layout->rects = new Rectangle[layout->size];

    for (int i = 0; i < layout->size;) {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        float x_increment = (float)font.recs[index].width*scaleFactor + spacing;
        layout->rects[i] = { textOffsetX, textOffsetY, x_increment, fontSize };

        if (codepoint == '\n') {
            if (textOffsetX > layout->bounding_box.x + layout->bounding_box.width) {
                layout->bounding_box.width = textOffsetX - layout->bounding_box.x;
            }
            textOffsetX = start.x;
            textOffsetY += 20;
        } else {
            if (font.glyphs[index].advanceX == 0) textOffsetX += x_increment;
            else textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }
    
    if (textOffsetX > layout->bounding_box.x + layout->bounding_box.width) {
        layout->bounding_box.width = textOffsetX - layout->bounding_box.x;
    }
    layout->bounding_box.height = textOffsetY + 20 - layout->bounding_box.y;
}

text::Layout::~Layout() {
    delete[] rects;
}

int text::Layout::GetCharacterIndex(Vector2 position) const
{
    // Returns byte offset into the char array, rather than glyph position
    // Returns -1 if position is outside text region

    for (int i = 0; i < size;) {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);

        if (CheckCollisionPointRec(position, rects[i])) {
            return i;
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
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

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }
}

void text::Layout::DrawTextLayout(Font font, float fontSize, Color tint, Rectangle render_rect, uint8_t z_layer) const {
    if (font.texture.id == 0) font = GetFontDefault();  // (Raylib cmt) Security check in case of not valid font

    if (GetSettingBool("sdf_text", false)) {
        RELOAD_IF_NECAISSARY(sdf_shader)
        float z_layer_f = 1.0f - z_layer / 256.0f;
        SetShaderValue(sdf_shader::shader, sdf_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(sdf_shader::shader);
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
                Vector2 char_pos = { rects[i].x, rects[i].y };
                DrawTextCodepoint(font, codepoint, char_pos, fontSize, tint);
            }
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }

    if (GetSettingBool("sdf_text", false)) {
        EndShaderMode();
    }
}

void text::DrawText(const char *text, Vector2 position, Color color) {
    text::DrawTextEx(GetCustomDefaultFont(), text, position, DEFAULT_FONT_SIZE, 1, color, GetScreenRect(), 0);
}

void text::DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
    text::DrawTextEx(font, text, position, fontSize, spacing, tint, GetScreenRect(), 0);
}

void text::DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, 
                        Color tint, Rectangle render_rect, uint8_t z_layer) {
    Layout layout;
    text::GetLayout(&layout, position, font, text, fontSize, spacing);
    layout.DrawTextLayout(font, fontSize, tint, render_rect, z_layer);
}
