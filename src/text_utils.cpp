#include "text_utils.hpp"
#include "render_utils.hpp"
#include "ui.hpp"
#include "debug_console.hpp"

std::vector<Rectangle> PrepareText(int token_start, int token_end, int token_from, Font font, const char *text, float fontSize, float spacing) {
    if (font.texture.id == 0) font = GetFontDefault();
    int size = TextLength(text);

    int textOffsetY = 0;
    float textOffsetX = 0.0f;

    float scaleFactor = fontSize/font.baseSize;

    for (int i = 0; i < size;) {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        if (codepoint == '\n') {
            textOffsetY += 20;
            textOffsetX = 0.0f;
        } else {
            float x_increment = (float)font.recs[index].width*scaleFactor + spacing;
            Rectangle char_rect = { textOffsetX, textOffsetY, x_increment, fontSize };
            


            if (font.glyphs[index].advanceX == 0) textOffsetX += x_increment;
            else textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }
    return std::vector<Rectangle>();
}

int GetCharacterIndex(Vector2 position, Font font, const char *text, float fontSize, float spacing) {
    // Returns byte offset into the char array, rather than glyph position
    // Returns -1 if position is outside text region
    if (font.texture.id == 0) font = GetFontDefault();
    int size = TextLength(text);

    int textOffsetY = 0;
    float textOffsetX = 0.0f;

    float scaleFactor = fontSize/font.baseSize;

    for (int i = 0; i < size;) {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        if (codepoint == '\n') {
            textOffsetY += 20;
            textOffsetX = 0.0f;
        } else {
            float x_increment = (float)font.recs[index].width*scaleFactor + spacing;
            Rectangle char_rect = { textOffsetX, textOffsetY, x_increment, fontSize };
            if (CheckCollisionPointRec(position, char_rect)) {
                return i;
            }

            if (font.glyphs[index].advanceX == 0) textOffsetX += x_increment;
            else textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }
    return -1;
}

Rectangle GetTextRect(int token_start, int token_end, int token_from, Font font, const char *text, float fontSize, float spacing) {
    if (font.texture.id == 0) font = GetFontDefault();
    int size = TextLength(text);

    int textOffsetY = 0;
    float textOffsetX = 0.0f;

    float scaleFactor = fontSize/font.baseSize;

    Rectangle rect = Rectangle{ 0, 0, 0, 20 };

    for (int i = 0; i < size;) {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);
        if (i == token_start) {
            rect.x = textOffsetX;
            rect.y = textOffsetY;
        }
        else if (i == token_end) {
            rect.width = textOffsetX - rect.x;
            return rect;
        }
        if (codepoint == '\n') {
            if (i > token_from) {
                rect.width = textOffsetX - rect.x;
                return rect;
            }
            textOffsetY += 20;
            textOffsetX = 0.0f;
            if (i > token_start) {
                rect.x = textOffsetX;
                rect.y = textOffsetY;
            }
        } else {
            float x_increment = (float)font.recs[index].width*scaleFactor + spacing;
            //Rectangle char_rect = { textOffsetX, textOffsetY, x_increment, fontSize };


            if (font.glyphs[index].advanceX == 0) textOffsetX += x_increment;
            else textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }
    return rect;
}

std::vector<Rectangle> GetTextRects(const TokenList *tokens, Font font, const char *text, float fontSize, float spacing) {
    std::vector<Rectangle> res = std::vector<Rectangle>();
    if (tokens->length == 0) return res;

    if (font.texture.id == 0) font = GetFontDefault();
    int size = TextLength(text);

    int textOffsetY = 0;
    float textOffsetX = 0.0f;

    float scaleFactor = fontSize/font.baseSize;

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
        int index = GetGlyphIndex(font, codepoint);
        if (i == current_start_pos) {
            rect.x = textOffsetX;
            rect.y = textOffsetY;
            in_token = true;
        }
        else if (i == current_end_pos) {
            rect.width = textOffsetX - rect.x;
            res.push_back(rect);
            in_token = false;
            current_token_index++;
        }
        if (codepoint == '\n') {
            if (in_token) {
                res.push_back(rect);
            }
            textOffsetY += 20;
            textOffsetX = 0.0f;
            if (in_token) {
                rect.x = textOffsetX;
                rect.y = textOffsetY;
            }
        } else {
            float x_increment = (float)font.recs[index].width*scaleFactor + spacing;
            //Rectangle char_rect = { textOffsetX, textOffsetY, x_increment, fontSize };


            if (font.glyphs[index].advanceX == 0) textOffsetX += x_increment;
            else textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }
    return res;
}

void InternalDrawText(const char *text, Vector2 position, Color color) {
    InternalDrawTextEx(GetCustomDefaultFont(), text, position, DEFAULT_FONT_SIZE, 1, color, GetScreenRect(), 0);
}

void InternalDrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
    InternalDrawTextEx(font, text, position, fontSize, spacing, tint, GetScreenRect(), 0);
}

void InternalDrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, 
                        Color tint, Rectangle render_rect, uint8_t z_layer) {
    if (font.texture.id == 0) font = GetFontDefault();  // (Raylib cmt) Security check in case of not valid font

    int size = TextLength(text);    // (Raylib cmt) Total size in bytes of the text, scanned by codepoints in loop

    int textOffsetY = 0;            // (Raylib cmt) Offset between lines (on linebreak '\n')
    float textOffsetX = 0.0f;       // (Raylib cmt) Offset X to next character to draw

    float scaleFactor = fontSize/font.baseSize;         // (Raylib cmt) Character quad scaling factor

    //if (GetSettingBool("sdf_text", false)) {
    //    RELOAD_IF_NECAISSARY(sdf_shader)
    //    float z_layer_f = 1.0f - z_layer / 256.0f;
    //    SetShaderValue(sdf_shader::shader, sdf_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT);
    //    BeginShaderMode(sdf_shader::shader);
    //}

    for (int i = 0; i < size;) {
        // (Raylib cmt)Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        if (codepoint == '\n') {
            // (Raylib cmt) NOTE: Line spacing is a global variable, use SetTextLineSpacing() to setup
            textOffsetY += 20;  // TODO, raylib doesn't expose line space, so how to?
            textOffsetX = 0.0f;
        } else {
            float x_increment = (float)font.recs[index].width*scaleFactor + spacing;
            Vector2 char_pos = { position.x + textOffsetX, position.y + textOffsetY };

            // This part is custom

            Vector2 char_pos2 = { position.x + textOffsetX + x_increment, position.y + textOffsetY + fontSize };
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

    //if (GetSettingBool("sdf_text", false)) {
    //    EndShaderMode();
    //}
}