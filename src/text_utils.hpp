#ifndef TEXTUTILS_H
#define TEXTUTILS_H

#include "basic.hpp"
#include "string_builder.hpp"


std::vector<Rectangle> PrepareText(Vector2 position, Font font, const char *text, float fontSize, float spacing);
int GetCharacterIndex(Vector2 position, Font font, const char *text, float fontSize, float spacing);
Rectangle GetTextRect(int token_start, int token_end, int token_from, Font font, const char *text, float fontSize, float spacing);
std::vector<Rectangle> GetTextRects(const TokenList* tokens, Font font, const char *text, float fontSize, float spacing);

void InternalDrawText(const char *text, Vector2 position, Color color);
void InternalDrawTextEx(Font font, const char *text, Vector2 position, 
                        float fontSize, float spacing, Color tint);
void InternalDrawTextEx(Font font, const char *text, Vector2 position, float fontSize, 
                        float spacing, Color tint, Rectangle render_rect, uint8_t z_layer);

#endif  // TEXTUTILS_H