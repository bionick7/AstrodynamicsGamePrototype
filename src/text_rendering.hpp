#ifndef TEXTUTILS_H
#define TEXTUTILS_H

#include "basic.hpp"
#include "string_builder.hpp"
#include "list.hpp"

namespace text {
    struct Layout {
        Rectangle* rects;
        Rectangle bounding_box;
        const char* text;  // Does not own
        int size;
        
        ~Layout();

        void Offset(int x, int y);

        int GetCharacterIndex(Vector2 position) const;
        Rectangle GetTokenRect(int token_start, int token_end, int token_from) const;
        void GetTextRects(List<Rectangle>* buffer, const TokenList *tokens) const;
        void DrawTextLayout(Font font, float fontSize, Color tint, Color background, Rectangle render_rect, uint8_t z_layer) const;
    };

    void GetLayout(Layout* layout, Vector2 start, Font font, const char *text, float fontSize, float spacing);
    void DrawText(const char *text, Vector2 position, Color color);
    void DrawTextEx(Font font, const char *text, Vector2 position, 
                            float fontSize, float spacing, Color tint);
    void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, 
                            float spacing, Color tint, Rectangle render_rect, uint8_t z_layer);
}

#endif  // TEXTUTILS_H