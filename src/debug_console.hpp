#ifndef DEBUG_CONSOLE_H
#define DEBUG_CONSOLE_H

#define SETTING_MAX_SIZE 256
#define SETTING_MAX_OVERRIDES 10

bool IsInDebugConsole();
void DrawDebugConsole();

struct Setting {
	char name[SETTING_MAX_SIZE] = "";
    char overrides[SETTING_MAX_SIZE][SETTING_MAX_OVERRIDES] = {0};
    int override_index = 0;

    Setting() = default;
    Setting(const char* name, const char* value);

    const char* Get() const;
    bool GetAsBool() const;
    double GetAsDouble() const;
    void Set(const char* value);
    void PushOverride(const char* value);
    void PopOverride();
};

Setting* GetSetting(const char* key);
const char* GetSettingStr(const char* key, const char* default_="");
bool GetSettingBool(const char* key, bool default_=false);
double GetSettingNum(const char* key, double default_=0);

#endif  // DEBUG_CONSOLE_H