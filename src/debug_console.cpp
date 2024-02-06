#include "debug_console.hpp"
#include "raylib.h"
#include "ui.hpp"
#include "constants.hpp"
#include "global_state.hpp"
#include "basic.hpp"

namespace settings {
    Setting* list = NULL;
    int capacity = 0;
    int size = 0;

    void Init() {
        // TODO: Load file
        list = new Setting[3];
        list[0] = Setting("a", "1");
        list[1] = Setting("b", "2");
        list[2] = Setting("c", "3");
        size = 3;
        capacity = 3;
    }
}

Setting::Setting(const char* name, const char* value) {
    strcpy(this->name, name);
    strcpy(overrides[0], value);
    override_index = 0;
}

Setting* GetSetting(const char* key) {
    if (settings::list == NULL) { settings::Init(); }
    for(int i=0; i < settings::size; i++) {
        if (strcmp(settings::list[i].name, key) == 0) {
            return &settings::list[i];
        }
    }
    return NULL;
}

const char* GetSettingValue(const char* key, const char* default_) {
    Setting* setting = GetSetting(key);
    if (setting == NULL) {
        return default_;
    }
    return setting->Get();
}

const char *Setting::Get() {
    return overrides[override_index];
}

void Setting::Set(const char *value) {
    strcpy(overrides[0], value);
}

void Setting::PushOverride(const char *value) {
    strcpy(overrides[override_index], value);
    if (override_index < SETTING_MAX_OVERRIDES - 1) {
        override_index++;
    }
}

void Setting::PopOverride() {
    if (override_index > 0) {
        override_index--;
    }
}

#define DEBUG_CONSOLE_MAX_LINES 128
#define DEBUG_CONSOLE_PROMPT_BACKLOG_SIZE 128
#define DEBUG_CONSOLE_MAX_LINE_SIZE 1024
namespace debug_console {
    char lines[DEBUG_CONSOLE_MAX_LINE_SIZE][DEBUG_CONSOLE_MAX_LINES] = {0};
    char prompt_backlog[DEBUG_CONSOLE_MAX_LINE_SIZE][DEBUG_CONSOLE_PROMPT_BACKLOG_SIZE] = {0};
    char current_prompt[DEBUG_CONSOLE_MAX_LINE_SIZE] = "";

    int line_index = 0;
    int cursor = 0;
    int prompt_backlog_offset = 0;
    int prompt_backlog_index = 0;

    bool is_console_shown = true;
}

using namespace debug_console;

void PushLine(const char* line) {
    strcpy(lines[line_index], line);
    line_index = (line_index + 1) % DEBUG_CONSOLE_MAX_LINES;
}

const char* FetchArg(char* arg, const char* inp) {
    for(int i=0; ;i++, inp++) {
        if (*inp == '\0') {
            arg[i] = '\0';
            return inp;
        }
        if (*inp == ' ' || *inp == '\t') {
            arg[i] = '\0';
            return inp+1;
        }
        arg[i] = *inp;
    }
    NOT_REACHABLE
    return NULL;
}

void SetSetting(const char* prompt) {
    static char setting_name[DEBUG_CONSOLE_MAX_LINE_SIZE];
    static char setting_value[DEBUG_CONSOLE_MAX_LINE_SIZE];
    if (settings::list == NULL) { settings::Init(); }
    if (*prompt == '\0') {
        PushLine("No setting name provided");
        return;
    }
    prompt = FetchArg(setting_name, prompt);
    Setting* setting = GetSetting(setting_name);
    if (setting == NULL) {
        PushLine("No such setting");
        return;
    }
    if (*prompt == '\0') {
        PushLine("No setting value provided");
        return;
    }
    prompt = FetchArg(setting_value, prompt);
    setting->Set(setting_value);
}

void ListSettings(const char* prompt) {
    if (settings::list == NULL) { settings::Init(); }
    for(int i=0; i < settings::size; i++) {
        StringBuilder sb;
        sb.Add(settings::list[i].name).Add(" : ").Add(settings::list[i].Get());
        PushLine(sb.c_str);
    }
}

struct { const char* name; void(*func)(const char*); } commands[] = {
    { "set", SetSetting },
    { "list", ListSettings },
};

void InterpreteResult(const char* prompt) {
    static char command_str[DEBUG_CONSOLE_MAX_LINE_SIZE];
    prompt = FetchArg(command_str, prompt);
    //INFO("received prompt '%s'", prompt);
    for(int i=0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (strcmp(command_str, commands[i].name) == 0) {
            commands[i].func(prompt);
            return;
        }
    }
    PushLine("no such command");
}

bool IsInDebugConsole() {
    return is_console_shown;
}

void DrawDebugConsole() {
    bool allow_toggle = !GetGlobalState()->IsKeyBoardFocused() || is_console_shown;
    if (allow_toggle && IsKeyPressed(KEY_F10)) {
        is_console_shown = !is_console_shown;
    }
    if (!is_console_shown) {
        return;
    }
    
    int height = 500;
    if (height > GetScreenHeight()) height = GetScreenHeight();
    UIContextCreateNew(0, 0, GetScreenWidth(), height, 16, WHITE);
    UIContextEnclose(BLACK, WHITE);
    int line_height = UIContextCurrent().text_size + UIContextCurrent().text_margin_y;
    const int input_height = 30;
    const int shown_lines = (height - input_height) / line_height;
    for(int i = shown_lines-1; i >= 0; i--) {
        int show_line = (line_index - i) % DEBUG_CONSOLE_MAX_LINES;
        if (show_line < 0) show_line += DEBUG_CONSOLE_MAX_LINES;
        if (lines[show_line] == NULL) {
            UIContextWrite("");
        }
        else {
            UIContextWrite(lines[show_line]);
        }
    }

    for(;;) {
        int c = GetCharPressed();
        int key = GetKeyPressed();
        int prompt_len = strlen(current_prompt);
        if (key == 0) break;
        if (key == KEY_ENTER || key == KEY_KP_ENTER) {
            strcpy(prompt_backlog[prompt_backlog_index], current_prompt);
            prompt_backlog_index = (prompt_backlog_index + 1) % DEBUG_CONSOLE_PROMPT_BACKLOG_SIZE;
            StringBuilder sb;
            sb.Add(" > ").Add(current_prompt);
            PushLine(sb.c_str);
            InterpreteResult(current_prompt);

            current_prompt[0] = '\0';
            cursor = 0;
        }
        else if (key == KEY_BACKSPACE && cursor > 0) {
            cursor--;
            for(int i=cursor; i < prompt_len; i++) {
                current_prompt[i] = current_prompt[i+1];
            }
        }
        else if (key == KEY_DELETE) {
            for(int i=cursor; i < prompt_len; i++) {
                current_prompt[i] = current_prompt[i+1];
            }
        }
        else if (key == KEY_LEFT && cursor > 0) {
            cursor--;
        }
        else if (key == KEY_RIGHT && cursor < prompt_len) {
            cursor++;
        }
        else if (key == KEY_UP) {
            prompt_backlog_offset++;
            int search_index = (prompt_backlog_index - prompt_backlog_offset) % DEBUG_CONSOLE_PROMPT_BACKLOG_SIZE;
            strcpy(current_prompt, prompt_backlog[search_index]);
            if (cursor < strlen(current_prompt)) cursor = strlen(current_prompt);
        }
        else if (key == KEY_DOWN) {
            prompt_backlog_offset--;
            int search_index = (prompt_backlog_index - prompt_backlog_offset) % DEBUG_CONSOLE_PROMPT_BACKLOG_SIZE;
            strcpy(current_prompt, prompt_backlog[search_index]);
            if (cursor < strlen(current_prompt)) cursor = strlen(current_prompt);
        }
        else if (key == KEY_END) {
            cursor = prompt_len;
        }
        else if (key == KEY_HOME) {
            cursor = 0;
        }
        else if (c != 0 && strlen(current_prompt) < DEBUG_CONSOLE_MAX_LINE_SIZE){
            for(int i=prompt_len; i >= cursor; i--) {
                current_prompt[i+1] = current_prompt[i];
            }
            current_prompt[cursor] = c;
            cursor++;
        }
    }
    
    StringBuilder sb;
    sb.Add(" $ ").Add(current_prompt);
    UIContextWrite(sb.c_str);
    int x_offset = MeasureTextEx(GetCustomDefaultFont(), TextSubtext(sb.c_str, 0, cursor + 3), 16, 1).x;
    DrawLine(x_offset, UIContextCurrent().y_cursor, x_offset, UIContextCurrent().y_cursor - line_height, WHITE);
}
