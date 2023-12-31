#include "constants.hpp"

Color Palette::blue = GetColor(0x006D80FFu);
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

Color Palette::ship = Palette::green;
Color Palette::ship_dark = ColorAlphaBlend(Palette::bg, Palette::ship, GetColor(0x50505050u));
Color Palette::ship_alt = ColorAlphaBlend(Palette::bg, Palette::ship, GetColor(0x80808080u));
