#include "debug_console.hpp"
#include "raylib.h"
#include "ui.hpp"
#include "constants.hpp"
#include "global_state.hpp"
#include "basic.hpp"

#define SETTINGS_FILE_PATH "settings.yaml"

namespace settings {
    Setting* list = NULL;
    int size = 0;

    void Init() {
        DataNode dn;
        DataNode::FromFile(&dn, SETTINGS_FILE_PATH, FileFormat::Auto, true);
        size = dn.GetFieldCount();
        list = new Setting[size];
        for(int i=0; i < size; i++) {
            const char* key = dn.GetKey(i);
            list[i] = Setting(key, dn.Get(key));
        }
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

const char *Setting::Get() const {
    return overrides[override_index];
}

bool Setting::GetAsBool() const {
    const char* c = overrides[override_index];
    return strcmp(c, "y") * strcmp(c, "true") == 0;
}

double Setting::GetAsDouble() const {
    return atof(overrides[override_index]);
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

const char* GetSettingStr(const char* key, const char* default_) {
    Setting* setting = GetSetting(key);
    if (setting == NULL) {
        return default_;
    }
    return setting->Get();
}


bool GetSettingBool(const char* key, bool default_) {
    Setting* setting = GetSetting(key);
    if (setting == NULL) {
        return default_;
    }
    return setting->GetAsBool();
}

double GetSettingNum(const char* key, double default_) {
    Setting* setting = GetSetting(key);
    if (setting == NULL) {
        return default_;
    }
    return setting->GetAsDouble();
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

    bool is_console_shown = false;
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

void Help(const char*);
void SetSetting(const char*);
void ListSettings(const char*);
void SaveSettings(const char*);
void RelaodSettings(const char*);
void GiveResource(const char*);

struct { const char* name; void(*func)(const char*); } commands[] = {
    { "help", Help },
    { "set", SetSetting },
    { "list", ListSettings },
    { "save", SaveSettings },
    { "reload", RelaodSettings },
    { "give_rsc", GiveResource },
};

void Help(const char* prompt) {
    for(int i=0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        PushLine(commands[i].name);
    }
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

void ReloadShaders(const char* prompt) {
    ReloadShaders();
    RenderServer::ReloadShaders();
}

void SaveSettings(const char* prompt) {
    if (settings::list == NULL) { settings::Init(); }
    DataNode dn;
    for(int i=0; i < settings::size; i++) {
        dn.Set(settings::list[i].name, settings::list[i].Get());  // Applies any overrides
    }
    dn.WriteToFile(SETTINGS_FILE_PATH, FileFormat::YAML);
}

void RelaodSettings(const char* prompt) {
    settings::Init();
}

void GiveResource(const char* prompt) {
    static char planet_name[DEBUG_CONSOLE_MAX_LINE_SIZE];
    static char resource_name[DEBUG_CONSOLE_MAX_LINE_SIZE];
    static char resource_value[DEBUG_CONSOLE_MAX_LINE_SIZE];
    prompt = FetchArg(planet_name, prompt);
    prompt = FetchArg(resource_name, prompt);
    if (*prompt == '\0') {
        PushLine("3 arguments expected");
        return;
    }
    prompt = FetchArg(resource_value, prompt);
    
    ResourceType rsc_index = RESOURCE_NONE;
    for (int i=0; i < RESOURCE_MAX; i++) {
        if (strcmp(resource_name, resource_names[i]) == 0)
            rsc_index = (ResourceType)i;
    }
    if (rsc_index < 0) {
        PushLine("Invalid resource name");
        return;
    }

    Planet* planet = GetPlanet(GetPlanets()->GetIndexByName(planet_name));
    if (planet == NULL) {
        PushLine("No such planet");
    }
    int val = TextToInteger(resource_value);
    planet->economy.GiveResource(ResourceTransfer(rsc_index, val));
}

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
    ui::CreateNew(0, 0, GetScreenWidth(), height, DEFAULT_FONT_SIZE, WHITE, BLACK);
    ui::Current()->z_layer = 254;
    ui::Enclose();
    int line_height = ui::Current()->text_size + ui::Current()->text_margin_y;
    const int input_height = 30;
    const int shown_lines = (height - input_height) / line_height;
    for(int i = shown_lines-1; i >= 0; i--) {
        int show_line = (line_index - i) % DEBUG_CONSOLE_MAX_LINES;
        if (show_line < 0) show_line += DEBUG_CONSOLE_MAX_LINES;
        ui::Write(lines[show_line]);
    }

    for(int c = GetCharPressed(); c != 0; c = GetCharPressed()) {
        int prompt_len = strlen(current_prompt);
        if (c != 0 && strlen(current_prompt) < DEBUG_CONSOLE_MAX_LINE_SIZE){
            for(int i=prompt_len; i >= cursor; i--) {
                current_prompt[i+1] = current_prompt[i];
            }
            current_prompt[cursor] = c;
            cursor++;
        }
    }
    
    for(int key = GetKeyPressed(); key != 0; key = GetKeyPressed()) {
        int prompt_len = strlen(current_prompt);
        if (key == KEY_ENTER || key == KEY_KP_ENTER) {
            strcpy(prompt_backlog[prompt_backlog_index], current_prompt);
            prompt_backlog_index = (prompt_backlog_index + 1) % DEBUG_CONSOLE_PROMPT_BACKLOG_SIZE;
            StringBuilder sb;
            sb.Add(" > ").Add(current_prompt);
            PushLine(sb.c_str);
            InterpreteResult(current_prompt);

            current_prompt[0] = '\0';
            cursor = 0;
            prompt_backlog_offset = 0;
        }
        else if (key == KEY_C && IsKeyDown(KEY_LEFT_CONTROL)) {
            SetClipboardText(current_prompt);
        }
        else if (key == KEY_V && IsKeyDown(KEY_LEFT_CONTROL)) {
            const char* insert = GetClipboardText();
            strncpy(&current_prompt[cursor], insert, DEBUG_CONSOLE_MAX_LINE_SIZE - cursor);
            cursor += strlen(insert);
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
    }
    
    StringBuilder sb;
    sb.Add(" $ ").Add(current_prompt);
    ui::Write(sb.c_str);
    int x_offset = ui::Current()->text_start_x + MeasureTextEx(GetCustomDefaultFont(), TextSubtext(sb.c_str, 0, cursor + 3), DEFAULT_FONT_SIZE, 1).x;
    BeginRenderInUIMode(ui::Current()->z_layer);
    DrawLine(x_offset, ui::Current()->y_cursor, x_offset, ui::Current()->y_cursor - line_height, WHITE);
    EndRenderInUIMode();
}
