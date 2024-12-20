#include "debug_console.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "global_state.hpp"
#include "basic.hpp"
#include "assets.hpp"
#include "debug_drawing.hpp"
#include "app_meta.hpp"

#include "raylib.h"
#include <sys/time.h>

#define SETTINGS_FILE_PATH "settings.yaml"

void _OnSettingsChanged() {
    ReconfigureWindow();
    
    GlobalState* gs = GetGlobalState();
    gs->camera.macro_camera.fovy = GetSettingNum("fov_deg", 90);
    // ...
}

namespace settings {
    Setting* list = NULL;
    int size = 0;

    void Init() {
        DataNode dn;
        DataNode::FromFile(&dn, SETTINGS_FILE_PATH, file_format::AUTO, true);
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
    _OnSettingsChanged();
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

void Help(const char* );
void SetSetting(const char* );
void ListSettings(const char* );
void SaveSettings(const char* );
void ReloadSettings(const char* );
void GiveResource(const char* );
void UnlockTech(const char* );
void SetVar(const char* );
void GetVar(const char* );
void ListVars(const char* );
void Save(const char* );
void Load(const char* );

struct { const char* name; void(*func)(const char*); } commands[] = {
    { "help", Help },
    { "set", SetSetting },
    { "list_settings", ListSettings },
    { "save_settings", SaveSettings },
    { "reload", ReloadSettings },
    { "give_rsc", GiveResource },
    { "unlock_tech", UnlockTech },
    { "set_var", SetVar },
    { "get_var", GetVar },
    { "list_vars", ListVars },
    { "load", Load },
    { "save", Save },
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
    assets::Reload();
}

void SaveSettings(const char* prompt) {
    if (settings::list == NULL) { settings::Init(); }
    DataNode dn;
    for(int i=0; i < settings::size; i++) {
        dn.Set(settings::list[i].name, settings::list[i].Get());  // Applies any overrides
    }
    dn.WriteToFile(SETTINGS_FILE_PATH, file_format::YAML);
}

void ReloadSettings(const char* prompt) {
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
    
    resources::T rsc_index = resources::NONE;
    for (int i=0; i < resources::MAX; i++) {
        if (strcmp(resource_name, resources::names[i]) == 0)
            rsc_index = (resources::T)i;
    }
    if (rsc_index < 0) {
        PushLine("Invalid resource name");
        return;
    }

    Planet* planet = GetPlanet(GetPlanets()->GetIdByName(planet_name));
    if (planet == NULL) {
        PushLine("No such planet");
    }
    int val = TextToInteger(resource_value);
    //planet->economy.GiveResource(rsc_index, val);
    planet->economy.GiveResource(rsc_index, val);
}

void UnlockTech(const char* prompt) {
    static char tech_id[DEBUG_CONSOLE_MAX_LINE_SIZE];
    if (*prompt == '\0') {
        PushLine("Expected argument: techtreenode id");
        return;
    }
    prompt = FetchArg(tech_id, prompt);
    RID technode_id = GetGlobalState()->GetFromStringIdentifier(tech_id);
    if (IsIdValidTyped(technode_id, EntityType::TECHTREE_NODE)) {
        GetTechTree()->UnlockTechnology(technode_id, true);
    } else {
        PushLine(TextFormat("Invalid tech id '%s'", tech_id));
    }
}

void SetVar(const char* prompt) {
    static char query[DEBUG_CONSOLE_MAX_LINE_SIZE];
    if (*prompt == '\0') {
        PushLine("Expected argument: query");
        return;
    }
    prompt = FetchArg(query, prompt);
    global_vars::Interpret(query);
}

void GetVar(const char* prompt) {
    static char var_name[DEBUG_CONSOLE_MAX_LINE_SIZE];
    if (*prompt == '\0') {
        PushLine("Expected argument: var_name");
        return;
    }
    prompt = FetchArg(var_name, prompt);
    if (!global_vars::HasVar(var_name)) {
        PushLine(TextFormat("No such var '%s'", var_name));
        return;
    }
    PushLine(TextFormat("%s = %d", var_name, global_vars::Get(var_name)));
}

void ListVars(const char* prompt) {
    for (int i=0; i < global_vars::GetVarCount(); i++) {
        const global_vars::GlobalVariable* var = global_vars::GetVarAt(i);
        PushLine(TextFormat("%s = %d", var->name, var->value));
    }
}

void Save(const char* prompt) {
    static char filename[DEBUG_CONSOLE_MAX_LINE_SIZE];
    if (*prompt == '\0') {
        PushLine("Expected argument: filename");
        return;
    }
    prompt = FetchArg(filename, prompt);
    
    GetGlobalState()->SaveGame(filename);

    PushLine(TextFormat("File saved as: %s", filename));
}

void Load(const char* prompt) {
    static char filename[DEBUG_CONSOLE_MAX_LINE_SIZE];
    if (*prompt == '\0') {
        PushLine("Expected argument: filename");
        return;
    }
    prompt = FetchArg(filename, prompt);
    
    GetGlobalState()->LoadGame(filename);

    PushLine(TextFormat("File loaded from: %s", filename));
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
    ui::CreateNew(0, 0, GetScreenWidth(), height, DEFAULT_FONT_SIZE, WHITE, BLACK, z_layers::DEBUG_CONSOLE);
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
    int x_offset = ui::Current()->x + MeasureTextEx(GetCustomDefaultFont(DEFAULT_FONT_SIZE), TextSubtext(sb.c_str, 0, cursor + 3), DEFAULT_FONT_SIZE, 1).x + DEFAULT_FONT_SIZE/2;
    ui::BeginDirectDraw();
    DrawLine(x_offset, ui::Current()->y_cursor, x_offset, ui::Current()->y_cursor - line_height, WHITE);
    ui::EndDirectDraw();
}

timeval timer_stack[100];  // If you stack more than 100 timers, you're doing something wrong
int timer_stack_index = 0;

void PushTimer() {
    if (timer_stack_index >= 100) {
        timer_stack_index = 100;
        return;
    }

    struct timeval start;
    gettimeofday(&start, NULL);

    timer_stack[timer_stack_index++] = start;
}

double PopTimer() {
    if (timer_stack_index <= 0) {
        timer_stack_index = 0;
        return 0.0;
    }

    struct timeval start = timer_stack[--timer_stack_index];
    struct timeval end;
    gettimeofday(&end, NULL);

    int elapsed = ((end.tv_sec - start.tv_sec) * 1000000) + (end.tv_usec - start.tv_usec);
    return elapsed / 1000.0;
}

double PopAndReadTimer(const char *label, bool to_screen) {
    double ms = PopTimer();
    if (!GetSettingBool("show_performance_stats")) return 0;
    if (to_screen) {
        DebugPrintText("%s: %f ms", label, ms);
    } else {
        INFO("%s: %f ms", label, ms);
    }
    return ms;
}
