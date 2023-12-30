#include "buildings.hpp"
#include "logging.hpp"
#include "ui.hpp"
#include "debug_drawing.hpp"
#include "global_state.hpp"
#include "constants.hpp"
#include <map>

std::map<std::string, building_index_t> building_ids = std::map<std::string, building_index_t>();
BuildingClass* buildings = NULL;
size_t building_count = 0;

BuildingInstance::BuildingInstance(building_index_t p_class_index) {
    class_index = p_class_index;
    disabled = false;
}

bool BuildingInstance::IsValid() const {
    if (class_index == BUILDING_INDEX_INVALID) {
        return false;
    }
    const BuildingClass* building_class = GetBuildingByIndex(class_index);
    if (building_class == NULL) {
        return false;
    }
    return true;
}

void BuildingInstance::Effect(resource_count_t* resource_delta, resource_count_t* stats) {
    const BuildingClass* building_class = GetBuildingByIndex(class_index);
    disabled = false;
    for (int i=0; i < STAT_MAX; i++) {
        if (stats[i] < building_class->stat_required[i]) {
            disabled = true;
            return;
        }
    }
    for(int i=0; i < RESOURCE_MAX; i++) {
        resource_delta[i] += building_class->resource_delta_contributions[i];
    }
    for (int i=0; i < STAT_MAX; i++) {
        stats[i] += building_class->stat_contributions[i];
        stats[i] -= building_class->stat_required[i];
    }
}

void _DrawRelevantStatsFromArray(
    std::stringstream& ss, 
    const resource_count_t array[], const char array_names[][RESOURCE_NAME_MAX_SIZE], 
    int array_size, resource_count_t scaler, const char* suffix
) {
    for (int i=0; i < array_size; i++) {
        if (array[i] > 0) {
            char temp[1024];
            sprintf(&temp[0], "%s: %+3d%s", array_names[i], array[i] / scaler, suffix);
            ss << std::string(temp) << "\n";
        }
    }
}

bool BuildingInstance::UIDraw() {
    UIContextPushInset(3, 16);
    ButtonStateFlags button_state = UIContextAsButton();
    HandleButtonSound(button_state & (BUTTON_STATE_FLAG_JUST_PRESSED | BUTTON_STATE_FLAG_JUST_HOVER_IN));
    if (button_state & BUTTON_STATE_FLAG_HOVER) {
        if (!IsValid()) {
            UIContextEnclose(Palette::bg, Palette::red);
        } else if (disabled) {
            UIContextEnclose(Palette::bg, Palette::red);
        } else {
            const BuildingClass* building_class = GetBuildingByIndex(class_index);
            UIContextEnclose(Palette::bg, Palette::ui_main);
            std::stringstream ss = std::stringstream();
            ss << building_class->name << "\n";
            ss << building_class->description << "\n";
            _DrawRelevantStatsFromArray(ss, building_class->resource_delta_contributions, resource_names, RESOURCE_MAX, 1000, "T");
            _DrawRelevantStatsFromArray(ss, building_class->stat_contributions, stat_names, STAT_MAX, 1, "");
            _DrawRelevantStatsFromArray(ss, building_class->stat_required, stat_names, STAT_MAX, -1, "");
            UISetMouseHint(ss.str().c_str());
        }
    } 
    // Write stuff
    if (IsValid()) {
        UIContextWrite(GetBuildingByIndex(class_index)->name);
    } else {
        UIContextWrite("EMPTY");
    }
    UIContextPop();
    return button_state & BUTTON_STATE_FLAG_JUST_PRESSED;
}

void _LoadArray(const DataNode* data_node, resource_count_t array[], const char array_names[][RESOURCE_NAME_MAX_SIZE], int array_size, resource_count_t scaler) {
    if (data_node != NULL) {
        for (int i=0; i < array_size; i++) {
            array[i] = data_node->GetF(array_names[i], 0, true) * scaler;  // t -> kg
        }
    }
}

void _WriteSingleBuildingToFile(FILE* file, const BuildingClass* mc) {
    fprintf(file, "%s : ", mc->name);
    for (int i=0; i < RESOURCE_MAX; i++) {
        if (mc->resource_delta_contributions[i] < 0) {
            fprintf(file, "%5.0d/d %s", -mc->resource_delta_contributions[i], GetResourceData(i).name);
        }
    }
    for (int i=0; i < STAT_MAX; i++) {
        if (mc->stat_required[i] > 0) {
            fprintf(file, "%3d %s", mc->stat_required[i], stat_names[i]);
        }
    }
    fprintf(file, " ==> ");
    for (int i=0; i < RESOURCE_MAX; i++) {
        if (mc->resource_delta_contributions[i] > 0) {
            fprintf(file, "%5.0d/d %s", mc->resource_delta_contributions[i], GetResourceData(i).name);
        }
    }
    for (int i=0; i < STAT_MAX; i++) {
        if (mc->stat_contributions[i] > 0) {
            fprintf(file, "%5d %s", mc->stat_contributions[i], stat_names[i]);
        }
    }
    fprintf(file, "\n");

}

void WriteBuildingsToFile(const char* filename) {
    //if (!FileExists(filename)) {
    //    ERROR("Could not find file '%s'", filename)
    //    return;
    //}
    FILE* file = fopen(filename, "w");
    for (int i=0; i < RESOURCE_MAX; i++) {
        fprintf(file, " ==== %s ====\n", GetResourceData(i).name);
        for (int building_index=0; building_index < building_count; building_index++) {
            const BuildingClass* mc = &buildings[building_index];
            if (mc->resource_delta_contributions[i] != 0){
                _WriteSingleBuildingToFile(file, mc);
            }
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

int LoadBuildings(const DataNode* data) {
    if (buildings != NULL) {
        WARNING("Loading buildings more than once (I'm not freeing this memory)");
    }
    building_count = data->GetArrayChildLen("buildings");
    if (building_count == 0){
        WARNING("No buildings loaded")
        return 0;
    }
    buildings = (BuildingClass*) malloc(sizeof(BuildingClass) * building_count);
    for (int building_index=0; building_index < building_count; building_index++) {
        const DataNode* mod_data = data->GetArrayChild("buildings", building_index);
        BuildingClass mod = {0};

        strncpy(mod.name, mod_data->Get("name", "[NAME MISSING]"), BUILDING_NAME_MAX_SIZE);
        strncpy(mod.description, mod_data->Get("description", "[DESCRITION MISSING]"), BUILDING_DESCRIPTION_MAX_SIZE);

        _LoadArray(mod_data->GetChild("resource_delta", true), mod.resource_delta_contributions, resource_names, RESOURCE_MAX, 1000);
        _LoadArray(mod_data->GetChild("build_cost", true), mod.build_costs, resource_names, RESOURCE_MAX, 1000);
        _LoadArray(mod_data->GetChild("stat_increase", true), mod.stat_contributions, stat_names, STAT_MAX, 1);
        _LoadArray(mod_data->GetChild("stat_require", true), mod.stat_required, stat_names, STAT_MAX, 1);

        buildings[building_index] = mod;
        auto pair = building_ids.insert_or_assign(mod_data->Get("id", "_"), building_index);
        buildings[building_index].id = pair.first->first.c_str();  // points to string in dictionary
    }
    return building_count;
}

building_index_t GetBuildingIndexById(const char *id) {
    auto find = building_ids.find(id);
    if (find == building_ids.end()) {
        ERROR("No such building id '%s'", id)
        return BUILDING_INDEX_INVALID;
    }
    return find->second;
}

const BuildingClass *GetBuildingByIndex(building_index_t index) {
    if (buildings == NULL) {
        ERROR("Buildings uninitialized")
        return NULL;
    }
    if (index >= building_count) {
        ERROR("Invalid building index (%d >= %d or negative)", index, building_count)
        return NULL;
    }
    return &buildings[index];
}

bool show_building_construction_ui = false;
entity_id_t building_construction_planet_id = GetInvalidId();
int building_construction_slot_index = -1;

void BuildingConstructionOpen(entity_id_t planet_id, int slot_index) {
    show_building_construction_ui = true;
    building_construction_planet_id = planet_id;
    building_construction_slot_index = slot_index;
}

void BuildingConstructionClose() {
    show_building_construction_ui = false;
    building_construction_planet_id = GetInvalidId();
    building_construction_slot_index = -1;
}

bool BuildingConstructionIsOpen() {
    return show_building_construction_ui;
}

void BuildingConstructionUI() {
    const int sprite_margin_out = 2;
    const int sprite_margin_tot = sprite_margin_out + 2;

    if (!show_building_construction_ui) {
        return;
    }

    UIContextCreateNew(16*30 + 20, 10, 4*(32+sprite_margin_tot), 4*(32+sprite_margin_tot), 16, Palette::ui_main);
    UIContextEnclose(Palette::bg, Palette::ui_main);
    for (building_index_t i=0; i < building_count && i < 16; i++) {
        const BuildingClass* building_class = GetBuildingByIndex(i);

        UIContextPushGridCell(4, 4, i % 4, i / 4);
        ButtonStateFlags button_state = UIContextAsButton();
        HandleButtonSound(button_state & (BUTTON_STATE_FLAG_JUST_PRESSED | BUTTON_STATE_FLAG_JUST_HOVER_IN));
        if (button_state & BUTTON_STATE_FLAG_HOVER) {
            UIContextShrink(2, 2);
            UIContextEnclose(Palette::bg, Palette::ui_main);
            std::stringstream ss = std::stringstream();
            ss << building_class->name << "\n";
            ss << building_class->description << "\n";
            _DrawRelevantStatsFromArray(ss, building_class->resource_delta_contributions, resource_names, RESOURCE_MAX, 1000, "T");
            _DrawRelevantStatsFromArray(ss, building_class->stat_contributions, stat_names, STAT_MAX, 1, "");
            _DrawRelevantStatsFromArray(ss, building_class->stat_required, stat_names, STAT_MAX, -1, "");
            ss << "COST:\n";
            _DrawRelevantStatsFromArray(ss, building_class->build_costs, resource_names, RESOURCE_MAX, 1000, "T");
            UISetMouseHint(ss.str().c_str());
        }
        if (button_state & BUTTON_STATE_FLAG_JUST_PRESSED) {
            GetPlanet(building_construction_planet_id)->RequestBuild(building_construction_slot_index, i);
            BuildingConstructionClose();
        }
        
        UIContextPop();  // GridCell
    }
}
