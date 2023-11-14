#include "modules.hpp"
#include "logging.hpp"
#include <map>

std::map<std::string, module_index_t> module_ids = std::map<std::string, module_index_t>();
Module* modules = NULL;
size_t module_count = 0;

ResourceTransfer ResourceTransferInvert(ResourceTransfer rt) {
    rt.quantity = -rt.quantity;
    return rt;
}

void Module::Effect(resource_count_t* resource_delta, resource_count_t* stats) const {
    for(int i=0; i < RESOURCE_MAX; i++) {
        resource_delta[i] = resource_delta_contributions[i];
    }
    for (int i=0; i < STAT_MAX; i++) {
        stats[i] += stat_contributions[i];
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
    modules = (Module*) malloc(sizeof(Module) * module_count);
    for (int module_index=0; module_index < module_count; module_index++) {
        const DataNode* mod_data = data->GetArrayChild("modules", module_index);
        Module mod = {0};

        strncpy(mod.name, mod_data->Get("name", "[NAME MISSING]"), MODULE_NAME_MAX_SIZE);
        strncpy(mod.description, mod_data->Get("description", "[DESCRITION MISSING]"), MODULE_DESCRIPTION_MAX_SIZE);

        const DataNode* resource_delta = mod_data->GetChild("resource_delta", true);
        if (resource_delta != NULL) {
            for (int resource_index=0; resource_index < resource_delta->GetChildCount(); resource_index++) {
                const char* resource_id = resource_delta->GetKey(resource_index);
                double value = resource_delta->GetF(resource_id);

                NOT_IMPLEMENTED
            }
        }
        
        const DataNode* stats = mod_data->GetChild("stats", true);
        if (stats != NULL) {
            for (int resource_index=0; resource_index < stats->GetChildCount(); resource_index++) {
                const char* stat_name = stats->GetKey(resource_index);
                double value = stats->GetF(stat_name);

                NOT_IMPLEMENTED
            }
        }

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

Module *GetModuleByIndex(module_index_t index) {
    if (index >= module_count) {
        ERROR("Invalid module index (%d >= %d or negative)", index, module_count)
        return NULL;
    }
    return &modules[index];
}
