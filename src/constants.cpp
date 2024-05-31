#include "constants.hpp"

Color Palette::blue = GetColor(0x2395A9FFu);
//Color Palette::blue = GetColor(0x6495EDFFu);
Color Palette::green = GetColor(0x4Fc76CFFu);
Color Palette::red = GetColor(0xFF5155FFu);
Color Palette::white = GetColor(0xFFFFA0FFu);

Color Palette::bg = GetColor(0x1D2025FFu);

Color Palette::ui_main = Palette::white;
Color Palette::ui_dark = ColorAlphaBlend(Palette::bg, Palette::ui_main, GetColor(0x50505050u));
Color Palette::ui_alt = ColorAlphaBlend(Palette::bg, Palette::ui_main, GetColor(0x80808080u));

Color Palette::interactable_main = Palette::blue;
Color Palette::interactable_dark = ColorAlphaBlend(Palette::bg, Palette::interactable_main, GetColor(0x50505050u));
Color Palette::interactable_alt = ColorAlphaBlend(Palette::bg, Palette::interactable_main, GetColor(0x80808080u));

Color Palette::ally = Palette::blue;
Color Palette::ally_dark = ColorAlphaBlend(Palette::bg, Palette::ally, GetColor(0x50505050u));
Color Palette::ally_alt = ColorAlphaBlend(Palette::bg, Palette::ally, GetColor(0x80808080u));

Color Palette::enemy = Palette::red;
Color Palette::enemy_dark = ColorAlphaBlend(Palette::bg, Palette::enemy, GetColor(0x50505050u));
Color Palette::enemy_alt = ColorAlphaBlend(Palette::bg, Palette::enemy, GetColor(0x80808080u));

#define EXPORT_COLOR(name) fprintf(f, "%d %d %d %s\n", Palette::name.r, Palette::name.g, Palette::name.b, #name);
void Palette::ExportToFile(const char* filename) {
    FILE* f = fopen(filename, "wt");
    if (f == NULL) {
        printf("Could not export palette to '%s'", filename);
        return;
    }
    fprintf(f, "GIMP Palette\n");
    fprintf(f, "Name: retroscreen\n");
    fprintf(f, "Columns: 1\n");

    EXPORT_COLOR(bg)
    EXPORT_COLOR(green)
    EXPORT_COLOR(ui_main)
    EXPORT_COLOR(ui_dark)
    EXPORT_COLOR(ui_alt)
    EXPORT_COLOR(interactable_main)
    EXPORT_COLOR(interactable_dark)
    EXPORT_COLOR(interactable_alt)
    EXPORT_COLOR(enemy)
    EXPORT_COLOR(enemy_dark)
    EXPORT_COLOR(enemy_alt)

    fclose(f);
}