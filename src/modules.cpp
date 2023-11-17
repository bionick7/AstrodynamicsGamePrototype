#include "modules.hpp"
#include "logging.hpp"
#include "ui.hpp"
#include "debug_drawing.hpp"
#include "global_state.hpp"
#include <map>

std::map<std::string, module_index_t> module_ids = std::map<std::string, module_index_t>();
ModuleClass* modules = NULL;
size_t module_count = 0;

ResourceTransfer ResourceTransferInvert(ResourceTransfer rt) {
    rt.quantity = -rt.quantity;
    return rt;
}


ModuleInstance::ModuleInstance(module_index_t p_class_index) {
    class_index = p_class_index;
    disabled = false;
}

bool ModuleInstance::IsValid() {
    if (class_index == MODULE_INDEX_INVALID) {
        return false;
    }
    const ModuleClass* module_class = GetModuleByIndex(class_index);
    if (module_class == NULL) {
        return false;
    }
    return true;
}

void ModuleInstance::Effect(resource_count_t* resource_delta, resource_count_t* stats) {
    const ModuleClass* module_class = GetModuleByIndex(class_index);
    disabled = false;
    for (int i=0; i < STAT_MAX; i++) {
        if (stats[i] < module_class->stat_required[i]) {
            disabled = true;
            return;
        }
    }
    for(int i=0; i < RESOURCE_MAX; i++) {
        resource_delta[i] += module_class->resource_delta_contributions[i];
    }
    for (int i=0; i < STAT_MAX; i++) {
        stats[i] += module_class->stat_contributions[i];
        stats[i] -= module_class->stat_required[i];
    }
}

void _DrawRelevantStatsFromArray(std::stringstream& ss, const resource_count_t array[], const char array_names[][MAX_NAME_LENGTH], int array_size, resource_count_t scaler, const char* suffix) {
    for (int i=0; i < array_size; i++) {
        if (array[i] > 0) {
            char temp[1024];
            sprintf(&temp[0], "%s: %+3.0f%s", array_names[i], array[i] / scaler, suffix);
            ss << std::string(temp) << "\n";
        }
    }
}

bool ModuleInstance::UIDraw() {
    UIContextPushInset(3, 16);
    ButtonStateFlags button_state = UIContextAsButton();
    if (button_state & BUTTON_STATE_FLAG_HOVER) {
        if (!IsValid()) {
            UIContextEnclose(1, 1, BG_COLOR, PALETTE_RED);
        } else if (disabled) {
            UIContextEnclose(1, 1, BG_COLOR, PALETTE_RED);
        } else {
            const ModuleClass* module_class = GetModuleByIndex(class_index);
            UIContextEnclose(1, 1, BG_COLOR, MAIN_UI_COLOR);
            std::stringstream ss = std::stringstream();
            ss << module_class->name << "\n";
            ss << module_class->description << "\n";
            _DrawRelevantStatsFromArray(ss, module_class->resource_delta_contributions, resources_names, RESOURCE_MAX, 1000, "T");
            _DrawRelevantStatsFromArray(ss, module_class->stat_contributions, stat_names, STAT_MAX, 1, "");
            _DrawRelevantStatsFromArray(ss, module_class->stat_required, stat_names, STAT_MAX, -1, "");
            UISetMouseHint(ss.str().c_str());
        }
    } 
    // Write stuff
    if (IsValid()) {
        UIContextWrite(GetModuleByIndex(class_index)->name);
    } else {
        UIContextWrite("EMPTY");
    }
    UIContextPop();
    return button_state & BUTTON_STATE_FLAG_JUST_PRESSED;
}

void _LoadArray(const DataNode* data_node, resource_count_t array[], const char array_names[][MAX_NAME_LENGTH], int array_size, resource_count_t scaler) {
    if (data_node != NULL) {
        for (int i=0; i < array_size; i++) {
            array[i] = data_node->GetF(array_names[i], 0, true) * scaler;  // t -> kg
        }
    }
}

int LoadModules(const DataNode* data) {
    if (modules != NULL) {
        WARNING("Loading modules more than once (possible memory leak)");
    }
    module_count = data->GetArrayChildLen("modules");
    if (module_count == 0){
        WARNING("No modules loaded")
        return 0;
    }
    modules = (ModuleClass*) malloc(sizeof(ModuleClass) * module_count);
    for (int module_index=0; module_index < module_count; module_index++) {
        const DataNode* mod_data = data->GetArrayChild("modules", module_index);
        ModuleClass mod = {0};

        strncpy(mod.name, mod_data->Get("name", "[NAME MISSING]"), MODULE_NAME_MAX_SIZE);
        strncpy(mod.description, mod_data->Get("description", "[DESCRITION MISSING]"), MODULE_DESCRIPTION_MAX_SIZE);

        _LoadArray(mod_data->GetChild("resource_delta", true), mod.resource_delta_contributions, resources_names, RESOURCE_MAX, 1000);
        _LoadArray(mod_data->GetChild("build_cost", true), mod.build_costs, resources_names, RESOURCE_MAX, 1000);
        _LoadArray(mod_data->GetChild("stat_increase", true), mod.stat_contributions, stat_names, STAT_MAX, 1);
        _LoadArray(mod_data->GetChild("stat_require", true), mod.stat_required, stat_names, STAT_MAX, 1);

        modules[module_index] = mod;
        module_ids.insert_or_assign(mod_data->Get("id", "_"), module_index);
    }
    return module_count;
}

module_index_t GetModuleIndexById(const char *id) {
    auto find = module_ids.find(id);
    if (find == module_ids.end()) {
        ERROR("No such module id '%s", id)
        return MODULE_INDEX_INVALID;
    }
    return find->second;
}

const ModuleClass *GetModuleByIndex(module_index_t index) {
    if (index >= module_count) {
        ERROR("Invalid module index (%d >= %d or negative)", index, module_count)
        return NULL;
    }
    return &modules[index];
}

bool show_module_construction_ui = false;
entity_id_t module_construction_planet_id = GetInvalidId();
int module_construction_slot_index = -1;

void ModuleConstructionOpen(entity_id_t planet_id, int slot_index) {
    show_module_construction_ui = true;
    module_construction_planet_id = planet_id;
    module_construction_slot_index = slot_index;
}

void ModuleConstructionClose() {
    show_module_construction_ui = false;
    module_construction_planet_id = GetInvalidId();
    module_construction_slot_index = -1;
}

bool ModuleConstructionIsOpen() {
    return show_module_construction_ui;
}

void ModuleConstructionUI() {
    const int sprite_margin_out = 2;
    const int sprite_margin_tot = sprite_margin_out + 2;

    if (!show_module_construction_ui) {
        return;
    }

    UIContextCreate(16*30 + 20, 10, 4*(32+sprite_margin_tot), 4*(32+sprite_margin_tot), 16, MAIN_UI_COLOR);
    UIContextEnclose(0, 0, BG_COLOR, MAIN_UI_COLOR);
    for (module_index_t i=0; i < module_count && i < 16; i++) {
        const ModuleClass* module_class = GetModuleByIndex(i);

        UIContextPushGridCell(4, 4, i % 4, i / 4);
        ButtonStateFlags state_flags = UIContextAsButton();
        if (state_flags & BUTTON_STATE_FLAG_HOVER) {
            UIContextEnclose(-2, -2, BG_COLOR, MAIN_UI_COLOR);
            std::stringstream ss = std::stringstream();
            ss << module_class->name << "\n";
            ss << module_class->description << "\n";
            _DrawRelevantStatsFromArray(ss, module_class->resource_delta_contributions, resources_names, RESOURCE_MAX, 1000, "T");
            _DrawRelevantStatsFromArray(ss, module_class->stat_contributions, stat_names, STAT_MAX, 1, "");
            _DrawRelevantStatsFromArray(ss, module_class->stat_required, stat_names, STAT_MAX, -1, "");
            ss << "COST:\n";
            _DrawRelevantStatsFromArray(ss, module_class->build_costs, resources_names, RESOURCE_MAX, 1000, "T");
            UISetMouseHint(ss.str().c_str());
        }
        if (state_flags & BUTTON_STATE_FLAG_JUST_PRESSED) {
            GetPlanet(module_construction_planet_id).RequestBuild(module_construction_slot_index, i);
            ModuleConstructionClose();
        }
        
        UIContextPop();  // GridCell
    }
}
